#ifndef YTDLP_EXTRACTOR_ZOOM_HPP
#define YTDLP_EXTRACTOR_ZOOM_HPP

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
 * Zoom Recording Extractor
 *
 * Supports extraction of Zoom meeting recordings from URLs like:
 * - https://DOMAIN.zoom.us/rec/play/VIDEO_ID
 * - https://DOMAIN.zoom.us/rec/share/VIDEO_ID
 * - https://DOMAIN.zoom.us/recording/play/VIDEO_ID
 * - https://DOMAIN.zoom.us/recording/share/VIDEO_ID
 *
 * Features:
 * - Password-protected recording support
 * - Camera stream extraction (viewMp4Url)
 * - Screen share extraction (shareMp4Url)
 * - Combined camera+screen extraction (viewMp4WithshareUrl)
 * - Subtitle support (transcript, cc, chapter)
 */
class ZoomIE : public InfoExtractor {
public:
    // URL pattern for Zoom recordings
    static const std::vector<std::regex> URL_PATTERNS;

    // Constructor
    explicit ZoomIE(core::YoutubeDL* downloader = nullptr);

    // InfoExtractor interface
    std::string ie_key() const override;
    std::string ie_name() const override;

    // URL matching
    static bool suitable(const std::string& url);
    static std::string extract_id(const std::string& url);

protected:
    // Main extraction logic
    core::InfoDict _real_extract(const std::string& url) override;

private:
    // Helper methods
    nlohmann::json _get_page_data(std::string_view webpage, const std::string& video_id);

    std::string _get_real_webpage(
        const std::string& url,
        const std::string& base_url,
        const std::string& video_id,
        const std::string& url_type
    );

    struct UrlComponents {
        std::string base_url;
        std::string url_type;  // "play" or "share"
        std::string video_id;
    };

    UrlComponents _parse_url(const std::string& url);
};

} // namespace ytdlp::extractor

#endif // YTDLP_EXTRACTOR_ZOOM_HPP
