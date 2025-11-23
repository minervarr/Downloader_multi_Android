#include "ytdlp/networking/http_header_dict.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace ytdlp::networking {

std::string HTTPHeaderDict::to_title_case(std::string_view key) {
    std::string result;
    result.reserve(key.size());

    bool capitalize_next = true;
    for (char c : key) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            if (capitalize_next) {
                result += std::toupper(static_cast<unsigned char>(c));
                capitalize_next = false;
            } else {
                result += std::tolower(static_cast<unsigned char>(c));
            }
        } else {
            result += c;
            // Capitalize after hyphen or space
            if (c == '-' || c == ' ') {
                capitalize_next = true;
            }
        }
    }

    return result;
}

HTTPHeaderDict::HTTPHeaderDict(const std::map<std::string, std::string>& headers) {
    update(headers);
}

HTTPHeaderDict::HTTPHeaderDict(const std::vector<std::map<std::string, std::string>>& header_maps) {
    for (const auto& headers : header_maps) {
        update(headers);
    }
}

void HTTPHeaderDict::set(std::string_view key, std::string_view value) {
    std::string title_key = to_title_case(key);

    // Store the original casing
    sensitive_map_[title_key] = std::string(key);

    // Store the value (trim whitespace)
    std::string val_str(value);
    // Trim leading whitespace
    val_str.erase(0, val_str.find_first_not_of(" \t\r\n"));
    // Trim trailing whitespace
    val_str.erase(val_str.find_last_not_of(" \t\r\n") + 1);

    headers_[title_key] = val_str;
}

std::optional<std::string> HTTPHeaderDict::get(std::string_view key) const {
    std::string title_key = to_title_case(key);
    auto it = headers_.find(title_key);
    if (it != headers_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string HTTPHeaderDict::get(std::string_view key, std::string_view default_value) const {
    auto result = get(key);
    return result.value_or(std::string(default_value));
}

bool HTTPHeaderDict::contains(std::string_view key) const {
    std::string title_key = to_title_case(key);
    return headers_.find(title_key) != headers_.end();
}

bool HTTPHeaderDict::remove(std::string_view key) {
    std::string title_key = to_title_case(key);
    sensitive_map_.erase(title_key);
    return headers_.erase(title_key) > 0;
}

std::optional<std::string> HTTPHeaderDict::pop(std::string_view key) {
    std::string title_key = to_title_case(key);
    auto it = headers_.find(title_key);
    if (it != headers_.end()) {
        std::string value = it->second;
        headers_.erase(it);
        sensitive_map_.erase(title_key);
        return value;
    }
    return std::nullopt;
}

std::string HTTPHeaderDict::pop(std::string_view key, std::string_view default_value) {
    auto result = pop(key);
    return result.value_or(std::string(default_value));
}

std::string HTTPHeaderDict::setdefault(std::string_view key, std::string_view value) {
    std::string title_key = to_title_case(key);
    auto it = headers_.find(title_key);
    if (it != headers_.end()) {
        return it->second;
    }

    // Not found, set it
    set(key, value);
    return std::string(value);
}

void HTTPHeaderDict::update(const std::map<std::string, std::string>& other) {
    for (const auto& [key, value] : other) {
        set(key, value);
    }
}

void HTTPHeaderDict::update(const HTTPHeaderDict& other) {
    // Use sensitive() to preserve original casing from the other dict
    auto other_sensitive = other.sensitive();
    for (const auto& [key, value] : other_sensitive) {
        set(key, value);
    }
}

void HTTPHeaderDict::clear() {
    headers_.clear();
    sensitive_map_.clear();
}

size_t HTTPHeaderDict::size() const {
    return headers_.size();
}

bool HTTPHeaderDict::empty() const {
    return headers_.empty();
}

std::map<std::string, std::string> HTTPHeaderDict::to_map() const {
    return headers_;
}

std::map<std::string, std::string> HTTPHeaderDict::sensitive() const {
    std::map<std::string, std::string> result;
    for (const auto& [title_key, value] : headers_) {
        auto it = sensitive_map_.find(title_key);
        if (it != sensitive_map_.end()) {
            result[it->second] = value;
        } else {
            // Fallback to title-case if no sensitive mapping
            result[title_key] = value;
        }
    }
    return result;
}

std::string HTTPHeaderDict::operator[](std::string_view key) const {
    return get(key, "");
}

HTTPHeaderDict HTTPHeaderDict::operator|(const HTTPHeaderDict& other) const {
    HTTPHeaderDict result(*this);
    result.update(other);
    return result;
}

HTTPHeaderDict HTTPHeaderDict::operator|(const std::map<std::string, std::string>& other) const {
    HTTPHeaderDict result(*this);
    result.update(other);
    return result;
}

} // namespace ytdlp::networking
