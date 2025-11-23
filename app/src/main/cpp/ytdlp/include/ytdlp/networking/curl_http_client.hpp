#ifndef YTDLP_NETWORKING_CURL_HTTP_CLIENT_HPP
#define YTDLP_NETWORKING_CURL_HTTP_CLIENT_HPP

#include "ytdlp/networking/request.hpp"
#include "ytdlp/networking/response.hpp"
#include "ytdlp/networking/cookie_jar.hpp"
#include <curl/curl.h>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <cstdint>

namespace ytdlp::networking {

/**
 * CURL-based HTTP Client
 *
 * A complete HTTP/HTTPS client built on libcurl with support for:
 * - All standard HTTP methods (GET, POST, PUT, PATCH, DELETE, HEAD)
 * - Cookie management via CookieJar
 * - Proxy support (HTTP, HTTPS, SOCKS)
 * - SSL/TLS with certificate verification
 * - Automatic redirect following
 * - Compression (gzip, deflate, brotli)
 * - Connection timeouts and stall detection
 * - Streaming downloads to file with progress callbacks
 * - Custom user agents
 *
 * The client reuses a single CURL handle for efficiency and supports
 * both in-memory and streaming requests/responses.
 *
 * Usage:
 *   CurlHttpClient::Config config;
 *   config.user_agent = "MyApp/1.0";
 *   config.timeout = 30;
 *
 *   CurlHttpClient client(config);
 *   auto resp = client.get("https://example.com");
 */
class CurlHttpClient {
public:
    /**
     * Client Configuration
     */
    struct Config {
        // Timeouts (in seconds)
        long connect_timeout = 10;     // Connection timeout
        long timeout = 300;             // Total request timeout

        // Stall detection
        long low_speed_limit = 1024;    // Bytes/sec minimum speed
        long low_speed_time = 10;       // Abort if < low_speed_limit for this many seconds

        // Redirects
        bool follow_redirects = true;
        long max_redirects = 10;

        // SSL/TLS
        bool verify_ssl = true;
        std::string ca_cert_path;       // Path to CA certificate bundle

        // User agent
        std::string user_agent = "ytdlp-cpp/1.0";

        // Debug
        bool verbose = false;
    };

    // Constructors
    CurlHttpClient();
    explicit CurlHttpClient(const Config& config);
    ~CurlHttpClient();

    // Move-only (CURL handle is not copyable)
    CurlHttpClient(const CurlHttpClient&) = delete;
    CurlHttpClient& operator=(const CurlHttpClient&) = delete;
    CurlHttpClient(CurlHttpClient&&) noexcept;
    CurlHttpClient& operator=(CurlHttpClient&&) noexcept;

    // Cookie jar management
    void set_cookie_jar(std::shared_ptr<CookieJar> jar);
    std::shared_ptr<CookieJar> cookie_jar() const { return cookie_jar_; }

    // Generic request execution
    Response execute(const Request& request);

    // Convenience methods
    Response get(
        std::string_view url,
        const std::map<std::string, std::string>& headers = {}
    );

    Response post(
        std::string_view url,
        const std::vector<uint8_t>& data,
        const std::map<std::string, std::string>& headers = {}
    );

    Response head(
        std::string_view url,
        const std::map<std::string, std::string>& headers = {}
    );

    Response put(
        std::string_view url,
        const std::vector<uint8_t>& data,
        const std::map<std::string, std::string>& headers = {}
    );

    Response patch(
        std::string_view url,
        const std::vector<uint8_t>& data,
        const std::map<std::string, std::string>& headers = {}
    );

    Response del(
        std::string_view url,
        const std::map<std::string, std::string>& headers = {}
    );

    // Streaming download to file
    bool download_to_file(
        const std::string& url,
        const std::string& output_path,
        const std::map<std::string, std::string>& headers = {},
        std::function<void(int64_t downloaded, int64_t total)> progress_callback = nullptr
    );

    // Configuration access
    const Config& config() const { return config_; }
    void set_config(const Config& config) { config_ = config; }

private:
    // RAII wrappers for CURL resources
    struct CurlHandle {
        CURL* handle = nullptr;

        CurlHandle();
        ~CurlHandle();

        // Move-only
        CurlHandle(const CurlHandle&) = delete;
        CurlHandle& operator=(const CurlHandle&) = delete;
        CurlHandle(CurlHandle&& other) noexcept;
        CurlHandle& operator=(CurlHandle&& other) noexcept;

        CURL* get() const { return handle; }
        void reset();  // curl_easy_reset
    };

    struct CurlHeaders {
        curl_slist* list = nullptr;

        ~CurlHeaders();

        curl_slist* get() const { return list; }
        void append(const std::string& header);
    };

    // Response data accumulator
    struct ResponseData {
        std::vector<uint8_t> body;
        std::map<std::string, std::string> headers;
        std::string effective_url;
        long response_code = 0;
    };

    // Internal configuration methods
    void configure_request(
        CURL* curl,
        const Request& request,
        ResponseData& response_data,
        CurlHeaders& headers
    );

    void set_curl_options(CURL* curl);
    void set_method(CURL* curl, const std::string& method);
    void set_proxy(CURL* curl, const Request& request);

    // CURL callbacks
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t header_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t file_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                  curl_off_t ultotal, curl_off_t ulnow);

    // Member variables
    CurlHandle curl_;
    Config config_;
    std::shared_ptr<CookieJar> cookie_jar_;
};

} // namespace ytdlp::networking

#endif // YTDLP_NETWORKING_CURL_HTTP_CLIENT_HPP
