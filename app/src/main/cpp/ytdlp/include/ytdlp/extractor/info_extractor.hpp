#ifndef YTDLP_EXTRACTOR_INFO_EXTRACTOR_HPP
#define YTDLP_EXTRACTOR_INFO_EXTRACTOR_HPP

#include "ytdlp/core/info_dict.hpp"
#include <string>
#include <optional>
#include <regex>
#include <nlohmann/json.hpp>

namespace ytdlp::core {
    class YoutubeDL;
}

namespace ytdlp::extractor {

class InfoExtractor {
public:
    explicit InfoExtractor(core::YoutubeDL* downloader = nullptr);
    virtual ~InfoExtractor() = default;

    // Main extraction method
    virtual core::InfoDict extract(const std::string& url);

    // Information methods
    virtual std::string ie_key() const;
    virtual std::string ie_name() const;

    // Downloader management
    void set_downloader(core::YoutubeDL* downloader);
    core::YoutubeDL* downloader() const { return downloader_; }

protected:
    // Abstract method to be implemented by subclasses
    virtual core::InfoDict _real_extract(const std::string& url) = 0;

    // Helper methods for downloading and parsing
    std::string _download_webpage(
        const std::string& url_or_request,
        const std::string& video_id,
        const std::optional<std::string>& note = std::nullopt,
        const std::optional<std::string>& errnote = std::nullopt,
        bool fatal = true
    );

    nlohmann::json _download_json(
        const std::string& url,
        const std::string& video_id,
        const std::optional<std::string>& note = std::nullopt,
        const std::optional<std::string>& errnote = std::nullopt,
        bool fatal = true,
        const std::map<std::string, std::string>& query = {}
    );

    std::string _search_regex(
        const std::string& pattern,
        std::string_view string,
        const std::string& name,
        const std::optional<std::string>& default_value = std::nullopt,
        bool fatal = true,
        std::regex_constants::syntax_option_type flags = std::regex::ECMAScript,
        int group = 1
    ) const;

    nlohmann::json _search_json(
        const std::string& pattern,
        std::string_view string,
        const std::string& name,
        const std::string& video_id,
        const std::optional<nlohmann::json>& default_value = std::nullopt,
        bool fatal = true
    ) const;

    std::string _og_search_property(
        const std::string& prop,
        std::string_view html,
        const std::optional<std::string>& name = std::nullopt,
        const std::optional<std::string>& default_value = std::nullopt,
        bool fatal = true
    ) const;

    std::string _html_search_meta(
        const std::vector<std::string>& names,
        std::string_view html,
        const std::optional<std::string>& display_name = std::nullopt,
        const std::optional<std::string>& default_value = std::nullopt,
        bool fatal = true
    ) const;

    std::map<std::string, std::string> _form_hidden_inputs(
        const std::string& form_id,
        std::string_view html
    ) const;

    // Logging helpers
    void to_screen(const std::string& message) const;
    void report_warning(const std::string& message, const std::string& video_id = "") const;

    // Match valid URL helper
    std::smatch _match_valid_url(const std::string& url) const;

protected:
    core::YoutubeDL* downloader_;
};

} // namespace ytdlp::extractor

#endif // YTDLP_EXTRACTOR_INFO_EXTRACTOR_HPP
