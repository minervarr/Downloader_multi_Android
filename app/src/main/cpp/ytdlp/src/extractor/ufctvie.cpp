#include "ytdlp/extractor/ufctvie.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// TODO: Convert Python _VALID_URL regex to C++ std::regex
// Original pattern: 
const std::vector<std::regex> UFCTVIE::URL_PATTERNS = {
    std::regex(R"()", std::regex::icase),
};

UFCTVIE::UFCTVIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string UFCTVIE::ie_key() const {
    return "UFCTV";
}

std::string UFCTVIE::ie_name() const {
    return "UFCTV";
}

bool UFCTVIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string UFCTVIE::extract_id(const std::string& url) {
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

core::InfoDict UFCTVIE::_real_extract(const std::string& url) {
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
