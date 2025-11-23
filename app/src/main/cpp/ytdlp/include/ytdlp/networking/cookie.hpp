#ifndef YTDLP_NETWORKING_COOKIE_HPP
#define YTDLP_NETWORKING_COOKIE_HPP

#include <string>
#include <string_view>
#include <optional>
#include <cstdint>

namespace ytdlp::networking {

/**
 * HTTP Cookie
 *
 * Represents an HTTP cookie with all its attributes following RFC 6265.
 * Used by CookieJar for cookie management and persistence.
 *
 * Cookie attributes:
 * - name/value: Cookie data
 * - domain: Domain scope (e.g., ".example.com")
 * - path: Path scope (e.g., "/")
 * - expires: Expiration timestamp (Unix epoch), nullopt for session cookies
 * - secure: Only send over HTTPS
 * - http_only: Not accessible via JavaScript
 *
 * Matching rules:
 * - Domain matching: Supports subdomain matching with leading dot
 * - Path matching: Prefix matching with directory boundary
 * - Expiration: Cookies with no expiration are session cookies
 */
struct Cookie {
    // Cookie data
    std::string name;
    std::string value;

    // Domain and path scope
    std::string domain;
    std::string path = "/";

    // Flags
    bool secure = false;
    bool http_only = false;
    bool domain_specified = false;
    bool path_specified = false;
    bool domain_initial_dot = false;  // Domain starts with '.' (subdomain matching)
    bool discard = false;              // Session cookie (don't persist)

    // Expiration (Unix timestamp in seconds, nullopt means session cookie)
    std::optional<int64_t> expires;

    // Comment and version (rarely used)
    std::string comment;
    std::string comment_url;
    int version = 0;

    // Cookie matching methods
    bool is_expired(std::optional<int64_t> now = std::nullopt) const;
    bool is_session_cookie() const;
    bool domain_matches(std::string_view request_domain) const;
    bool path_matches(std::string_view request_path) const;

    // Serialization
    std::string to_cookie_string() const;  // Returns "name=value" format
};

} // namespace ytdlp::networking

#endif // YTDLP_NETWORKING_COOKIE_HPP
