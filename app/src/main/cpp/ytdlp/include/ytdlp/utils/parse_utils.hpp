/**
 * Parse Utilities Header
 *
 * Provides parsing utilities for durations, file sizes, numbers, resolutions,
 * bitrates, and age limits. Some functions overlap with number_utils.hpp but
 * provide different parsing approaches.
 */

#ifndef YTDLP_UTILS_PARSE_UTILS_HPP
#define YTDLP_UTILS_PARSE_UTILS_HPP

#include <string>
#include <string_view>
#include <optional>
#include <utility>
#include <cstdint>

namespace ytdlp::utils {

// ============================================================================
// Duration Parsing
// ============================================================================

/**
 * Parse duration string (supports multiple formats)
 * @param s Duration string (e.g., "1:30:45", "90 minutes", "PT1H30M45S")
 * @return Duration in seconds or nullopt if invalid
 */
std::optional<double> parse_duration(std::string_view s);

// ============================================================================
// File Size Parsing
// ============================================================================

/**
 * Parse file size string
 * @param s File size string (e.g., "1.5 GB", "500 MB", "1024 KiB")
 * @return Size in bytes or nullopt if invalid
 */
std::optional<int64_t> parse_filesize(std::string_view s);

/**
 * Format file size as human-readable string
 * @param bytes Size in bytes
 * @param decimal Use decimal (1000) instead of binary (1024) units
 * @return Formatted string
 */
std::string format_filesize(int64_t bytes, bool decimal = false);

// ============================================================================
// Number Parsing
// ============================================================================

/**
 * Parse integer with optional default value
 * @param v String to parse
 * @param scale Scale factor to multiply result
 * @param default_value Default if parsing fails
 * @return Parsed integer or default value
 */
std::optional<int64_t> int_or_none(
    std::string_view v,
    int scale = 1,
    std::optional<int64_t> default_value = std::nullopt
);

/**
 * Parse float with optional default value
 * @param v String to parse
 * @param scale Scale factor to multiply result
 * @param default_value Default if parsing fails
 * @return Parsed float or default value
 */
std::optional<double> float_or_none(
    std::string_view v,
    double scale = 1.0,
    std::optional<double> default_value = std::nullopt
);

/**
 * Convert string to integer (handles thousand separators)
 * @param int_str String to parse
 * @return Integer value or nullopt if invalid
 */
std::optional<int64_t> str_to_int(std::string_view int_str);

/**
 * Parse count string with suffixes (K, M, B, T)
 * @param s Count string
 * @return Count value or nullopt if invalid
 */
std::optional<int64_t> parse_count(std::string_view s);

// ============================================================================
// Resolution Parsing
// ============================================================================

/**
 * Parse resolution string
 * @param resolution Resolution string (e.g., "1920x1080", "1080p")
 * @return Pair of (width, height), either may be nullopt
 */
std::pair<std::optional<int>, std::optional<int>> parse_resolution(
    std::string_view resolution
);

// ============================================================================
// Bitrate Parsing
// ============================================================================

/**
 * Parse bitrate string (e.g., "128kbps", "1.5Mbps")
 * @param s Bitrate string
 * @return Bitrate in bits per second or nullopt if invalid
 */
std::optional<int64_t> parse_bitrate(std::string_view s);

// ============================================================================
// Age Limit Parsing
// ============================================================================

/**
 * Parse age limit string (e.g., "18+", "PG-13")
 * @param s Age limit string
 * @return Age limit or nullopt if invalid
 */
std::optional<int> parse_age_limit(std::string_view s);

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Check if string represents a number
 * @param s String to check
 * @return True if string is a valid number
 */
bool is_number(std::string_view s);

/**
 * Check if string represents an integer
 * @param s String to check
 * @return True if string is a valid integer
 */
bool is_integer(std::string_view s);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_PARSE_UTILS_HPP
