/**
 * String Utilities Implementation
 */

#include "ytdlp/utils/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

#include <fmt/core.h>

namespace ytdlp::utils {

// ============================================================================
// URL Encoding/Decoding
// ============================================================================

std::string url_encode(std::string_view str, std::string_view safe) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : str) {
        // Keep alphanumeric and other safe characters intact
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ||
            safe.find(c) != std::string_view::npos) {
            escaped << c;
        } else {
            // Percent-encode everything else
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int(c);
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

std::string url_decode(std::string_view str) {
    std::string result;
    result.reserve(str.length());

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            // Decode percent-encoded character
            std::string hex_str(str.substr(i + 1, 2));
            char* end_ptr;
            long value = std::strtol(hex_str.c_str(), &end_ptr, 16);
            if (end_ptr == hex_str.c_str() + 2) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            // Plus sign = space in query strings
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

std::string urlencode_postdata(const std::map<std::string, std::string>& params) {
    std::string result;
    bool first = true;

    for (const auto& [key, value] : params) {
        if (!first) {
            result += '&';
        }
        first = false;
        result += url_encode(key);
        result += '=';
        result += url_encode(value);
    }

    return result;
}

// ============================================================================
// HTML Escaping/Unescaping
// ============================================================================

std::string escape_html(std::string_view text) {
    std::string result;
    result.reserve(text.length() * 1.1); // Reserve a bit extra

    for (char c : text) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default:   result += c; break;
        }
    }

    return result;
}

std::string unescape_html(std::string_view html) {
    std::string result;
    result.reserve(html.length());

    static const std::map<std::string, std::string> html_entities = {
        {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""},
        {"apos", "'"}, {"nbsp", " "}, {"copy", "©"}, {"reg", "®"},
        // Add more as needed
    };

    for (size_t i = 0; i < html.length(); ++i) {
        if (html[i] == '&') {
            size_t end = html.find(';', i);
            if (end != std::string::npos) {
                std::string entity(html.substr(i + 1, end - i - 1));

                // Numeric character reference (&#NNN; or &#xHHH;)
                if (!entity.empty() && entity[0] == '#') {
                    try {
                        int code;
                        if (entity.length() > 1 && (entity[1] == 'x' || entity[1] == 'X')) {
                            // Hexadecimal
                            code = std::stoi(entity.substr(2), nullptr, 16);
                        } else {
                            // Decimal
                            code = std::stoi(entity.substr(1));
                        }
                        if (code >= 0 && code <= 0x10FFFF) {
                            // Simple ASCII handling for now
                            if (code < 128) {
                                result += static_cast<char>(code);
                            } else {
                                // For non-ASCII, you'd need proper UTF-8 encoding
                                result += '?'; // Placeholder
                            }
                            i = end;
                            continue;
                        }
                    } catch (...) {
                        // Invalid numeric reference, keep as-is
                    }
                }

                // Named entity
                auto it = html_entities.find(entity);
                if (it != html_entities.end()) {
                    result += it->second;
                    i = end;
                    continue;
                }
            }
        }
        result += html[i];
    }

    return result;
}

// ============================================================================
// Filename Sanitization
// ============================================================================

std::string sanitize_filename(
    std::string_view filename,
    bool restricted,
    std::optional<bool> is_id
) {
    if (filename.empty()) {
        return "";
    }

    std::string result;
    result.reserve(filename.length());

    auto replace_insane = [&](char ch) -> std::string {
        // Control characters and DEL
        if (ch < 32 || ch == 127 || ch == '?') {
            return "";
        }

        // Quote handling
        if (ch == '"') {
            return restricted ? "" : "'";
        }

        // Colon handling (problematic on Windows)
        if (ch == ':') {
            return restricted ? "_-" : " -";
        }

        // Path separators and wildcards
        if (ch == '\\' || ch == '/' || ch == '|' || ch == '*' || ch == '<' || ch == '>') {
            return "_";
        }

        // Restricted mode: stricter character set
        if (restricted && (ch == '!' || ch == '&' || ch == '\'' || ch == '(' || ch == ')' ||
                           ch == '[' || ch == ']' || ch == '{' || ch == '}' || ch == '$' ||
                           ch == ';' || ch == '`' || ch == '^' || ch == ',' || ch == '#' ||
                           std::isspace(ch) || static_cast<unsigned char>(ch) > 127)) {
            return "_";
        }

        return std::string(1, ch);
    };

    // Apply sanitization
    for (char ch : filename) {
        result += replace_insane(ch);
    }

    // Remove repeated underscores
    if (!is_id.value_or(false)) {
        std::string temp;
        bool last_was_underscore = false;
        for (char ch : result) {
            if (ch == '_') {
                if (!last_was_underscore) {
                    temp += ch;
                }
                last_was_underscore = true;
            } else {
                temp += ch;
                last_was_underscore = false;
            }
        }
        result = temp;

        // Strip underscores from edges
        while (!result.empty() && result.front() == '_') {
            result.erase(result.begin());
        }
        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }

        // Strip leading dots (hidden files on Unix)
        while (!result.empty() && result.front() == '.') {
            result.erase(result.begin());
        }
    }

    // Ensure we have something
    if (result.empty()) {
        result = "_";
    }

    return result;
}

