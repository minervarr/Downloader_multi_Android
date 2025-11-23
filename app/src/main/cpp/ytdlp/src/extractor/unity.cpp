#include "ytdlp/extractor/unity.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// Converted from Python _VALID_URL
// Original: https?://(?:www\.)?unity3d\.com/learn/tutorials/(?:[^/]+/)*(?P<id>[^/?#&]+)
// Note: Python's (?P<id>...) named groups converted to (...) capture groups
const std::vector<std::regex> UnityIE::URL_PATTERNS = {
    std::regex(R"(https?://(?:www\.)?unity3d\.com/learn/tutorials/(?:[^/]+/)*([^/?#&]+))", std::regex::icase),
};

UnityIE::UnityIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string UnityIE::ie_key() const {
    return "Unity";
}

std::string UnityIE::ie_name() const {
    return "Unity";
}

bool UnityIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string UnityIE::extract_id(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        std::smatch match;
        if (std::regex_search(url, match, pattern)) {
            if (match.size() > 1) {
                return match[1].str();
            }
        }
    }
    throw std::runtime_error("Unable to extract video ID from URL: " + url);
}

core::InfoDict UnityIE::_real_extract(const std::string& url) {
    // Extract video ID from URL
    std::string video_id = extract_id(url);

    report_extraction(video_id);

    // Download webpage
    std::string webpage = _download_webpage(url, video_id, "Downloading video page");

    // Search for YouTube video ID in the webpage
    // Original Python: data-video-id="([_0-9a-zA-Z-]+)"
    std::regex youtube_id_regex(R"delim(data-video-id="([_0-9a-zA-Z-]+)")delim");
    std::smatch match;

    if (!std::regex_search(webpage, match, youtube_id_regex)) {
        throw std::runtime_error("Could not find YouTube video ID");
    }

    std::string youtube_id = match[1].str();

    // Return a url_result that delegates to YouTube extractor
    // This is equivalent to Python's: return self.url_result(youtube_id, ie=YoutubeIE.ie_key(), video_id=video_id)
    core::InfoDict result;
    result["_type"] = "url_transparent";
    result["url"] = "https://www.youtube.com/watch?v=" + youtube_id;
    result["ie_key"] = "Youtube";  // Delegate to YouTube extractor
    result["id"] = video_id;
    result["display_id"] = video_id;

    return result;
}

} // namespace ytdlp::extractor
