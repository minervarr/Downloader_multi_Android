#include "ytdlp/extractor/vimeo.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

const std::vector<std::regex> VimeoExtractor::URL_PATTERNS = {
    // https://vimeo.com/123456789
    std::regex(R"(https?://(?:www\.)?vimeo\.com/(\d+))", std::regex::icase),
    // https://player.vimeo.com/video/123456789
    std::regex(R"(https?://player\.vimeo\.com/video/(\d+))", std::regex::icase),
    // https://vimeo.com/channels/staffpicks/123456789
    std::regex(R"(https?://(?:www\.)?vimeo\.com/channels/[^/]+/(\d+))", std::regex::icase),
    // https://vimeo.com/album/1234/video/123456789
    std::regex(R"(https?://(?:www\.)?vimeo\.com/album/\d+/video/(\d+))", std::regex::icase),
    // https://vimeo.com/groups/name/videos/123456789
    std::regex(R"(https?://(?:www\.)?vimeo\.com/groups/[^/]+/videos/(\d+))", std::regex::icase),
};

VimeoExtractor::VimeoExtractor(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string VimeoExtractor::ie_key() const {
    return "Vimeo";
}

std::string VimeoExtractor::ie_name() const {
    return "Vimeo";
}

bool VimeoExtractor::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string VimeoExtractor::extract_id(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        std::smatch match;
        if (std::regex_search(url, match, pattern)) {
            if (match.size() > 1) {
                return match[1].str();
            }
        }
    }
    throw std::runtime_error("Unable to extract Vimeo video ID from URL: " + url);
}

nlohmann::json VimeoExtractor::extract_config(std::string_view webpage, const std::string& video_id) const {
    // Try to find config in various JavaScript variables

    // Pattern 1: var config = {...}
    auto config = _search_json(
        R"(var\s+config\s*=\s*)",
        webpage,
        "config",
        video_id,
        "",
        false
    );

    if (!config.is_null()) {
        return config;
    }

    // Pattern 2: window.vimeo.clip_page_config = {...}
    config = _search_json(
        R"(window\.vimeo\.clip_page_config\s*=\s*)",
        webpage,
        "clip_page_config",
        video_id,
        "",
        false
    );

    if (!config.is_null()) {
        return config;
    }

    // Pattern 3: Look for JSON-LD as fallback
    auto json_lds = _extract_json_ld(webpage, video_id, false);
    if (!json_lds.empty()) {
        // Convert JSON-LD to config-like structure
        nlohmann::json config_from_ld;
        config_from_ld["video"] = json_lds[0];
        return config_from_ld;
    }

    throw std::runtime_error("Unable to extract Vimeo config for video " + video_id);
}

std::vector<nlohmann::json> VimeoExtractor::extract_formats(
    const nlohmann::json& config,
    const std::string& video_id
) const {
    std::vector<nlohmann::json> formats;

    // Extract from request.files.progressive (direct MP4 downloads)
    if (config.contains("request") && config["request"].contains("files")) {
        auto files = config["request"]["files"];

        if (files.contains("progressive") && files["progressive"].is_array()) {
            for (const auto& progressive : files["progressive"]) {
                nlohmann::json format;

                // Required fields
                if (progressive.contains("url")) {
                    format["url"] = progressive["url"];
                } else {
                    continue;  // Skip if no URL
                }

                // Quality/format info
                if (progressive.contains("quality")) {
                    format["format_id"] = progressive["quality"];
                    format["format_note"] = progressive["quality"];
                }

                if (progressive.contains("width")) {
                    format["width"] = progressive["width"];
                }

                if (progressive.contains("height")) {
                    format["height"] = progressive["height"];
                }

                if (progressive.contains("fps")) {
                    format["fps"] = progressive["fps"];
                }

                // File info
                if (progressive.contains("mime")) {
                    std::string mime = progressive["mime"];
                    if (mime.find("video/mp4") != std::string::npos) {
                        format["ext"] = "mp4";
                    }
                }

                formats.push_back(format);
            }
        }
    }

    // If no formats found, this might be a premium/password protected video
    if (formats.empty()) {
        report_warning("No formats found for video " + video_id + ". Video may require authentication or be unavailable.", video_id);
    }

    return formats;
}

