/**
 * Date/Time Utilities Header
 *
 * Provides date and time parsing utilities including ISO 8601, various date formats,
 * timezone handling, and date arithmetic.
 */

#ifndef YTDLP_UTILS_DATE_UTILS_HPP
#define YTDLP_UTILS_DATE_UTILS_HPP

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <cstdint>

namespace ytdlp::utils {

// ============================================================================
// Date Precision Enum
// ============================================================================

enum class DatePrecision {
    Microsecond,
    Second,
    Minute,
    Hour,
    Day
};

// ============================================================================
// Timezone Information
// ============================================================================

struct TimezoneInfo {
    std::string date_str;              // Date string with timezone removed
    std::optional<int> offset_seconds; // Timezone offset in seconds
};

// ============================================================================
// Date Format Lists
// ============================================================================

/**
 * Get list of date formats to try when parsing
 * @param day_first If true, interpret ambiguous dates as DD/MM/YYYY instead of MM/DD/YYYY
 * @return Vector of strptime format strings
 */
std::vector<std::string> date_formats(bool day_first = true);

// ============================================================================
// Timezone Extraction
// ============================================================================

/**
 * Extract timezone information from date string
 * @param date_str Date string possibly containing timezone
 * @param default_offset Default offset if no timezone found
 * @return TimezoneInfo with date and offset
 */
TimezoneInfo extract_timezone(std::string_view date_str,
                                std::optional<int> default_offset = std::nullopt);

// ============================================================================
// Date Parsing
// ============================================================================

/**
 * Parse ISO 8601 date/time string
 * @param date_str ISO 8601 date string (e.g., "2023-01-15T10:30:45Z")
 * @param delimiter Delimiter between date and time ('T' or ' ')
 * @param timezone Optional timezone offset in seconds
 * @return Unix timestamp or nullopt if invalid
 */
std::optional<int64_t> parse_iso8601(std::string_view date_str,
                                      char delimiter = 'T',
                                      std::optional<int> timezone = std::nullopt);

/**
 * Parse date string and return as YYYYMMDD string
 * @param date_str Date string in various formats
 * @param day_first If true, interpret ambiguous dates as DD/MM/YYYY
 * @return Date as YYYYMMDD string or nullopt if invalid
 */
std::optional<std::string> unified_strdate(std::string_view date_str, bool day_first = true);

/**
 * Parse date string and return as Unix timestamp
 * @param date_str Date string in various formats
 * @param day_first If true, interpret ambiguous dates as DD/MM/YYYY
 * @return Unix timestamp or nullopt if invalid
 */
std::optional<int64_t> unified_timestamp(std::string_view date_str, bool day_first = true);

/**
 * Parse RFC 2822 date string
 * @param timestr RFC 2822 date string (e.g., "Mon, 15 Jan 2023 10:30:45 +0100")
 * @return Unix timestamp or nullopt if invalid
 */
std::optional<int64_t> timeconvert(std::string_view timestr);

/**
 * Parse date/time with relative offsets
 * @param date_str Date string (supports "now", "today", "yesterday", "now-2hours", etc.)
 * @param format strptime format string
 * @param precision Precision for rounding
 * @return Unix timestamp or nullopt if invalid
 */
std::optional<int64_t> datetime_from_str(std::string_view date_str,
                                          std::string_view format = "%Y%m%d",
                                          DatePrecision precision = DatePrecision::Day);

// ============================================================================
// Date Formatting
// ============================================================================

/**
 * Format timestamp or date string using strftime
 * @param timestamp Unix timestamp (int64_t) or YYYYMMDD string
 * @param format strftime format string
 * @return Formatted string or nullopt if invalid
 */
std::optional<std::string> strftime_or_none(const std::variant<int64_t, std::string>& timestamp,
                                             std::string_view format);

/**
 * Convert YYYYMMDD to YYYY-MM-DD
 * @param date_str Date string (8 digits)
 * @return Hyphenated date or nullopt if invalid
 */
std::optional<std::string> hyphenate_date(std::string_view date_str);

// ============================================================================
// Date/Time Arithmetic
// ============================================================================

/**
 * Round timestamp to specified precision
 * @param timestamp Unix timestamp
 * @param precision Precision to round to
 * @return Rounded timestamp
 */
int64_t datetime_round(int64_t timestamp, DatePrecision precision);

/**
 * Calculate total seconds from time components
 * @param days Days
 * @param hours Hours
 * @param minutes Minutes
 * @param seconds Seconds
 * @param milliseconds Milliseconds
 * @param microseconds Microseconds
 * @return Total seconds (as double)
 */
double time_seconds(int days = 0, int hours = 0, int minutes = 0, double seconds = 0.0,
                    int milliseconds = 0, int microseconds = 0);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_DATE_UTILS_HPP
