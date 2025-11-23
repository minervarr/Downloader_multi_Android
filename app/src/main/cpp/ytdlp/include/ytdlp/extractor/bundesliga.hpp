#ifndef YTDLP_EXTRACTOR_BUNDESLIGA_HPP
#define YTDLP_EXTRACTOR_BUNDESLIGA_HPP

#include "ytdlp/extractor/info_extractor.hpp"
#include "ytdlp/core/info_dict.hpp"
#include <string>
#include <vector>
#include <regex>

namespace ytdlp::core {
    class YoutubeDL;
}

namespace ytdlp::extractor {

/**
 * Bundesliga Extractor
 *
 * Supports extraction of Bundesliga videos from URLs like:
 * - https://www.bundesliga.com/en/bundesliga/videos?vid=VIDEO_ID
 * - https://www.bundesliga.com/de/bundesliga/videos/some-title?vid=VIDEO_ID
 *
 * The Bundesliga website hosts videos on JW Platform, so this extractor
 * simply redirects to the JWPlatform extractor with the extracted video ID.
 *
 * Valid URL pattern:
 * https?://(?:www\.)?bundesliga\.com/[a-z]{2}/bundesliga/videos(?:/[^?]+)?\?vid=(?P<id>[a-zA-Z0-9]{8})
 *
 * The video ID is always an 8-character alphanumeric string passed via the 'vid' query parameter.
 */
class BundesligaIE : public InfoExtractor {
public:
    // URL pattern for Bundesliga videos
    static const std::vector<std::regex> URL_PATTERNS;

    // Constructor
    explicit BundesligaIE(core::YoutubeDL* downloader = nullptr);

    // InfoExtractor interface
    std::string ie_key() const override;
    std::string ie_name() const override;

    // URL matching
    static bool suitable(const std::string& url);
    static std::string extract_id(const std::string& url);

protected:
    // Main extraction logic
    // Returns a URL result redirecting to JWPlatform extractor
    core::InfoDict _real_extract(const std::string& url) override;
};

} // namespace ytdlp::extractor

#endif // YTDLP_EXTRACTOR_BUNDESLIGA_HPP
