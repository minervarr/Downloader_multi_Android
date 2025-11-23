/**
 * Number Utilities Header
 *
 * Provides number parsing and formatting utilities including file sizes,
 * counts, durations, resolutions, and bitrates.
 */

#ifndef YTDLP_UTILS_NUMBER_UTILS_HPP
#define YTDLP_UTILS_NUMBER_UTILS_HPP

#include <string>
#include <string_view>
#include <map>
#include <optional>
#include <cstdint>

namespace ytdlp::utils {

// ============================================================================
// File Size Parsing and Formatting
// ============================================================================

/**
 * Parse file size string (e.g., "1.5 GB", "500 MB", "1024 KiB")
 * @param s File size string
 * @return Size in bytes or nullopt if invalid
 */
std::optional<int64_t> parse_filesize(std::string_view s);

/**
 * Format bytes as human-readable file size
 * @param bytes Size in bytes
 * @return Formatted string (e.g., "1.50GiB")
 */
std::string format_bytes(int64_t bytes);

// ============================================================================
// Count Parsing
// ============================================================================

/**
 * Parse count string with suffixes (e.g., "1.2K", "5M", "10B")
 * @param s Count string
 * @return Count value or nullopt if invalid
 */
std::optional<int64_t> parse_count(std::string_view s);

/**
 * Convert string to integer (handles commas and periods as separators)
 * @param s String to parse
 * @return Integer value or nullopt if invalid
 */
std::optional<int64_t> str_to_int(std::string_view s);

// ============================================================================
// Duration Parsing and Formatting
// ============================================================================

/**
 * Parse duration string (e.g., "1:30:45", "90 minutes", "PT1H30M45S")
 * @param s Duration string
 * @return Duration in seconds or nullopt if invalid
 */
std::optional<double> parse_duration(std::string_view s);

/**
 * Format seconds as duration string
 * @param seconds Duration in seconds
 * @return Formatted string (e.g., "1:30:45" or "5:23")
 */
std::string format_duration(double seconds);

// ============================================================================
// Resolution Parsing
// ============================================================================

/**
 * Parse resolution string (e.g., "1920x1080", "1080p", "4k")
 * @param s Resolution string
 * @param lenient If true, allow partial matches
 * @return Map with "width" and/or "height" keys (empty if invalid)
 */
std::map<std::string, int> parse_resolution(std::string_view s, bool lenient = false);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_NUMBER_UTILS_HPP
