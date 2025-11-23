#include "ytdlp/networking/curl_http_client.hpp"
#include "ytdlp/utils/network_utils.hpp"
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace ytdlp::networking {

// ============================================================================
// CurlHandle RAII wrapper
// ============================================================================

CurlHttpClient::CurlHandle::CurlHandle() {
    handle = curl_easy_init();
    if (!handle) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

CurlHttpClient::CurlHandle::~CurlHandle() {
    if (handle) {
        curl_easy_cleanup(handle);
        handle = nullptr;
    }
}

CurlHttpClient::CurlHandle::CurlHandle(CurlHandle&& other) noexcept
    : handle(other.handle) {
    other.handle = nullptr;
}

CurlHttpClient::CurlHandle& CurlHttpClient::CurlHandle::operator=(CurlHandle&& other) noexcept {
    if (this != &other) {
        if (handle) {
            curl_easy_cleanup(handle);
        }
        handle = other.handle;
        other.handle = nullptr;
    }
    return *this;
}

void CurlHttpClient::CurlHandle::reset() {
    if (handle) {
        curl_easy_reset(handle);
    }
}

// ============================================================================
// CurlHeaders RAII wrapper
// ============================================================================

CurlHttpClient::CurlHeaders::~CurlHeaders() {
    if (list) {
        curl_slist_free_all(list);
        list = nullptr;
    }
}

void CurlHttpClient::CurlHeaders::append(const std::string& header) {
    list = curl_slist_append(list, header.c_str());
}

// ============================================================================
// CurlHttpClient implementation
// ============================================================================

CurlHttpClient::CurlHttpClient()
    : config_() {
}

CurlHttpClient::CurlHttpClient(const Config& config)
    : config_(config) {
}

CurlHttpClient::~CurlHttpClient() = default;

CurlHttpClient::CurlHttpClient(CurlHttpClient&&) noexcept = default;
CurlHttpClient& CurlHttpClient::operator=(CurlHttpClient&&) noexcept = default;

void CurlHttpClient::set_cookie_jar(std::shared_ptr<CookieJar> jar) {
    cookie_jar_ = jar;
}

Response CurlHttpClient::execute(const Request& request) {
    // Reset curl handle for reuse
    curl_.reset();
    CURL* curl = curl_.get();

    // Response data accumulator
    ResponseData response_data;

    // Set up headers
    CurlHeaders curl_headers;
    for (const auto& [key, value] : request.headers().to_map()) {
        curl_headers.append(key + ": " + value);
    }

    // Add cookies from cookie jar if available
    if (cookie_jar_) {
        std::string cookie_header = cookie_jar_->get_cookie_header(request.url());
        if (!cookie_header.empty()) {
            curl_headers.append("Cookie: " + cookie_header);
        }
    }

    // Configure request
    configure_request(curl, request, response_data, curl_headers);

    // Execute request
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }

    // Get response code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_data.response_code);

    // Get effective URL (after redirects)
    char* effective_url = nullptr;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
    if (effective_url) {
        response_data.effective_url = effective_url;
    } else {
        response_data.effective_url = request.url();
    }

    // Create response body stream
    auto body_stream = std::make_shared<std::istringstream>(
        std::string(response_data.body.begin(), response_data.body.end())
    );

    // Create and return Response
    return Response(
        body_stream,
        response_data.effective_url,
        response_data.headers,
        static_cast<int>(response_data.response_code)
    );
}

Response CurlHttpClient::get(
    std::string_view url,
    const std::map<std::string, std::string>& headers
) {
    Request req(url, "GET", headers);
    return execute(req);
}

Response CurlHttpClient::post(
    std::string_view url,
    const std::vector<uint8_t>& data,
    const std::map<std::string, std::string>& headers
) {
    Request req(url, "POST", headers);
    req.set_data(data);
    return execute(req);
}

Response CurlHttpClient::head(
    std::string_view url,
    const std::map<std::string, std::string>& headers
) {
    Request req(url, "HEAD", headers);
    return execute(req);
}

Response CurlHttpClient::put(
    std::string_view url,
    const std::vector<uint8_t>& data,
    const std::map<std::string, std::string>& headers
) {
    Request req(url, "PUT", headers);
    req.set_data(data);
    return execute(req);
}

Response CurlHttpClient::patch(
    std::string_view url,
    const std::vector<uint8_t>& data,
    const std::map<std::string, std::string>& headers
) {
    Request req(url, "PATCH", headers);
    req.set_data(data);
    return execute(req);
}

Response CurlHttpClient::del(
    std::string_view url,
    const std::map<std::string, std::string>& headers
) {
    Request req(url, "DELETE", headers);
    return execute(req);
}

void CurlHttpClient::configure_request(
    CURL* curl,
    const Request& request,
    ResponseData& response_data,
    CurlHeaders& headers
) {
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, request.url().c_str());

    // Set method
    set_method(curl, request.method());

    // Set headers
    if (headers.get()) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.get());
    }

    // Set request body if present
    if (request.has_data()) {
        const auto& data = request.data();
        if (std::holds_alternative<std::vector<uint8_t>>(data)) {
            const auto& bytes = std::get<std::vector<uint8_t>>(data);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, bytes.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bytes.size());
        }
        // TODO: Handle stream data
    }

    // Set proxy
    set_proxy(curl, request);

    // Set response callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_data);

    // Set standard options
    set_curl_options(curl);
}

