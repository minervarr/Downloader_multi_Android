#include "ytdlp/extractor/beat_bump_video.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <stdexcept>

namespace ytdlp::extractor {

// Converted from Python _VALID_URL: https?://beatbump\.(?:ml|io)/listen\?id=(?P<id>[\w-]+)
// Named group (?P<id>...) converted to capture group (...)
const std::vector<std::regex> BeatBumpVideoIE::URL_PATTERNS = {
    std::regex(R"(https?://beatbump\.(?:ml|io)/listen\?id=([\w-]+))", std::regex::icase),
};

BeatBumpVideoIE::BeatBumpVideoIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string BeatBumpVideoIE::ie_key() const {
    return "BeatBumpVideo";
}

std::string BeatBumpVideoIE::ie_name() const {
    return "BeatBumpVideo";
}

bool BeatBumpVideoIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string BeatBumpVideoIE::extract_id(const std::string& url) {
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

core::InfoDict BeatBumpVideoIE::_real_extract(const std::string& url) {
    // Python: id_ = self._match_id(url)
    //         return self.url_result(f'https://music.youtube.com/watch?v={id_}', YoutubeIE, id_)
    // Simple URL transformation - BeatBump to YouTube Music

    std::string video_id = extract_id(url);

    // Build YouTube Music URL
    std::string youtube_url = "https://music.youtube.com/watch?v=" + video_id;

    // Create URL result pointing to YouTube
    core::InfoDict info;
    info["_type"] = "url";
    info["url"] = youtube_url;
    info["ie_key"] = "Youtube";
    info["id"] = video_id;

    return info;
}

} // namespace ytdlp::extractor
