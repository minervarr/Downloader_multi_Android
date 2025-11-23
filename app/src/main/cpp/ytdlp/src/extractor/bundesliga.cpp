#include "ytdlp/extractor/bundesliga.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// Converted from Python _VALID_URL: https?://(?:www\.)?bundesliga\.com/[a-z]{2}/bundesliga/videos(?:/[^?]+)?\?vid=(?P<id>[a-zA-Z0-9]{8})
// Named group (?P<id>...) converted to capture group (...)
const std::vector<std::regex> BundesligaIE::URL_PATTERNS = {
    std::regex(R"(https?://(?:www\.)?bundesliga\.com/[a-z]{2}/bundesliga/videos(?:/[^?]+)?\?vid=([a-zA-Z0-9]{8}))", std::regex::icase),
};

BundesligaIE::BundesligaIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string BundesligaIE::ie_key() const {
    return "Bundesliga";
}

std::string BundesligaIE::ie_name() const {
    return "Bundesliga";
}

bool BundesligaIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string BundesligaIE::extract_id(const std::string& url) {
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

core::InfoDict BundesligaIE::_real_extract(const std::string& url) {
    // Python: video_id = self._match_id(url)
    //         return self.url_result(f'jwplatform:{video_id}', JWPlatformIE, video_id)
    // Redirect to JWPlatform extractor

    std::string video_id = extract_id(url);

    // Create URL result pointing to JWPlatform
    core::InfoDict info;
    info["_type"] = "url";
    info["url"] = "jwplatform:" + video_id;
    info["ie_key"] = "JWPlatform";
    info["id"] = video_id;

    return info;
}

} // namespace ytdlp::extractor
