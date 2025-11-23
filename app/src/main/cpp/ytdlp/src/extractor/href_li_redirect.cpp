#include "ytdlp/extractor/href_li_redirect.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// Converted from Python _VALID_URL: https?://href\.li/\?(?P<url>.+)
// Named group (?P<url>...) converted to capture group (...)
const std::vector<std::regex> HrefLiRedirectIE::URL_PATTERNS = {
    std::regex(R"(https?://href\.li/\?(.+))", std::regex::icase),
};

HrefLiRedirectIE::HrefLiRedirectIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string HrefLiRedirectIE::ie_key() const {
    return "href.li";
}

std::string HrefLiRedirectIE::ie_name() const {
    return "href.li";
}

bool HrefLiRedirectIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string HrefLiRedirectIE::extract_id(const std::string& url) {
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

core::InfoDict HrefLiRedirectIE::_real_extract(const std::string& url) {
    // Python: return self.url_result(self._match_valid_url(url).group('url'))
    // This is a simple redirect extractor - extract the URL parameter and redirect

    std::smatch match;
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, match, pattern) && match.size() > 1) {
            std::string redirect_url = match[1].str();

            // Create URL result (equivalent to Python's url_result)
            core::InfoDict info;
            info["_type"] = "url";
            info["url"] = redirect_url;
            info["ie_key"] = "Generic";  // Let Generic extractor handle it
            return info;
        }
    }

    throw std::runtime_error("Unable to extract redirect URL from: " + url);
}

} // namespace ytdlp::extractor
