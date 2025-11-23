#include "ytdlp/extractor/outside_tvie.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// Converted from Python _VALID_URL: https?://(?:www\.)?outsidetv\.com/(?:[^/]+/)*?play/[a-zA-Z0-9]{8}/\d+/\d+/(?P<id>[a-zA-Z0-9]{8})
// Named group (?P<id>...) converted to capture group (...)
const std::vector<std::regex> OutsideTVIE::URL_PATTERNS = {
    std::regex(R"(https?://(?:www\.)?outsidetv\.com/(?:[^/]+/)*?play/[a-zA-Z0-9]{8}/\d+/\d+/([a-zA-Z0-9]{8}))", std::regex::icase),
};

OutsideTVIE::OutsideTVIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string OutsideTVIE::ie_key() const {
    return "OutsideTV";
}

std::string OutsideTVIE::ie_name() const {
    return "OutsideTV";
}

bool OutsideTVIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string OutsideTVIE::extract_id(const std::string& url) {
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

core::InfoDict OutsideTVIE::_real_extract(const std::string& url) {
    // Python: jw_media_id = self._match_id(url)
    //         return self.url_result('jwplatform:' + jw_media_id, 'JWPlatform', jw_media_id)
    // Redirect to JWPlatform extractor

    std::string jw_media_id = extract_id(url);

    // Create URL result pointing to JWPlatform
    core::InfoDict info;
    info["_type"] = "url";
    info["url"] = "jwplatform:" + jw_media_id;
    info["ie_key"] = "JWPlatform";
    info["id"] = jw_media_id;

    return info;
}

} // namespace ytdlp::extractor
