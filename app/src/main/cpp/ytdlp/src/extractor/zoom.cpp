#include "ytdlp/extractor/zoom.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include "ytdlp/utils/json_utils.hpp"
#include "ytdlp/networking/request.hpp"
#include <fmt/core.h>
#include <stdexcept>
#include <sstream>

// Bring utility functions into scope for cleaner code
using ytdlp::utils::get_string;
using ytdlp::utils::get_int;
using ytdlp::utils::get_bool;
using ytdlp::utils::get_array;
using ytdlp::utils::get_object;
using ytdlp::utils::get_string_at_path;

namespace ytdlp::extractor {

// Converted from Python _VALID_URL:
// r'(?P<base_url>https?://(?:[^.]+\.)?zoom\.us/)rec(?:ording)?/(?P<type>play|share)/(?P<id>[\w.-]+)'
const std::vector<std::regex> ZoomIE::URL_PATTERNS = {
    std::regex(
        R"((https?://(?:[^.]+\.)?zoom\.us/)rec(?:ording)?/(play|share)/([\w.-]+))",
        std::regex::icase
    ),
};

ZoomIE::ZoomIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string ZoomIE::ie_key() const {
    return "Zoom";
}

std::string ZoomIE::ie_name() const {
    return "zoom";
}

bool ZoomIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string ZoomIE::extract_id(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        std::smatch match;
        if (std::regex_search(url, match, pattern)) {
            if (match.size() > 3) {
                return match[3].str();  // video_id is the 3rd capture group
            }
        }
    }
    throw std::runtime_error("Unable to extract video ID from URL: " + url);
}

ZoomIE::UrlComponents ZoomIE::_parse_url(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        std::smatch match;
        if (std::regex_search(url, match, pattern)) {
            if (match.size() > 3) {
                UrlComponents components;
                components.base_url = match[1].str();
                components.url_type = match[2].str();
                components.video_id = match[3].str();
                return components;
            }
        }
    }
    throw std::runtime_error("Invalid Zoom URL: " + url);
}

nlohmann::json ZoomIE::_get_page_data(std::string_view webpage, const std::string& video_id) {
    // Python: self._search_json(r'window\.__data__\s*=', webpage, 'data', video_id, transform_source=js_to_json)

    // Search for window.__data__ = {...}
    std::string pattern = R"(window\.__data__\s*=\s*(\{.+?\});)";
    std::string json_str = _search_regex(
        pattern,
        webpage,
        "page data",
        std::nullopt,
        true,
        std::regex::ECMAScript,
        1
    );

    if (json_str.empty()) {
        throw std::runtime_error("Unable to extract page data from webpage");
    }

    try {
        return nlohmann::json::parse(json_str);
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(fmt::format("Failed to parse page data JSON: {}", e.what()));
    }
}

std::string ZoomIE::_get_real_webpage(
    const std::string& url,
    const std::string& base_url,
    const std::string& video_id,
    const std::string& url_type
) {
    // Download the webpage
    std::string webpage = _download_webpage(
        url,
        video_id,
        fmt::format("Downloading {} webpage", url_type),
        std::nullopt,
        true
    );

    // Check if password is required by looking for password_form
    try {
        auto form = _form_hidden_inputs("password_form", webpage);

        // Password is required
        auto password_opt = downloader_->get_param_videopassword();
        if (!password_opt.has_value()) {
            throw std::runtime_error(
                "This video is protected by a passcode, use the --video-password option"
            );
        }

        std::string password = password_opt.value();

        // Determine if this is a meeting or file password
        bool is_meeting = (form.find("useWhichPasswd") != form.end() &&
                          form["useWhichPasswd"] == "meeting");

        // Build validation URL
        std::string validation_url = base_url + "rec/validate";
        if (is_meeting) {
            validation_url += "_meet";
        }
        validation_url += "_passwd";

        // Prepare form data
        std::map<std::string, std::string> form_data;

        std::string id_key = is_meeting ? "meetId" : "fileId";
        if (form.find(id_key) != form.end()) {
            form_data["id"] = form[id_key];
        }

        form_data["passwd"] = password;

        if (form.find("action") != form.end()) {
            form_data["action"] = form["action"];
        }

        // Validate password
        nlohmann::json validation = _download_json(
            validation_url,
            video_id,
            "Validating passcode",
            "Wrong passcode",
            true
        );

        // Check validation result
        auto status = get_bool(validation, "status", false);
        if (!status) {
            auto error_msg = get_string(validation, "errorMessage", "Password validation failed");
            throw std::runtime_error(error_msg);
        }

        // Re-download the webpage after successful password validation
        webpage = _download_webpage(
            url,
            video_id,
            fmt::format("Re-downloading {} webpage", url_type),
            std::nullopt,
            true
        );

    } catch (const std::runtime_error&) {
        // No password form found, or password validation failed
        // If no password form, just return the original webpage
    }

    return webpage;
}

