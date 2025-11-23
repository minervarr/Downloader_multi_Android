/**
 * Network Utilities Header
 *
 * Provides network-related utilities including user agents, URL parsing/building,
 * query parameter handling, proxy management, and header utilities.
 */

#ifndef YTDLP_UTILS_NETWORK_UTILS_HPP
#define YTDLP_UTILS_NETWORK_UTILS_HPP

#include <string>
#include <string_view>
#include <map>
#include <optional>

namespace ytdlp::utils {

// ============================================================================
// User Agent Constants and Generation
// ============================================================================

namespace user_agents {
    extern const char* CHROME_WINDOWS;
    extern const char* CHROME_MAC;
    extern const char* CHROME_LINUX;
    extern const char* FIREFOX_WINDOWS;
    extern const char* SAFARI_MAC;
    extern const char* EDGE_WINDOWS;
}

/**
 * Generate random Chrome user agent with recent version
 * @return Random Chrome user agent string
 */
std::string random_user_agent();

/**
 * Get standard HTTP headers (including random user agent)
 * @return Map of standard HTTP headers
 */
std::map<std::string, std::string> std_headers();

// ============================================================================
// URL Parsing and Building
// ============================================================================

/**
 * Parse URL into components
 * @param url URL to parse
 * @return Map with keys: scheme, netloc, path, params, query, fragment
 */
std::map<std::string, std::string> parse_url(std::string_view url);

/**
 * Build URL from components
 * @param components Map with URL components
 * @return Reconstructed URL
 */
std::string build_url(const std::map<std::string, std::string>& components);

/**
 * Remove dot segments from path (resolve . and ..)
 * @param path Path to normalize
 * @return Normalized path
 */
std::string remove_dot_segments(std::string_view path);

/**
 * Escape URL according to RFC 3986
 * @param s String to escape
 * @return Escaped URL
 */
std::string escape_rfc3986(std::string_view s);

/**
 * Normalize URL (lowercase netloc, escape non-ASCII, remove dot segments)
 * @param url URL to normalize
 * @return Normalized URL
 */
std::string normalize_url(std::string_view url);

// ============================================================================
// URL Query Parameters
// ============================================================================

/**
 * Extract query parameters from URL
 * @param url URL with query string
 * @return Map of decoded query parameters
 */
std::map<std::string, std::string> extract_query_params(std::string_view url);

/**
 * Update URL query parameters
 * @param url Original URL
 * @param query_update Parameters to add/update
 * @return URL with updated query string
 */
std::string update_url_query(std::string_view url,
                               const std::map<std::string, std::string>& query_update);

/**
 * Update URL components
 * @param url Original URL
 * @param scheme New scheme (optional)
 * @param netloc New netloc (optional)
 * @param path New path (optional)
 * @param query New query (optional)
 * @param fragment New fragment (optional)
 * @return Updated URL
 */
std::string update_url(std::string_view url,
                        std::optional<std::string> scheme = std::nullopt,
                        std::optional<std::string> netloc = std::nullopt,
                        std::optional<std::string> path = std::nullopt,
                        std::optional<std::string> query = std::nullopt,
                        std::optional<std::string> fragment = std::nullopt);

// ============================================================================
// URL Utilities
// ============================================================================

/**
 * Get hostname from URL
 * @param url URL to parse
 * @return Hostname (without port)
 */
std::string get_hostname(std::string_view url);

/**
 * Get port from URL
 * @param url URL to parse
 * @return Port number (80 for http, 443 for https, 0 if none/invalid)
 */
int get_port(std::string_view url);

/**
 * Get scheme from URL
 * @param url URL to parse
 * @return Scheme (e.g., "http", "https")
 */
std::string get_scheme(std::string_view url);

/**
 * Check if URL is HTTPS
 * @param url URL to check
 * @return True if scheme is https
 */
bool is_https(std::string_view url);

/**
 * Convert URL to HTTPS
 * @param url URL to convert
 * @return URL with https scheme
 */
std::string to_https(std::string_view url);

/**
 * Join base URL with relative path
 * @param base Base URL
 * @param path Relative or absolute path/URL
 * @return Joined URL
 */
std::string url_join(std::string_view base, std::string_view path);

// ============================================================================
// HTTP Headers and Proxies
// ============================================================================

/**
 * Clean HTTP headers (remove yt-dlp specific headers)
 * @param headers Headers map to clean (modified in place)
 */
void clean_headers(std::map<std::string, std::string>& headers);

/**
 * Clean and normalize proxy settings
 * @param proxies Proxy map to clean (modified in place)
 * @param headers Headers map (may be modified)
 */
void clean_proxies(std::map<std::string, std::string>& proxies,
                    std::map<std::string, std::string>& headers);

/**
 * Select appropriate proxy for URL
 * @param url URL to access
 * @param proxies Map of proxy settings
 * @return Proxy URL or empty string if no proxy
 */
std::string select_proxy(std::string_view url,
                          const std::map<std::string, std::string>& proxies);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_NETWORK_UTILS_HPP
