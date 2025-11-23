#include "ytdlp/extractor/my_vidster.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// TODO: Convert Python _VALID_URL regex to C++ std::regex
// Original pattern: https?://(?:www\.)?myvidster\.com/video/(?P<id>\d+)
const std::vector<std::regex> MyVidsterIE::URL_PATTERNS = {
    std::regex(R"(https?://(?:www\\.)?myvidster\\.com/video/(?P<id>\\d+))", std::regex::icase),
};

MyVidsterIE::MyVidsterIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string MyVidsterIE::ie_key() const {
    return "MyVidster";
}

std::string MyVidsterIE::ie_name() const {
    return "MyVidster";
}

bool MyVidsterIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string MyVidsterIE::extract_id(const std::string& url) {
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

core::InfoDict MyVidsterIE::_real_extract(const std::string& url) {
    // Extract video ID
    std::string video_id = extract_id(url);

    report_extraction(video_id);

    // TODO: Convert Python _real_extract() logic
    // Download webpage
    std::string webpage = _download_webpage(
        url,
        video_id,
        "Downloading video page"
    );

    // TODO: Add extraction logic here
    // - Extract config/JSON data
    // - Extract metadata
    // - Extract formats

    core::InfoDict info;
    info["id"] = video_id;
    info["extractor"] = ie_key();
    info["extractor_key"] = ie_key();
    info["webpage_url"] = url;
    info["_type"] = "video";

    // TODO: Populate info dict with extracted data

    return info;
}

} // namespace ytdlp::extractor