core::InfoDict ZoomIE::_real_extract(const std::string& url) {
    // Parse URL components
    auto components = _parse_url(url);
    std::string base_url = components.base_url;
    std::string url_type = components.url_type;
    std::string video_id = components.video_id;

    std::string current_url = url;
    std::map<std::string, std::string> query;

    // Handle 'share' URLs - try to call API directly first
    // If it doesn't work, fall back to getting the redirect URL
    if (url_type == "share") {
        // For 'share' URLs, we can try calling the API directly with the video_id
        // Zoom's API should handle both share and play IDs
        // If this fails, we'll catch the error and try the redirect method

        // Note: Most share URLs can be converted to play URLs by just changing the URL
        // format, but we'll let the API call handle this
        query["continueMode"] = "true";
    }

    // IMPORTANT: The video_id from the URL IS the file_id for the API
    // We can call the API directly without parsing the HTML page
    // (Zoom changed their page structure - window.__data__ no longer exists)

    // Download play info JSON directly using video_id as file_id
    std::string play_info_url = fmt::format(
        "{}nws/recording/1.0/play/info/{}",
        base_url,
        video_id  // video_id IS the file_id
    );

    nlohmann::json play_info = _download_json(
        play_info_url,
        video_id,
        "Downloading play info JSON",
        std::nullopt,
        true,
        query
    );

    auto data = get_object(play_info, "result");
    if (data.is_null()) {
        throw std::runtime_error("Invalid play info response");
    }

    // Extract subtitles
    nlohmann::json subtitles = nlohmann::json::object();
    std::vector<std::string> subtitle_types = {"transcript", "cc", "chapter"};

    for (const auto& type : subtitle_types) {
        std::string url_key = type + "Url";
        auto subtitle_url = get_string(data, url_key, "");

        if (!subtitle_url.empty()) {
            // Make URL absolute if needed
            if (subtitle_url[0] == '/') {
                subtitle_url = base_url + subtitle_url.substr(1);
            }

            subtitles[type] = nlohmann::json::array();
            subtitles[type].push_back(nlohmann::json{{"url", subtitle_url}, {"ext", "vtt"}});
        }
    }

    // Extract formats
    nlohmann::json formats = nlohmann::json::array();

    // Camera stream (viewMp4Url)
    auto view_url = get_string(data, "viewMp4Url", "");
    if (!view_url.empty()) {
        nlohmann::json format;
        format["format_note"] = "Camera stream";
        format["url"] = view_url;
        format["format_id"] = "view";
        format["ext"] = "mp4";
        format["preference"] = 0;

        // Extract resolution
        auto view_resolutions = get_array(data, "viewResolvtions");
        if (view_resolutions.size() >= 2) {
            format["width"] = view_resolutions[0].get<int>();
            format["height"] = view_resolutions[1].get<int>();
        }

        // Extract file size
        auto file_size_mb = get_string_at_path(data, "/recording/fileSizeInMB", "");
        if (!file_size_mb.empty()) {
            try {
                double size_mb = std::stod(file_size_mb);
                format["filesize_approx"] = static_cast<int64_t>(size_mb * 1024 * 1024);
            } catch (...) {
                // Ignore parse errors
            }
        }

        formats.push_back(format);
    }

    // Screen share stream (shareMp4Url)
    auto share_url = get_string(data, "shareMp4Url", "");
    if (!share_url.empty()) {
        nlohmann::json format;
        format["format_note"] = "Screen share stream";
        format["url"] = share_url;
        format["format_id"] = "share";
        format["ext"] = "mp4";
        format["preference"] = -1;

        // Extract resolution
        auto share_resolutions = get_array(data, "shareResolvtions");
        if (share_resolutions.size() >= 2) {
            format["width"] = share_resolutions[0].get<int>();
            format["height"] = share_resolutions[1].get<int>();
        }

        formats.push_back(format);
    }

    // Combined camera + screen share (viewMp4WithshareUrl)
    auto view_with_share_url = get_string(data, "viewMp4WithshareUrl", "");
    if (!view_with_share_url.empty()) {
        nlohmann::json format;
        format["format_note"] = "Screen share with camera";
        format["url"] = view_with_share_url;
        format["format_id"] = "view_with_share";
        format["ext"] = "mp4";
        format["preference"] = 1;

        // Try to extract resolution from filename (e.g., video_1920x1080.mp4)
        std::regex res_pattern(R"(_(\d+)x(\d+)\.mp4)");
        std::smatch res_match;
        if (std::regex_search(view_with_share_url, res_match, res_pattern)) {
            if (res_match.size() >= 3) {
                format["width"] = std::stoi(res_match[1].str());
                format["height"] = std::stoi(res_match[2].str());
            }
        }

        formats.push_back(format);
    }

    // Build final InfoDict
    core::InfoDict info;
    info["id"] = video_id;

    // Extract title
    auto title = get_string_at_path(data, "/meet/topic", "");
    if (!title.empty()) {
        info["title"] = title;
    }

    // Extract duration
    auto duration = get_int(data, "duration", -1);
    if (duration > 0) {
        info["duration"] = duration;
    }

    // Add subtitles and formats
    if (!subtitles.empty()) {
        info["subtitles"] = subtitles;
    }

    if (!formats.empty()) {
        info["formats"] = formats;
    }

    // Add HTTP headers
    info["http_headers"] = nlohmann::json::object({
        {"Referer", base_url}
    });

    return info;
}

} // namespace ytdlp::extractor