core::InfoDict VimeoExtractor::extract_metadata(
    const nlohmann::json& config,
    std::string_view webpage,
    const std::string& video_id
) const {
    core::InfoDict info;

    // Extract title
    if (config.contains("video") && config["video"].contains("title")) {
        info["title"] = core::info::get_string(config["video"], "title");
    } else {
        // Fallback to Open Graph
        std::string og_title = _og_search_title(webpage, false);
        if (!og_title.empty()) {
            info["title"] = og_title;
        } else {
            info["title"] = "Vimeo video " + video_id;
        }
    }

    // Extract description
    if (config.contains("video") && config["video"].contains("description")) {
        std::string desc = core::info::get_string(config["video"], "description");
        if (!desc.empty()) {
            info["description"] = desc;
        }
    } else {
        std::string og_desc = _og_search_description(webpage, false);
        if (!og_desc.empty()) {
            info["description"] = og_desc;
        }
    }

    // Extract thumbnail
    if (config.contains("video") && config["video"].contains("thumbs")) {
        auto thumbs = config["video"]["thumbs"];
        if (thumbs.is_object() && thumbs.contains("1280")) {
            info["thumbnail"] = core::info::get_string(thumbs, "1280");
        } else if (thumbs.is_object() && thumbs.contains("640")) {
            info["thumbnail"] = core::info::get_string(thumbs, "640");
        }
    } else {
        std::string og_thumb = _og_search_thumbnail(webpage, false);
        if (!og_thumb.empty()) {
            info["thumbnail"] = og_thumb;
        }
    }

    // Extract uploader
    if (config.contains("video") && config["video"].contains("owner")) {
        auto owner = config["video"]["owner"];
        if (owner.contains("name")) {
            info["uploader"] = core::info::get_string(owner, "name");
        }
        if (owner.contains("id")) {
            info["uploader_id"] = std::to_string(core::info::get_int(owner, "id"));
        }
    }

    // Extract duration
    if (config.contains("video") && config["video"].contains("duration")) {
        info["duration"] = core::info::get_int(config["video"], "duration");
    }

    // Extract view count
    if (config.contains("video") && config["video"].contains("plays")) {
        info["view_count"] = core::info::get_int(config["video"], "plays");
    }

    return info;
}

core::InfoDict VimeoExtractor::_real_extract(const std::string& url) {
    // Extract video ID
    std::string video_id = extract_id(url);

    report_extraction(video_id);

    // Build video page URL
    std::string video_url = "https://vimeo.com/" + video_id;

    // Download webpage
    std::string webpage = _download_webpage(
        video_url,
        video_id,
        "Downloading video page"
    );

    // Extract config JSON
    nlohmann::json config = extract_config(webpage, video_id);

    // Build InfoDict
    core::InfoDict info;
    info["id"] = video_id;
    info["extractor"] = ie_key();
    info["extractor_key"] = ie_key();
    info["webpage_url"] = video_url;
    info["_type"] = "video";

    // Extract metadata
    core::InfoDict metadata = extract_metadata(config, webpage, video_id);
    for (const auto& [key, value] : metadata.items()) {
        info[key] = value;
    }

    // Extract formats
    auto formats = extract_formats(config, video_id);
    if (!formats.empty()) {
        info["formats"] = formats;
    } else {
        // If no formats, set single URL (might be from JSON-LD)
        if (config.contains("video") && config["video"].contains("contentUrl")) {
            info["url"] = core::info::get_string(config["video"], "contentUrl");
        }
    }

    return info;
}

} // namespace ytdlp::extractor
