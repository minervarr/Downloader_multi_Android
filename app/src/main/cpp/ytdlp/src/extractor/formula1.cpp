#include "ytdlp/extractor/formula1.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// Converted from Python _VALID_URL: https?://(?:www\.)?formula1\.com/en/latest/video\.[^.]+\.(?P<id>\d+)\.html
// Named group (?P<id>...) converted to capture group (...)
const std::vector<std::regex> Formula1IE::URL_PATTERNS = {
    std::regex(R"(https?://(?:www\.)?formula1\.com/en/latest/video\.[^.]+\.(\d+)\.html)", std::regex::icase),
};

Formula1IE::Formula1IE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string Formula1IE::ie_key() const {
    return "Formula1";
}

std::string Formula1IE::ie_name() const {
    return "Formula1";
}

bool Formula1IE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string Formula1IE::extract_id(const std::string& url) {
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

core::InfoDict Formula1IE::_real_extract(const std::string& url) {
    // Python: bc_id = self._match_id(url)
    //         return self.url_result(self.BRIGHTCOVE_URL_TEMPLATE % bc_id, 'BrightcoveNew', bc_id)
    // Extract Brightcove ID and delegate to BrightcoveNew extractor

    std::string bc_id = extract_id(url);

    // Build Brightcove URL
    const std::string brightcove_url_template =
        "http://players.brightcove.net/6057949432001/S1WMrhjlh_default/index.html?videoId=%s";
    char brightcove_url[512];
    std::snprintf(brightcove_url, sizeof(brightcove_url), brightcove_url_template.c_str(), bc_id.c_str());

    // Create URL result pointing to BrightcoveNew
    core::InfoDict info;
    info["_type"] = "url";
    info["url"] = std::string(brightcove_url);
    info["ie_key"] = "BrightcoveNew";
    info["id"] = bc_id;

    return info;
}

} // namespace ytdlp::extractor
