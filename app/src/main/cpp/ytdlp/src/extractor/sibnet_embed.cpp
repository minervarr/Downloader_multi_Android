#include "ytdlp/extractor/sibnet_embed.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// Note: _VALID_URL = False in Python (embed-only extractor)
// This extractor only works through _EMBED_REGEX detection
// Original _EMBED_REGEX: <iframe\b[^>]+\bsrc=(["\'])(?P<url>(?:https?:)?//video\.sibnet\.ru/shell\.php\?.*?\bvideoid=\d+.*?)\1
// Ref: https://help.sibnet.ru/?sibnet_video_embed
const std::vector<std::regex> SibnetEmbedIE::URL_PATTERNS = {
    // Embed URL pattern for Sibnet video player
    std::regex(R"((?:https?:)?//video\.sibnet\.ru/shell\.php\?.*?\bvideoid=(\d+))", std::regex::icase),
};

SibnetEmbedIE::SibnetEmbedIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string SibnetEmbedIE::ie_key() const {
    return "SibnetEmbed";
}

std::string SibnetEmbedIE::ie_name() const {
    return "SibnetEmbed";
}

bool SibnetEmbedIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string SibnetEmbedIE::extract_id(const std::string& url) {
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

core::InfoDict SibnetEmbedIE::_real_extract(const std::string& url) {
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
