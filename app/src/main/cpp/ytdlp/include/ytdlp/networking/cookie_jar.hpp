#ifndef YTDLP_NETWORKING_COOKIE_JAR_HPP
#define YTDLP_NETWORKING_COOKIE_JAR_HPP

#include "ytdlp/networking/cookie.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <tuple>
#include <cstdint>

namespace ytdlp::networking {

/**
 * Cookie Jar
 *
 * Manages HTTP cookies following RFC 6265 and Netscape cookie file format.
 * Supports loading/saving cookies from/to Netscape-format cookie files
 * (used by curl, wget, and browsers).
 *
 * Features:
 * - Netscape cookie file format (compatible with curl --cookie-jar)
 * - HttpOnly cookie support
 * - Automatic cookie expiration handling
 * - Domain and path matching for cookie selection
 * - Session cookie management
 * - Cookie precedence (longest path first)
 *
 * Cookie file format (tab-separated):
 * domain \t flag \t path \t secure \t expiration \t name \t value
 *
 * Example:
 * .example.com \t TRUE \t / \t FALSE \t 1234567890 \t session_id \t abc123
 */
class CookieJar {
public:
    // Constructors
    CookieJar();
    explicit CookieJar(std::string_view filename);

    // Cookie management
    void set_cookie(const Cookie& cookie);
    std::vector<Cookie> get_cookies_for_url(
        std::string_view url,
        bool include_expired = false
    ) const;
    std::string get_cookie_header(std::string_view url) const;

    // File I/O (Netscape format)
    void load(std::string_view filename, bool ignore_expires = false);
    void save(
        std::string_view filename,
        bool ignore_discard = false,
        bool ignore_expires = false
    ) const;

    // Bulk operations
    void clear();
    bool empty() const;
    size_t size() const;
    size_t remove_expired(std::optional<int64_t> now = std::nullopt);
    std::vector<Cookie> get_all_cookies() const;
    void merge(const CookieJar& other);

private:
    // Cookie storage (keyed by domain|path|name)
    std::map<std::string, Cookie> cookies_;

    // Netscape cookie file constants
    static constexpr const char* NETSCAPE_HEADER =
        "# Netscape HTTP Cookie File\n"
        "# This is a generated file! Do not edit.\n\n";
    static constexpr const char* HTTPONLY_PREFIX = "#HttpOnly_";

    // Helper methods
    static std::string make_key(const Cookie& cookie);
    static std::tuple<std::string, std::string, std::string> parse_url(std::string_view url);
};

} // namespace ytdlp::networking

#endif // YTDLP_NETWORKING_COOKIE_JAR_HPP
