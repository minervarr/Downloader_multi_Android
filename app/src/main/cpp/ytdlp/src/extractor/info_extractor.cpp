#include "ytdlp/extractor/info_extractor.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include "ytdlp/networking/request.hpp"
#include "ytdlp/networking/response.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include "ytdlp/utils/json_utils.hpp"
#include <stdexcept>
#include <iostream>

namespace ytdlp::extractor {

InfoExtractor::InfoExtractor(core::YoutubeDL* downloader)
    : downloader_(downloader) {
}

std::string InfoExtractor::ie_key() const {
    return "InfoExtractor";
}

std::string InfoExtractor::ie_name() const {
    return "Generic";
}

core::InfoDict InfoExtractor::extract(const std::string& url) {
    try {
        return _real_extract(url);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to extract from " + url + ": " + e.what());
    }
}

void InfoExtractor::set_downloader(core::YoutubeDL* downloader) {
    downloader_ = downloader;
}

void InfoExtractor::to_screen(const std::string& msg) const {
    if (downloader_) {
        downloader_->to_stdout(msg);
    }
}

void InfoExtractor::report_warning(const std::string& msg, const std::string& video_id) const {
    if (downloader_) {
        downloader_->report_warning(msg);
    }
}

std::string InfoExtractor::_download_webpage(
    const std::string& url_or_request,
    const std::string& video_id,
    const std::optional<std::string>& note,
    const std::optional<std::string>& errnote,
    bool fatal
) {
    if (!downloader_) {
        throw std::runtime_error("No downloader set for InfoExtractor");
    }

    if (note.has_value()) {
        to_screen("[" + ie_key() + "] " + video_id + ": " + note.value());
    }

    try {
        // Get HTTP client from downloader
        auto& http_client = downloader_->http_client();

        // Create GET request
        networking::Request request(url_or_request);

        // Execute request
        auto response = http_client.execute(request);

        // Check status code
        if (!response.is_success()) {
            std::string error_msg = "HTTP Error " + std::to_string(response.status());
            if (errnote.has_value()) {
                error_msg = errnote.value() + ": " + error_msg;
            }

            if (fatal) {
                throw std::runtime_error(error_msg);
            }

            report_warning(error_msg, video_id);
            return "";
        }

        return response.read_all();

    } catch (const std::exception& e) {
        std::string error_msg = std::string(e.what());
        if (errnote.has_value()) {
            error_msg = errnote.value() + ": " + error_msg;
        }

        if (fatal) {
            throw std::runtime_error(error_msg);
        }

        report_warning(error_msg, video_id);
        return "";
    }
}

std::string InfoExtractor::_search_regex(
    const std::string& pattern,
    std::string_view string,
    const std::string& name,
    const std::optional<std::string>& default_value,
    bool fatal,
    std::regex_constants::syntax_option_type flags,
    int group
) const {
    try {
        std::regex re(pattern, flags);
        std::match_results<std::string_view::const_iterator> match;

        std::string str(string);  // Convert string_view to string for regex_search
        std::smatch smatch;

        if (std::regex_search(str, smatch, re)) {
            if (group >= 0 && static_cast<size_t>(group) < smatch.size()) {
                return smatch[group].str();
            }
        }

        // Not found
        if (default_value.has_value()) {
            return default_value.value();
        }

        if (fatal) {
            throw std::runtime_error("Unable to extract " + name);
        }

        return "";

    } catch (const std::regex_error& e) {
        throw std::runtime_error("Invalid regex pattern for " + name + ": " + e.what());
    }
}

std::string InfoExtractor::_og_search_property(
    const std::string& prop,
    std::string_view html,
    const std::optional<std::string>& name,
    const std::optional<std::string>& default_value,
    bool fatal
) const {
    // Search for Open Graph meta tags: <meta property="og:PROP" content="VALUE">
    std::string pattern = R"(<meta[^>]+property=["\']og:)" + prop + R"(["\'][^>]+content=["\']([^"\']+)["\'])";

    // Try property attribute first
    std::string result = _search_regex(
        pattern,
        html,
        name.value_or("og:" + prop),
        std::nullopt,
        false
    );

    if (!result.empty()) {
        return utils::unescape_html(result);
    }

    // Try content then property order
    pattern = R"(<meta[^>]+content=["\']([^"\']+)["\'][^>]+property=["\']og:)" + prop + R"(["\'])";
    result = _search_regex(
        pattern,
        html,
        name.value_or("og:" + prop),
        default_value,
        fatal
    );

    if (!result.empty()) {
        return utils::unescape_html(result);
    }

    return default_value.value_or("");
}

std::string InfoExtractor::_html_search_meta(
    const std::vector<std::string>& names,
    std::string_view html,
    const std::optional<std::string>& display_name,
    const std::optional<std::string>& default_value,
    bool fatal
) const {
    // Search for <meta name="NAME" content="VALUE"> or <meta property="NAME" content="VALUE">
    for (const auto& name : names) {
        // Try name attribute
        std::string pattern = R"(<meta[^>]+name=["\'])" + name + R"(["\'][^>]+content=["\']([^"\']+)["\'])";
        std::string result = _search_regex(pattern, html, name, std::nullopt, false);

        if (!result.empty()) {
            return utils::unescape_html(result);
        }

        // Try content then name order
        pattern = R"(<meta[^>]+content=["\']([^"\']+)["\'][^>]+name=["\'])" + name + R"(["\'])";
        result = _search_regex(pattern, html, name, std::nullopt, false);

        if (!result.empty()) {
            return utils::unescape_html(result);
        }

        // Try property attribute
        pattern = R"(<meta[^>]+property=["\'])" + name + R"(["\'][^>]+content=["\']([^"\']+)["\'])";
        result = _search_regex(pattern, html, name, std::nullopt, false);

        if (!result.empty()) {
            return utils::unescape_html(result);
        }
    }

    // Not found in any name
    if (default_value.has_value()) {
        return default_value.value();
    }

    if (fatal) {
        std::string name_list = names.empty() ? "" : names[0];
        throw std::runtime_error("Unable to extract " + display_name.value_or(name_list));
    }

    return "";
}

nlohmann::json InfoExtractor::_download_json(
    const std::string& url,
    const std::string& video_id,
    const std::optional<std::string>& note,
    const std::optional<std::string>& errnote,
    bool fatal,
    const std::map<std::string, std::string>& query
) {
    // Build URL with query parameters
    std::string full_url = url;
    if (!query.empty()) {
        std::string query_str;
        for (const auto& [key, value] : query) {
            if (!query_str.empty()) query_str += "&";
            query_str += utils::url_encode(key) + "=" + utils::url_encode(value);
        }
        full_url += (url.find('?') == std::string::npos ? "?" : "&") + query_str;
    }

    // Download webpage
    std::string json_text = _download_webpage(full_url, video_id, note, errnote, fatal);

    if (json_text.empty()) {
        return nlohmann::json();
    }

    // Parse JSON
    try {
        return nlohmann::json::parse(json_text);
    } catch (const nlohmann::json::exception& e) {
        std::string error_msg = "Failed to parse JSON for " + video_id + ": " + std::string(e.what());
        if (fatal) {
            throw std::runtime_error(error_msg);
        }
        report_warning(error_msg, video_id);
        return nlohmann::json();
    }
}

std::map<std::string, std::string> InfoExtractor::_form_hidden_inputs(
    const std::string& form_id,
    std::string_view html
) const {
    std::map<std::string, std::string> inputs;

    // Find the form
    std::string form_pattern = R"(<form[^>]+id\s*=\s*["\'])" + form_id + R"(["\'][^>]*>([\s\S]*?)</form>)";
    std::string form_content = _search_regex(form_pattern, html, "form " + form_id, std::nullopt, false);

    if (form_content.empty()) {
        throw std::runtime_error("Form not found: " + form_id);
    }

    // Extract all input fields
    std::regex input_pattern(R"(<input[^>]+type\s*=\s*["\']hidden["\'][^>]*>)", std::regex::icase);
    std::string content_str(form_content);

    auto begin = std::sregex_iterator(content_str.begin(), content_str.end(), input_pattern);
    auto end = std::sregex_iterator();

    for (std::sregex_iterator i = begin; i != end; ++i) {
        std::string input_tag = (*i).str();

        // Extract name attribute
        std::regex name_re(R"(name\s*=\s*["\']([^"\']+)["\'])", std::regex::icase);
        std::smatch name_match;
        if (!std::regex_search(input_tag, name_match, name_re) || name_match.size() < 2) {
            continue;
        }
        std::string name = name_match[1].str();

        // Extract value attribute
        std::regex value_re(R"(value\s*=\s*["\']([^"\']*)["\'])", std::regex::icase);
        std::smatch value_match;
        std::string value;
        if (std::regex_search(input_tag, value_match, value_re) && value_match.size() >= 2) {
            value = value_match[1].str();
        }

        inputs[name] = value;
    }

    // Also check for non-hidden inputs and other form fields
    std::regex all_input_pattern(R"(<input[^>]+name\s*=\s*["\']([^"\']+)["\'][^>]*>)", std::regex::icase);
    auto begin2 = std::sregex_iterator(content_str.begin(), content_str.end(), all_input_pattern);

    for (std::sregex_iterator i = begin2; i != end; ++i) {
        std::string input_tag = (*i).str();
        std::smatch match = *i;

        if (match.size() < 2) continue;
        std::string name = match[1].str();

        // Skip if already added
        if (inputs.find(name) != inputs.end()) {
            continue;
        }

        // Extract value
        std::regex value_re(R"(value\s*=\s*["\']([^"\']*)["\'])", std::regex::icase);
        std::smatch value_match;
        std::string value;
        if (std::regex_search(input_tag, value_match, value_re) && value_match.size() >= 2) {
            value = value_match[1].str();
        }

        inputs[name] = value;
    }

    return inputs;
}

std::smatch InfoExtractor::_match_valid_url(const std::string& url) const {
    // This is a virtual method that should be overridden by subclasses
    // For the base class, we just return an empty match
    std::smatch match;
    return match;
}

} // namespace ytdlp::extractor
