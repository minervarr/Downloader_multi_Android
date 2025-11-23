/**
 * String Utilities Header
 *
 * Provides string manipulation utilities including URL encoding/decoding,
 * HTML escaping/unescaping, filename sanitization, and general string operations.
 */

#ifndef YTDLP_UTILS_STRING_UTILS_HPP
#define YTDLP_UTILS_STRING_UTILS_HPP

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <tuple>

namespace ytdlp::utils {

// ============================================================================
// URL Encoding/Decoding
// ============================================================================

/**
 * URL-encode a string
 * @param str String to encode
 * @param safe Characters that should not be encoded (in addition to alphanumeric, -, _, ., ~)
 * @return URL-encoded string
 */
std::string url_encode(std::string_view str, std::string_view safe = "");

/**
 * URL-decode a string
 * @param str URL-encoded string
 * @return Decoded string
 */
std::string url_decode(std::string_view str);

/**
 * Encode a map of parameters as URL-encoded POST data
 * @param params Key-value pairs to encode
 * @return URL-encoded string (key1=value1&key2=value2)
 */
std::string urlencode_postdata(const std::map<std::string, std::string>& params);

// ============================================================================
// HTML Escaping/Unescaping
// ============================================================================

/**
 * Escape HTML special characters
 * @param text Text to escape
 * @return HTML-escaped string
 */
std::string escape_html(std::string_view text);

/**
 * Unescape HTML entities
 * @param html HTML string with entities
 * @return Unescaped string
 */
std::string unescape_html(std::string_view html);

// ============================================================================
// Filename Sanitization
// ============================================================================

/**
 * Sanitize filename for safe filesystem use
 * @param filename Filename to sanitize
 * @param restricted Use restricted character set (ASCII only)
 * @param is_id Whether this is an ID (less aggressive sanitization)
 * @return Sanitized filename
 */
std::string sanitize_filename(
    std::string_view filename,
    bool restricted = false,
    std::optional<bool> is_id = std::nullopt
);

/**
 * Sanitize a file path
 * @param path Path to sanitize
 * @param force Force sanitization even on Unix systems
 * @return Sanitized path
 */
std::string sanitize_path(std::string_view path, bool force = false);

// ============================================================================
// URL Sanitization
// ============================================================================

/**
 * Sanitize a URL by adding scheme if missing
 * @param url URL to sanitize
 * @param scheme Default scheme to use (default: "http")
 * @return Sanitized URL
 */
std::string sanitize_url(std::string_view url, std::string_view scheme = "http");

/**
 * Extract basic authentication from URL
 * @param url URL potentially containing user:pass@host
 * @return Tuple of (url_without_auth, username, password)
 */
std::tuple<std::string, std::string, std::string> extract_basic_auth(std::string_view url);

// ============================================================================
// String Manipulation
// ============================================================================

/**
 * Strip whitespace from both ends of string
 * @param str String to strip
 * @return Stripped string
 */
std::string strip(std::string_view str);

/**
 * Strip specific character from both ends of string
 * @param str String to strip
 * @param ch Character to remove
 * @return Stripped string
 */
std::string strip(std::string_view str, char ch);

/**
 * Strip whitespace from left end of string
 * @param str String to strip
 * @return Left-stripped string
 */
std::string lstrip(std::string_view str);

/**
 * Strip whitespace from right end of string
 * @param str String to strip
 * @return Right-stripped string
 */
std::string rstrip(std::string_view str);

/**
 * Convert string to lowercase
 * @param str String to convert
 * @return Lowercase string
 */
std::string to_lower(std::string_view str);

/**
 * Convert string to uppercase
 * @param str String to convert
 * @return Uppercase string
 */
std::string to_upper(std::string_view str);

/**
 * Split string by delimiter character
 * @param str String to split
 * @param delimiter Character to split on
 * @param max_split Maximum number of splits (-1 for unlimited)
 * @return Vector of substrings
 */
std::vector<std::string> split(std::string_view str, char delimiter, int max_split = -1);

/**
 * Split string by delimiter string
 * @param str String to split
 * @param delimiter String to split on
 * @param max_split Maximum number of splits (-1 for unlimited)
 * @return Vector of substrings
 */
std::vector<std::string> split(std::string_view str, std::string_view delimiter, int max_split = -1);

/**
 * Join vector of strings with delimiter
 * @param strings Strings to join
 * @param delimiter Delimiter to insert between strings
 * @return Joined string
 */
std::string join(const std::vector<std::string>& strings, std::string_view delimiter);

/**
 * Check if string starts with prefix
 * @param str String to check
 * @param prefix Prefix to look for
 * @return True if string starts with prefix
 */
bool starts_with(std::string_view str, std::string_view prefix);

/**
 * Check if string ends with suffix
 * @param str String to check
 * @param suffix Suffix to look for
 * @return True if string ends with suffix
 */
bool ends_with(std::string_view str, std::string_view suffix);

/**
 * Replace all occurrences of substring
 * @param str String to search in
 * @param from Substring to find
 * @param to Replacement string
 * @return String with all replacements made
 */
std::string replace_all(std::string_view str, std::string_view from, std::string_view to);

// ============================================================================
// Format Detection
// ============================================================================

/**
 * Determine file extension from URL or filename
 * @param url URL or filename
 * @param default_ext Default extension if none found
 * @return File extension (without dot)
 */
std::string determine_ext(std::string_view url, std::string_view default_ext = "");

// ============================================================================
// Number Parsing
// ============================================================================

/**
 * Parse string to integer
 * @param str String to parse
 * @return Parsed integer or nullopt if invalid
 */
std::optional<int> parse_int(std::string_view str);

/**
 * Parse string to floating point number
 * @param str String to parse
 * @return Parsed double or nullopt if invalid
 */
std::optional<double> parse_float(std::string_view str);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_STRING_UTILS_HPP