std::string sanitize_path(std::string_view path, bool force) {
#ifndef _WIN32
    if (!force) {
        return std::string(path);
    }
#endif

    // Simple implementation for now - can be enhanced
    std::string result(path);

    // Replace invalid characters
    for (char& ch : result) {
        if (ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '|' || ch == '?' || ch == '*') {
            ch = '#';
        }
    }

    // Handle path separators consistently
    std::replace(result.begin(), result.end(), '\\', '/');

    return result;
}

// ============================================================================
// URL Sanitization
// ============================================================================

std::string sanitize_url(std::string_view url, std::string_view scheme) {
    std::string result(url);

    // Add scheme if missing
    if (result.find("://") == std::string::npos) {
        result = fmt::format("{}://{}", scheme, result);
    }

    return result;
}

std::tuple<std::string, std::string, std::string> extract_basic_auth(std::string_view url) {
    // Find ://
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return {std::string(url), "", ""};
    }

    size_t auth_start = scheme_end + 3;
    size_t at_pos = url.find('@', auth_start);

    if (at_pos == std::string::npos) {
        // No auth
        return {std::string(url), "", ""};
    }

    // Extract auth part
    std::string auth_part(url.substr(auth_start, at_pos - auth_start));
    size_t colon_pos = auth_part.find(':');

    std::string username, password;
    if (colon_pos != std::string::npos) {
        username = auth_part.substr(0, colon_pos);
        password = auth_part.substr(colon_pos + 1);
    } else {
        username = auth_part;
    }

    // Build URL without auth
    std::string url_without_auth = fmt::format("{}://{}",
        url.substr(0, scheme_end),
        url.substr(at_pos + 1));

    return {url_without_auth, username, password};
}

// ============================================================================
// String Manipulation
// ============================================================================

std::string strip(std::string_view str) {
    size_t start = 0;
    size_t end = str.length();

    while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return std::string(str.substr(start, end - start));
}

std::string strip(std::string_view str, char ch) {
    size_t start = 0;
    size_t end = str.length();

    while (start < end && str[start] == ch) {
        ++start;
    }

    while (end > start && str[end - 1] == ch) {
        --end;
    }

    return std::string(str.substr(start, end - start));
}

std::string lstrip(std::string_view str) {
    size_t start = 0;
    while (start < str.length() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    return std::string(str.substr(start));
}

std::string rstrip(std::string_view str) {
    size_t end = str.length();
    while (end > 0 && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    return std::string(str.substr(0, end));
}

std::string to_lower(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string to_upper(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::vector<std::string> split(std::string_view str, char delimiter, int max_split) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;
    int splits = 0;

    while ((end = str.find(delimiter, start)) != std::string::npos) {
        result.emplace_back(str.substr(start, end - start));
        start = end + 1;
        ++splits;
        if (max_split >= 0 && splits >= max_split) {
            break;
        }
    }

    result.emplace_back(str.substr(start));
    return result;
}

std::vector<std::string> split(std::string_view str, std::string_view delimiter, int max_split) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;
    int splits = 0;

    while ((end = str.find(delimiter, start)) != std::string::npos) {
        result.emplace_back(str.substr(start, end - start));
        start = end + delimiter.length();
        ++splits;
        if (max_split >= 0 && splits >= max_split) {
            break;
        }
    }

    result.emplace_back(str.substr(start));
    return result;
}

std::string join(const std::vector<std::string>& strings, std::string_view delimiter) {
    if (strings.empty()) {
        return "";
    }

    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter;
        result += strings[i];
    }

    return result;
}

bool starts_with(std::string_view str, std::string_view prefix) {
    return str.length() >= prefix.length() &&
           str.substr(0, prefix.length()) == prefix;
}

bool ends_with(std::string_view str, std::string_view suffix) {
    return str.length() >= suffix.length() &&
           str.substr(str.length() - suffix.length()) == suffix;
}

std::string replace_all(std::string_view str, std::string_view from, std::string_view to) {
    std::string result(str);
    size_t pos = 0;

    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }

    return result;
}

// ============================================================================
// Format Detection
// ============================================================================

std::string determine_ext(std::string_view url, std::string_view default_ext) {
    // Remove query string and fragment
    size_t query_pos = url.find('?');
    size_t fragment_pos = url.find('#');
    size_t end_pos = std::min(query_pos, fragment_pos);

    std::string_view clean_url = url.substr(0, end_pos);

    // Find last dot
    size_t dot_pos = clean_url.rfind('.');
    if (dot_pos != std::string::npos && dot_pos < clean_url.length() - 1) {
        // Extract extension
        std::string ext(clean_url.substr(dot_pos + 1));

        // Convert to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(),
                      [](unsigned char c) { return std::tolower(c); });

        return ext;
    }

    return std::string(default_ext);
}

// ============================================================================
// Number Parsing
// ============================================================================

std::optional<int> parse_int(std::string_view str) {
    str = strip(str);
    if (str.empty()) {
        return std::nullopt;
    }

    int value;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec == std::errc{} && ptr == str.data() + str.size()) {
        return value;
    }

    return std::nullopt;
}

std::optional<double> parse_float(std::string_view str) {
    str = strip(str);
    if (str.empty()) {
        return std::nullopt;
    }

    try {
        size_t pos;
        double value = std::stod(std::string(str), &pos);
        if (pos == str.length()) {
            return value;
        }
    } catch (...) {
        // Parsing failed
    }

    return std::nullopt;
}

// NOTE: parse_filesize(), parse_duration(), format_bytes(), and format_seconds()
// have been moved to number_utils.cpp

} // namespace ytdlp::utils
