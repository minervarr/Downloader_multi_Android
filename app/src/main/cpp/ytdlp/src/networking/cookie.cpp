#include "ytdlp/networking/cookie.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <algorithm>
#include <ctime>

namespace ytdlp::networking {

bool Cookie::is_expired(std::optional<int64_t> now) const {
    // Session cookies (no expiration) never expire
    if (!expires.has_value()) {
        return false;
    }

    // expires == 0 means session cookie
    if (expires.value() == 0) {
        return false;
    }

    // Get current time if not provided
    int64_t current_time = now.value_or(static_cast<int64_t>(std::time(nullptr)));

    return expires.value() < current_time;
}

bool Cookie::is_session_cookie() const {
    return !expires.has_value() || expires.value() == 0;
}

bool Cookie::domain_matches(std::string_view request_domain) const {
    // Convert both to lowercase for case-insensitive comparison
    std::string cookie_domain_lower = ytdlp::utils::to_lower(domain);
    std::string request_domain_lower = ytdlp::utils::to_lower(std::string(request_domain));

    // Exact match
    if (cookie_domain_lower == request_domain_lower) {
        return true;
    }

    // If cookie domain starts with '.', it matches subdomains
    if (domain_initial_dot) {
        // Remove leading dot for matching
        std::string domain_without_dot = cookie_domain_lower;
        if (ytdlp::utils::starts_with(domain_without_dot, ".")) {
            domain_without_dot = domain_without_dot.substr(1);
        }

        // Check if request domain ends with cookie domain
        // e.g., ".example.com" matches "www.example.com"
        if (request_domain_lower == domain_without_dot) {
            return true;
        }

        // Check if request domain is a subdomain
        // e.g., ".example.com" matches "sub.example.com"
        if (ytdlp::utils::ends_with(request_domain_lower, "." + domain_without_dot)) {
            return true;
        }
    }

    return false;
}

bool Cookie::path_matches(std::string_view request_path) const {
    // Cookie path must be a prefix of request path
    if (ytdlp::utils::starts_with(request_path, path)) {
        // Exact match
        if (request_path.size() == path.size()) {
            return true;
        }

        // Path must be a directory prefix
        // e.g., "/foo" matches "/foo/bar" but not "/foobar"
        if (ytdlp::utils::ends_with(path, "/")) {
            return true;
        }

        // Check if next character is '/'
        if (request_path[path.size()] == '/') {
            return true;
        }
    }

    return false;
}

std::string Cookie::to_cookie_string() const {
    // Handle edge case where cookie has no name (value becomes name)
    if (name.empty()) {
        return value;
    }

    return name + "=" + value;
}

} // namespace ytdlp::networking
