#include "ytdlp/networking/cookie_jar.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include "ytdlp/utils/network_utils.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <tuple>
#include <stdexcept>

namespace ytdlp::networking {

CookieJar::CookieJar() = default;

CookieJar::CookieJar(std::string_view filename) {
    load(filename);
}

void CookieJar::set_cookie(const Cookie& cookie) {
    std::string key = make_key(cookie);
    cookies_[key] = cookie;
}

std::vector<Cookie> CookieJar::get_cookies_for_url(
    std::string_view url,
    bool include_expired
) const {
    auto [scheme, domain, path] = parse_url(url);
    bool is_secure = (scheme == "https");
    int64_t now = static_cast<int64_t>(std::time(nullptr));

    std::vector<Cookie> matching_cookies;

    for (const auto& [key, cookie] : cookies_) {
        // Check expiration
        if (!include_expired && cookie.is_expired(now)) {
            continue;
        }

        // Check domain
        if (!cookie.domain_matches(domain)) {
            continue;
        }

        // Check path
        if (!cookie.path_matches(path)) {
            continue;
        }

        // Check secure flag
        if (cookie.secure && !is_secure) {
            continue;
        }

        matching_cookies.push_back(cookie);
    }

    // Sort by path length (longest first) for proper cookie precedence
    std::sort(matching_cookies.begin(), matching_cookies.end(),
        [](const Cookie& a, const Cookie& b) {
            // Longer paths have higher precedence
            if (a.path.length() != b.path.length()) {
                return a.path.length() > b.path.length();
            }
            // If paths are equal, sort by name for deterministic ordering
            return a.name < b.name;
        });

    return matching_cookies;
}

std::string CookieJar::get_cookie_header(std::string_view url) const {
    auto cookies = get_cookies_for_url(url);

    if (cookies.empty()) {
        return "";
    }

    std::string header;
    for (size_t i = 0; i < cookies.size(); ++i) {
        if (i > 0) {
            header += "; ";
        }
        header += cookies[i].to_cookie_string();
    }

    return header;
}

void CookieJar::load(std::string_view filename, bool ignore_expires) {
    std::ifstream file{std::string(filename)};
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open cookie file: " + std::string(filename));
    }

    std::string line;
    int line_number = 0;
    int64_t now = static_cast<int64_t>(std::time(nullptr));

    while (std::getline(file, line)) {
        ++line_number;

        // Trim whitespace
        line = ytdlp::utils::strip(line);

        // Skip empty lines and comments (except HttpOnly)
        if (line.empty()) {
            continue;
        }

        // Check for HttpOnly prefix
        bool http_only = false;
        if (ytdlp::utils::starts_with(line, HTTPONLY_PREFIX)) {
            http_only = true;
            line = line.substr(std::strlen(HTTPONLY_PREFIX));
        }

        // Skip comment lines
        if (ytdlp::utils::starts_with(line, "#")) {
            continue;
        }

        // Parse cookie line: domain \t flag \t path \t secure \t expiration \t name \t value
        std::vector<std::string> fields = ytdlp::utils::split(line, "\t");

        if (fields.size() != 7) {
            // Skip malformed lines with a warning
            continue;
        }

        Cookie cookie;
        cookie.domain = fields[0];
        cookie.domain_initial_dot = (fields[1] == "TRUE");
        cookie.path = fields[2];
        cookie.secure = (fields[3] == "TRUE");
        cookie.name = fields[5];
        cookie.value = fields[6];
        cookie.http_only = http_only;
        cookie.domain_specified = true;
        cookie.path_specified = true;

        // Parse expiration
        if (!fields[4].empty()) {
            try {
                int64_t expiry = std::stoll(fields[4]);

                // Treat 0 as session cookie
                if (expiry == 0) {
                    cookie.expires = std::nullopt;
                    cookie.discard = true;
                } else {
                    cookie.expires = expiry;

                    // Skip expired cookies unless ignore_expires is set
                    if (!ignore_expires && expiry < now) {
                        continue;
                    }
                }
            } catch (const std::exception&) {
                // Invalid expiration, treat as session cookie
                cookie.expires = std::nullopt;
                cookie.discard = true;
            }
        } else {
            // Empty expiration means session cookie
            cookie.expires = std::nullopt;
            cookie.discard = true;
        }

        set_cookie(cookie);
    }
}

