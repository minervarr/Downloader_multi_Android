#include "ytdlp/extractor/generic.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include "ytdlp/utils/network_utils.hpp"
#include <algorithm>
#include <sstream>

namespace ytdlp::extractor {

const std::vector<std::string> GenericExtractor::VIDEO_EXTENSIONS = {
    ".mp4", ".webm", ".m4v", ".mov", ".avi", ".flv", ".mkv", ".wmv", ".3gp", ".ogv"
};

GenericExtractor::GenericExtractor(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string GenericExtractor::ie_key() const {
    return "Generic";
}

std::string GenericExtractor::ie_name() const {
    return "Generic";
}

bool GenericExtractor::is_direct_video_url(const std::string& url) const {
    std::string lower_url = utils::to_lower(url);

    // Remove query parameters for extension check
    size_t query_pos = lower_url.find('?');
    if (query_pos != std::string::npos) {
        lower_url = lower_url.substr(0, query_pos);
    }

    // Remove fragments for extension check
    size_t fragment_pos = lower_url.find('#');
    if (fragment_pos != std::string::npos) {
        lower_url = lower_url.substr(0, fragment_pos);
    }

    // Check if URL ends with video extension
    for (const auto& ext : VIDEO_EXTENSIONS) {
        if (utils::ends_with(lower_url, ext)) {
            return true;
        }
    }

    return false;
}

std::string GenericExtractor::extract_video_id(const std::string& url) const {
    // Try to extract ID from URL path
    auto parsed = utils::parse_url(url);

    // Safely get path and hostname with defaults
    std::string path = parsed.count("path") ? parsed.at("path") : "";
    std::string hostname = parsed.count("hostname") ? parsed.at("hostname") : "unknown";

    // Get the last component of the path
    size_t last_slash = path.find_last_of('/');
    if (last_slash != std::string::npos && last_slash + 1 < path.length()) {
        std::string filename = path.substr(last_slash + 1);

        // Remove extension
        size_t dot_pos = filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            filename = filename.substr(0, dot_pos);
        }

        // Remove query parameters
        size_t query_pos = filename.find('?');
        if (query_pos != std::string::npos) {
            filename = filename.substr(0, query_pos);
        }

        if (!filename.empty()) {
            return filename;
        }
    }

    // Fallback: use hostname + path hash
    return hostname + "_" + std::to_string(std::hash<std::string>{}(path));
}

std::string GenericExtractor::extract_og_video_url(std::string_view html) const {
    // Try og:video:secure_url first (HTTPS)
    std::string video_url = _search_regex(
        R"(<meta[^>]+property=["\']og:video:secure_url["\'][^>]+content=["\']([^"\']+)["\'])",
        html,
        "og:video:secure_url",
        std::nullopt,
        false
    );

    if (!video_url.empty()) {
        return utils::unescape_html(video_url);
    }

    // Try og:video:url
    video_url = _search_regex(
        R"(<meta[^>]+property=["\']og:video:url["\'][^>]+content=["\']([^"\']+)["\'])",
        html,
        "og:video:url",
        std::nullopt,
        false
    );

    if (!video_url.empty()) {
        return utils::unescape_html(video_url);
    }

    // Try og:video
    video_url = _search_regex(
        R"(<meta[^>]+property=["\']og:video["\'][^>]+content=["\']([^"\']+)["\'])",
        html,
        "og:video",
        std::nullopt,
        false
    );

    if (!video_url.empty()) {
        return utils::unescape_html(video_url);
    }

    return "";
}

std::string GenericExtractor::extract_title(std::string_view html, const std::string& url) const {
    // Try Open Graph title
    std::string title = _og_search_title(html, false);
    if (!title.empty()) {
        return title;
    }

    // Try meta name="title"
    title = _html_search_meta({"title"}, html, std::nullopt, std::nullopt, false);
    if (!title.empty()) {
        return title;
    }

    // Try HTML title tag
    title = _html_extract_title(html, "title", false);
    if (!title.empty()) {
        return title;
    }

    // Fallback: use URL hostname
    auto parsed = utils::parse_url(url);
    std::string hostname = parsed.count("hostname") ? parsed.at("hostname") : "video";
    return hostname + " video";
}

std::string GenericExtractor::extract_description(std::string_view html) const {
    // Try Open Graph description
    std::string desc = _og_search_description(html, false);
    if (!desc.empty()) {
        return desc;
    }

    // Try meta name="description"
    desc = _html_search_meta(
        {"description", "Description"},
        html,
        std::nullopt,
        std::nullopt,
        false
    );

    return desc;
}

std::string GenericExtractor::extract_thumbnail(std::string_view html) const {
    // Try Open Graph image
    std::string thumb = _og_search_thumbnail(html, false);
    if (!thumb.empty()) {
        return thumb;
    }

    // Try meta property="image"
    thumb = _html_search_meta(
        {"image", "thumbnail"},
        html,
        std::nullopt,
        std::nullopt,
        false
    );

    if (!thumb.empty()) {
        return thumb;
    }

    // Try link rel="image_src"
    thumb = _search_regex(
        R"(<link[^>]+rel=["\']image_src["\'][^>]+href=["\']([^"\']+)["\'])",
        html,
        "image_src",
        std::nullopt,
        false
    );

    return thumb;
}

std::vector<std::string> GenericExtractor::find_video_urls_in_html(std::string_view html) const {
    std::vector<std::string> urls;

    // Pattern 1: <video> tag with src attribute
    std::regex video_src_pattern(R"(<video[^>]+src=["\']([^"\']+)["\'])");
    std::string html_str(html);
    std::smatch match;
    std::string::const_iterator search_start(html_str.cbegin());

    while (std::regex_search(search_start, html_str.cend(), match, video_src_pattern)) {
        urls.push_back(match[1].str());
        search_start = match.suffix().first;
    }

    // Pattern 2: <source> tag inside <video>
    std::regex source_pattern(R"(<source[^>]+src=["\']([^"\']+)["\'])");
    search_start = html_str.cbegin();

    while (std::regex_search(search_start, html_str.cend(), match, source_pattern)) {
        urls.push_back(match[1].str());
        search_start = match.suffix().first;
    }

    // Remove duplicates
    std::sort(urls.begin(), urls.end());
    urls.erase(std::unique(urls.begin(), urls.end()), urls.end());

    return urls;
}

core::InfoDict GenericExtractor::_real_extract(const std::string& url) {
    std::string video_id = extract_video_id(url);

    report_extraction(video_id);

    core::InfoDict info;
    info["id"] = video_id;
    info["extractor"] = ie_key();
    info["extractor_key"] = ie_key();
    info["webpage_url"] = url;

    // Check if URL is a direct video file
    if (is_direct_video_url(url)) {
        to_screen("[" + ie_key() + "] " + video_id + ": Direct video URL detected");

        info["url"] = url;
        info["title"] = extract_video_id(url);  // Use ID as title for direct URLs
        info["ext"] = utils::determine_ext(url);
        info["_type"] = "video";

        return info;
    }

    // Download webpage
    std::string webpage = _download_webpage(
        url,
        video_id,
        "Downloading webpage"
    );

    if (webpage.empty()) {
        throw std::runtime_error("Failed to download webpage");
    }

    // Extract metadata
    std::string title = extract_title(webpage, url);
    std::string description = extract_description(webpage);
    std::string thumbnail = extract_thumbnail(webpage);

    info["title"] = title;

    if (!description.empty()) {
        info["description"] = description;
    }

    if (!thumbnail.empty()) {
        info["thumbnail"] = thumbnail;
    }

    // Try to find video URL
    std::string video_url = extract_og_video_url(webpage);

    if (video_url.empty()) {
        // Search for video URLs in HTML
        auto found_urls = find_video_urls_in_html(webpage);

        if (!found_urls.empty()) {
            video_url = found_urls[0];  // Use first found URL
        }
    }

    if (!video_url.empty()) {
        info["url"] = video_url;
        info["ext"] = utils::determine_ext(video_url);
        info["_type"] = "video";
    } else {
        // No video URL found - return URL type for further processing
        info["_type"] = "url";
        info["url"] = url;

        report_warning("Could not find video URL, returning URL type", video_id);
    }

    return info;
}

} // namespace ytdlp::extractor
