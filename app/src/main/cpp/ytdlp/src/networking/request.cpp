#include "ytdlp/networking/request.hpp"
#include "ytdlp/utils/network_utils.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <algorithm>
#include <cctype>

namespace ytdlp::networking {

Request::Request(
    std::string_view url,
    std::string_view method,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& proxies,
    const std::map<std::string, std::string>& query
) : proxies_(proxies) {
    // Set URL (will be normalized)
    std::string url_str(url);
    if (!query.empty()) {
        url_str = ytdlp::utils::update_url_query(url_str, query);
    }
    set_url(url_str);

    // Set method if provided
    if (!method.empty()) {
        set_method(method);
    }

    // Set headers
    if (!headers.empty()) {
        headers_.update(headers);
    }
}

void Request::set_url(std::string_view url) {
    std::string url_str(url);

    // Handle protocol-relative URLs
    if (ytdlp::utils::starts_with(url_str, "//")) {
        url_str = "http:" + url_str;
    }

    // Normalize the URL
    url_ = ytdlp::utils::normalize_url(url_str);
}

void Request::update_url_query(const std::map<std::string, std::string>& query) {
    if (!query.empty()) {
        url_ = ytdlp::utils::update_url_query(url_, query);
    }
}

std::string Request::method() const {
    if (!explicit_method_.empty()) {
        return explicit_method_;
    }

    // Auto-determine: POST if data present, else GET
    return has_data() ? "POST" : "GET";
}

void Request::set_method(std::string_view method) {
    // Convert to uppercase
    std::string upper_method(method);
    std::transform(upper_method.begin(), upper_method.end(), upper_method.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    explicit_method_ = upper_method;
}

void Request::set_header(std::string_view key, std::string_view value) {
    headers_.set(key, value);
}

void Request::set_data(const std::vector<uint8_t>& data) {
    // Remove Content-Length if data is changing
    if (has_data()) {
        headers_.remove("Content-Length");
    }

    data_ = data;

    // Set Content-Type if not already set
    if (!headers_.contains("Content-Type")) {
        headers_.set("Content-Type", "application/x-www-form-urlencoded");
    }
}

void Request::set_data(std::shared_ptr<std::istream> stream) {
    // Remove Content-Length if data is changing
    if (has_data()) {
        headers_.remove("Content-Length");
    }

    data_ = stream;

    // Set Content-Type if not already set
    if (!headers_.contains("Content-Type")) {
        headers_.set("Content-Type", "application/x-www-form-urlencoded");
    }
}

void Request::clear_data() {
    data_ = std::monostate{};
    headers_.remove("Content-Length");
    headers_.remove("Content-Type");
}

bool Request::has_data() const {
    return !std::holds_alternative<std::monostate>(data_);
}

void Request::set_proxies(const std::map<std::string, std::string>& proxies) {
    proxies_ = proxies;
}

void Request::set_proxy(std::string_view scheme, std::string_view proxy_url) {
    proxies_[std::string(scheme)] = std::string(proxy_url);
}

void Request::set_extension(std::string_view key, const std::any& value) {
    extensions_[std::string(key)] = value;
}

const std::any* Request::get_extension(std::string_view key) const {
    auto it = extensions_.find(std::string(key));
    if (it != extensions_.end()) {
        return &it->second;
    }
    return nullptr;
}

void Request::update(
    std::string_view url,
    const RequestData* data,
    const std::map<std::string, std::string>& headers,
    const std::map<std::string, std::string>& query,
    const std::map<std::string, std::any>& extensions
) {
    // Update data if provided
    if (data != nullptr) {
        data_ = *data;
    }

    // Update headers
    if (!headers.empty()) {
        headers_.update(headers);
    }

    // Update extensions
    for (const auto& [key, value] : extensions) {
        extensions_[key] = value;
    }

    // Update URL (with query if provided)
    if (!url.empty()) {
        std::string new_url(url);
        if (!query.empty()) {
            new_url = ytdlp::utils::update_url_query(new_url, query);
        }
        set_url(new_url);
    } else if (!query.empty()) {
        // Just update query on existing URL
        url_ = ytdlp::utils::update_url_query(url_, query);
    }
}

Request Request::copy() const {
    Request req(url_, explicit_method_, headers_.sensitive(), proxies_);
    req.data_ = data_;
    req.extensions_ = extensions_;
    return req;
}

} // namespace ytdlp::networking