void CookieJar::save(
    std::string_view filename,
    bool ignore_discard,
    bool ignore_expires
) const {
    std::ofstream file{std::string(filename)};
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open cookie file for writing: " + std::string(filename));
    }

    // Write header
    file << NETSCAPE_HEADER;

    int64_t now = static_cast<int64_t>(std::time(nullptr));

    for (const auto& [key, cookie] : cookies_) {
        // Skip session cookies unless ignore_discard is set
        if (!ignore_discard && cookie.discard) {
            continue;
        }

        // Skip expired cookies unless ignore_expires is set
        if (!ignore_expires && cookie.is_expired(now)) {
            continue;
        }

        // Write HttpOnly prefix if needed
        if (cookie.http_only) {
            file << HTTPONLY_PREFIX;
        }

        // Write cookie fields
        std::string name = cookie.name;
        std::string value = cookie.value;

        // Handle cookies with no name (value becomes name)
        if (name.empty()) {
            name = "";
            value = cookie.name.empty() ? cookie.value : cookie.name;
        }

        std::string expires_str;
        if (cookie.is_session_cookie()) {
            // Session cookies: use 0
            expires_str = "0";
        } else if (cookie.expires.has_value()) {
            expires_str = std::to_string(cookie.expires.value());
        } else {
            expires_str = "0";
        }

        file << cookie.domain << "\t"
             << (cookie.domain_initial_dot ? "TRUE" : "FALSE") << "\t"
             << cookie.path << "\t"
             << (cookie.secure ? "TRUE" : "FALSE") << "\t"
             << expires_str << "\t"
             << name << "\t"
             << value << "\n";
    }
}

void CookieJar::clear() {
    cookies_.clear();
}

bool CookieJar::empty() const {
    return cookies_.empty();
}

size_t CookieJar::size() const {
    return cookies_.size();
}

size_t CookieJar::remove_expired(std::optional<int64_t> now) {
    int64_t current_time = now.value_or(static_cast<int64_t>(std::time(nullptr)));
    size_t removed = 0;

    auto it = cookies_.begin();
    while (it != cookies_.end()) {
        if (it->second.is_expired(current_time)) {
            it = cookies_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }

    return removed;
}

std::vector<Cookie> CookieJar::get_all_cookies() const {
    std::vector<Cookie> result;
    result.reserve(cookies_.size());

    for (const auto& [key, cookie] : cookies_) {
        result.push_back(cookie);
    }

    return result;
}

void CookieJar::merge(const CookieJar& other) {
    for (const auto& [key, cookie] : other.cookies_) {
        cookies_[key] = cookie;
    }
}

std::string CookieJar::make_key(const Cookie& cookie) {
    // Key format: domain|path|name
    return cookie.domain + "|" + cookie.path + "|" + cookie.name;
}

std::tuple<std::string, std::string, std::string> CookieJar::parse_url(
    std::string_view url
) {
    // Extract scheme
    std::string scheme;
    std::string rest(url);

    size_t scheme_end = rest.find("://");
    if (scheme_end != std::string::npos) {
        scheme = rest.substr(0, scheme_end);
        rest = rest.substr(scheme_end + 3);
    } else {
        scheme = "http";
    }

    scheme = ytdlp::utils::to_lower(scheme);

    // Extract domain and path
    std::string domain;
    std::string path = "/";

    size_t path_start = rest.find('/');
    if (path_start != std::string::npos) {
        domain = rest.substr(0, path_start);
        path = rest.substr(path_start);
    } else {
        domain = rest;
    }

    // Remove port from domain
    size_t port_start = domain.find(':');
    if (port_start != std::string::npos) {
        domain = domain.substr(0, port_start);
    }

    // Remove query string and fragment from path
    size_t query_start = path.find('?');
    if (query_start != std::string::npos) {
        path = path.substr(0, query_start);
    }

    size_t fragment_start = path.find('#');
    if (fragment_start != std::string::npos) {
        path = path.substr(0, fragment_start);
    }

    return {scheme, domain, path};
}

} // namespace ytdlp::networking
