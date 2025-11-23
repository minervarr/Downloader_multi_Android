#ifndef YTDLP_NETWORKING_RESPONSE_HPP
#define YTDLP_NETWORKING_RESPONSE_HPP

#include "ytdlp/networking/http_header_dict.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <memory>
#include <istream>
#include <any>
#include <cstdint>

namespace ytdlp::networking {

/**
 * HTTP Response
 *
 * Represents an HTTP response with status code, headers, and streaming body.
 * The response body is accessed via std::istream for memory-efficient processing
 * of large responses.
 *
 * Features:
 * - Streaming body access (std::istream)
 * - HTTP status code and reason phrase
 * - Response headers via HTTPHeaderDict
 * - Effective URL (after redirects)
 * - Extension mechanism for custom metadata
 * - Multiple read methods (bytes, string, buffered)
 *
 * Usage:
 *   Response resp = client.get("https://example.com");
 *   if (resp.status() == 200) {
 *       std::string body = resp.read_all();
 *   }
 */
class Response {
public:
    // Constructors
    Response() = default;
    explicit Response(
        std::shared_ptr<std::istream> body,
        std::string_view url = "",
        const std::map<std::string, std::string>& headers = {},
        int status = 200,
        std::string_view reason = "",
        const std::map<std::string, std::any>& extensions = {}
    );

    // Status information
    int status() const { return status_; }
    const std::string& reason() const { return reason_; }
    const std::string& url() const { return url_; }  // Effective URL after redirects
    bool is_success() const;  // Returns true if status is 2xx

    // Header access
    const HTTPHeaderDict& headers() const { return headers_; }
    std::string get_header(std::string_view name, std::string_view default_value = "") const;
    std::vector<std::string> get_header_all(std::string_view name) const;

    // Body reading
    bool readable() const;
    size_t read(char* buffer, size_t size);
    std::vector<uint8_t> read_bytes(size_t size = 0);  // 0 means read all
    std::string read_all();
    void close();

    // Extensions (custom metadata)
    const std::any* get_extension(std::string_view key) const;

    // Direct stream access (advanced usage)
    std::shared_ptr<std::istream> body_stream() const { return body_; }

    // Helper: Get default reason phrase for HTTP status code
    static std::string default_reason_for_status(int status);

private:
    std::shared_ptr<std::istream> body_;
    std::string url_;
    HTTPHeaderDict headers_;
    int status_ = 200;
    std::string reason_;
    std::map<std::string, std::any> extensions_;
};

} // namespace ytdlp::networking

#endif // YTDLP_NETWORKING_RESPONSE_HPP