void CurlHttpClient::set_curl_options(CURL* curl) {
    // Timeouts
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config_.connect_timeout);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout);

    // Low-speed abort (stall detection) - abort if speed < low_speed_limit for low_speed_time seconds
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, config_.low_speed_limit);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, config_.low_speed_time);

    // Redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, config_.follow_redirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, config_.max_redirects);

    // SSL/TLS
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, config_.verify_ssl ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, config_.verify_ssl ? 2L : 0L);

    if (!config_.ca_cert_path.empty()) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, config_.ca_cert_path.c_str());
    }

    // User agent
    if (!config_.user_agent.empty()) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, config_.user_agent.c_str());
    }

    // Verbose debug
    curl_easy_setopt(curl, CURLOPT_VERBOSE, config_.verbose ? 1L : 0L);

    // Accept all encodings
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
}

void CurlHttpClient::set_method(CURL* curl, const std::string& method) {
    if (method == "GET") {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (method == "HEAD") {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    } else {
        // For other methods (PATCH, DELETE, etc.), use CUSTOMREQUEST
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    }
}

void CurlHttpClient::set_proxy(CURL* curl, const Request& request) {
    // Select proxy for this URL
    std::string proxy_url = ytdlp::utils::select_proxy(request.url(), request.proxies());

    if (!proxy_url.empty()) {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy_url.c_str());
    }
}

size_t CurlHttpClient::write_callback(
    char* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
) {
    size_t total_size = size * nmemb;
    auto* response_data = static_cast<ResponseData*>(userdata);

    // Append to body
    response_data->body.insert(
        response_data->body.end(),
        ptr,
        ptr + total_size
    );

    return total_size;
}

size_t CurlHttpClient::header_callback(
    char* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
) {
    size_t total_size = size * nmemb;
    auto* response_data = static_cast<ResponseData*>(userdata);

    // Parse header line
    std::string header_line(ptr, total_size);

    // Find colon separator
    size_t colon_pos = header_line.find(':');
    if (colon_pos != std::string::npos) {
        std::string key = header_line.substr(0, colon_pos);
        std::string value = header_line.substr(colon_pos + 1);

        // Trim whitespace from value
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        // Store header
        response_data->headers[key] = value;
    }

    return total_size;
}

// ============================================================================
// Streaming download implementation
// ============================================================================

struct DownloadContext {
    std::ofstream* file;
    std::function<void(int64_t, int64_t)>* progress_callback;
    int64_t total_bytes;
    int64_t downloaded_bytes;
};

size_t CurlHttpClient::file_write_callback(
    char* ptr,
    size_t size,
    size_t nmemb,
    void* userdata
) {
    size_t total_size = size * nmemb;
    auto* ctx = static_cast<DownloadContext*>(userdata);

    if (ctx->file && ctx->file->is_open()) {
        ctx->file->write(ptr, total_size);
        if (!ctx->file->good()) {
            // Write error
            return 0;
        }
    }

    return total_size;
}

int CurlHttpClient::progress_callback(
    void* clientp,
    curl_off_t dltotal,
    curl_off_t dlnow,
    curl_off_t ultotal,
    curl_off_t ulnow
) {
    auto* ctx = static_cast<DownloadContext*>(clientp);

    if (ctx->progress_callback && *ctx->progress_callback) {
        (*ctx->progress_callback)(static_cast<int64_t>(dlnow), static_cast<int64_t>(dltotal));
    }

    // Return 0 to continue, non-zero to abort
    return 0;
}

bool CurlHttpClient::download_to_file(
    const std::string& url,
    const std::string& output_path,
    const std::map<std::string, std::string>& headers,
    std::function<void(int64_t, int64_t)> progress_callback
) {
    // Open output file
    std::ofstream output_file(output_path, std::ios::binary);
    if (!output_file) {
        throw std::runtime_error("Failed to open output file: " + output_path);
    }

    // Reset curl handle
    curl_.reset();
    CURL* curl = curl_.get();

    // Setup download context
    DownloadContext ctx;
    ctx.file = &output_file;
    ctx.progress_callback = &progress_callback;
    ctx.total_bytes = 0;
    ctx.downloaded_bytes = 0;

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Set headers
    CurlHeaders curl_headers;
    for (const auto& [key, value] : headers) {
        curl_headers.append(key + ": " + value);
    }

    // Add cookies from cookie jar if available
    if (cookie_jar_) {
        std::string cookie_header = cookie_jar_->get_cookie_header(url);
        if (!cookie_header.empty()) {
            curl_headers.append("Cookie: " + cookie_header);
        }
    }

    if (curl_headers.get()) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers.get());
    }

    // Set write callback to write directly to file
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    // Set progress callback (use static member function, not the std::function parameter)
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, CurlHttpClient::progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    // Set standard options
    set_curl_options(curl);

    // Execute request
    CURLcode res = curl_easy_perform(curl);

    // Close file
    output_file.close();

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("CURL error: ") + curl_easy_strerror(res));
    }

    // Check HTTP status code
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (response_code != 200 && response_code != 206) {
        return false;
    }

    return true;
}

} // namespace ytdlp::networking
