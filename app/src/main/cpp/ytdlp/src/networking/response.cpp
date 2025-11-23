#include "ytdlp/networking/response.hpp"
#include <sstream>

namespace ytdlp::networking {

Response::Response(
    std::shared_ptr<std::istream> body,
    std::string_view url,
    const std::map<std::string, std::string>& headers,
    int status,
    std::string_view reason,
    const std::map<std::string, std::any>& extensions
) : body_(body),
    url_(url),
    status_(status),
    extensions_(extensions) {

    // Set headers
    headers_.update(headers);

    // Set reason (use default if not provided)
    if (reason.empty()) {
        reason_ = default_reason_for_status(status);
    } else {
        reason_ = std::string(reason);
    }
}

std::string Response::default_reason_for_status(int status) {
    // Common HTTP status codes
    switch (status) {
        // 2xx Success
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";

        // 3xx Redirection
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";

        // 4xx Client Error
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";
        case 422: return "Unprocessable Entity";
        case 425: return "Too Early";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 451: return "Unavailable For Legal Reasons";

        // 5xx Server Error
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        case 506: return "Variant Also Negotiates";
        case 507: return "Insufficient Storage";
        case 508: return "Loop Detected";
        case 510: return "Not Extended";
        case 511: return "Network Authentication Required";

        default:
            if (status >= 200 && status < 300) return "Success";
            if (status >= 300 && status < 400) return "Redirection";
            if (status >= 400 && status < 500) return "Client Error";
            if (status >= 500 && status < 600) return "Server Error";
            return "Unknown";
    }
}

std::string Response::get_header(std::string_view name, std::string_view default_value) const {
    auto value = headers_.get(name);
    if (value.has_value()) {
        return value.value();
    }
    return std::string(default_value);
}

std::vector<std::string> Response::get_header_all(std::string_view name) const {
    // HTTPHeaderDict stores single values, so we just return a vector with one element if present
    auto value = headers_.get(name);
    if (value.has_value()) {
        return {value.value()};
    }
    return {};
}

bool Response::readable() const {
    return body_ && body_->good();
}

size_t Response::read(char* buffer, size_t size) {
    if (!body_) {
        return 0;
    }

    body_->read(buffer, size);
    return body_->gcount();
}

std::vector<uint8_t> Response::read_bytes(size_t size) {
    if (!body_) {
        return {};
    }

    std::vector<uint8_t> result;

    if (size == 0) {
        // Read all
        body_->seekg(0, std::ios::end);
        auto stream_size = body_->tellg();
        body_->seekg(0, std::ios::beg);

        if (stream_size > 0) {
            result.resize(stream_size);
            body_->read(reinterpret_cast<char*>(result.data()), stream_size);
        }
    } else {
        // Read specified amount
        result.resize(size);
        body_->read(reinterpret_cast<char*>(result.data()), size);
        result.resize(body_->gcount());
    }

    return result;
}

std::string Response::read_all() {
    if (!body_) {
        return "";
    }

    std::ostringstream ss;
    ss << body_->rdbuf();
    return ss.str();
}

void Response::close() {
    // For std::istream, there's no explicit close
    // The stream will be closed when the shared_ptr is destroyed
    body_.reset();
}

const std::any* Response::get_extension(std::string_view key) const {
    auto it = extensions_.find(std::string(key));
    if (it != extensions_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Response::is_success() const {
    return status_ >= 200 && status_ < 300;
}

} // namespace ytdlp::networking
