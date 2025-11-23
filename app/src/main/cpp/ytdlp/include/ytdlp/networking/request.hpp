#ifndef YTDLP_NETWORKING_REQUEST_HPP
#define YTDLP_NETWORKING_REQUEST_HPP

#include "ytdlp/networking/http_header_dict.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <istream>
#include <any>
#include <cstdint>

namespace ytdlp::networking {

/**
 * HTTP Request
 *
 * Represents an HTTP request with URL, method, headers, body data, and extensions.
 * Supports both in-memory data (std::vector<uint8_t>) and streaming data (std::istream).
 *
 * Features:
 * - Automatic method detection (POST if data present, GET otherwise)
 * - URL normalization and query parameter handling
 * - Header management via HTTPHeaderDict
 * - Proxy configuration
 * - Extension mechanism for custom metadata
 * - Request cloning
 *
 * Usage:
 *   Request req("https://example.com/api", "POST");
 *   req.set_header("Content-Type", "application/json");
 *   req.set_data(json_bytes);
 *   auto response = client.execute(req);
 */
class Request {
public:
    // Request data type: empty (no data), bytes, or stream
    using RequestData = std::variant<
        std::monostate,                    // No data
        std::vector<uint8_t>,              // In-memory data
        std::shared_ptr<std::istream>      // Streaming data
    >;

    // Constructors
    Request() = default;
    explicit Request(
        std::string_view url,
        std::string_view method = "",
        const std::map<std::string, std::string>& headers = {},
        const std::map<std::string, std::string>& proxies = {},
        const std::map<std::string, std::string>& query = {}
    );

    // URL access
    const std::string& url() const { return url_; }
    void set_url(std::string_view url);
    void update_url_query(const std::map<std::string, std::string>& query);

    // Method access (auto-detects if not explicitly set)
    std::string method() const;
    void set_method(std::string_view method);

    // Header access
    const HTTPHeaderDict& headers() const { return headers_; }
    HTTPHeaderDict& headers() { return headers_; }
    void set_header(std::string_view key, std::string_view value);

    // Request body
    const RequestData& data() const { return data_; }
    void set_data(const std::vector<uint8_t>& data);
    void set_data(std::shared_ptr<std::istream> stream);
    void clear_data();
    bool has_data() const;

    // Proxy configuration
    const std::map<std::string, std::string>& proxies() const { return proxies_; }
    void set_proxies(const std::map<std::string, std::string>& proxies);
    void set_proxy(std::string_view scheme, std::string_view proxy_url);

    // Extensions (custom metadata)
    void set_extension(std::string_view key, const std::any& value);
    const std::any* get_extension(std::string_view key) const;

    // Bulk update
    void update(
        std::string_view url = "",
        const RequestData* data = nullptr,
        const std::map<std::string, std::string>& headers = {},
        const std::map<std::string, std::string>& query = {},
        const std::map<std::string, std::any>& extensions = {}
    );

    // Clone request
    Request copy() const;

private:
    std::string url_;
    std::string explicit_method_;  // Empty means auto-detect
    HTTPHeaderDict headers_;
    RequestData data_;
    std::map<std::string, std::string> proxies_;
    std::map<std::string, std::any> extensions_;
};

} // namespace ytdlp::networking

#endif // YTDLP_NETWORKING_REQUEST_HPP
