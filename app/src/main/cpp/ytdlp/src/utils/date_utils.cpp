#include "ytdlp/utils/date_utils.hpp"
#include <regex>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <map>
#include <algorithm>
#include <cmath>

namespace ytdlp::utils {

namespace {

// Timezone name to offset mapping (in hours)
const std::map<std::string, int> TIMEZONE_NAMES = {
    {"UTC", 0}, {"GMT", 0}, {"Z", 0},
    {"EST", -5}, {"EDT", -4},
    {"CST", -6}, {"CDT", -5},
    {"MST", -7}, {"MDT", -6},
    {"PST", -8}, {"PDT", -7},
    {"CET", 1}, {"CEST", 2},
    {"JST", 9}, {"KST", 9},
    {"IST", 5}, // India Standard Time
    {"AEST", 10}, {"AEDT", 11},
};

// Convert std::string_view to std::string for std::regex (which doesn't support string_view)
inline std::string sv_to_string(std::string_view sv) {
    return std::string(sv);
}

// Case-insensitive string search
bool contains_ignore_case(std::string_view haystack, std::string_view needle) {
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    return it != haystack.end();
}

} // anonymous namespace

std::vector<std::string> date_formats(bool day_first) {
    std::vector<std::string> formats;

    // Add ambiguous formats FIRST based on day_first preference
    // This ensures they're tried before YYYY/MM/DD formats
    if (day_first) {
        formats = {
            "%d/%m/%Y %H:%M:%S",
            "%d-%m-%Y %H:%M",
            "%d-%m-%Y",
            "%d.%m.%Y",
            "%d.%m.%y",
            "%d/%m/%Y",
            "%d/%m/%y",
        };
    } else {
        formats = {
            "%m/%d/%Y %H:%M:%S",
            "%m-%d-%Y",
            "%m.%d.%Y",
            "%m/%d/%Y",
            "%m/%d/%y",
        };
    }

    // Add base formats that are unambiguous
    // More specific formats (with time) should come first to avoid partial matches
    formats.insert(formats.end(), {
        // Formats with time components first
        "%Y-%m-%d %H:%M:%S.%f",
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%d %H:%M",
        "%Y/%m/%d %H:%M:%S",
        "%Y/%m/%d %H:%M",
        "%Y%m%d%H%M%S",
        "%Y%m%d%H%M",
        "%d.%m.%Y %H:%M",
        "%H:%M %d/%m/%Y",
        // Month name formats
        "%d %B %Y",
        "%d %b %Y",
        "%B %d %Y",
        "%b %d %Y",
        // Date-only formats
        "%Y %m %d",
        "%Y-%m-%d",
        "%Y.%m.%d",
        "%Y/%m/%d",
        "%Y%m%d",
        "%d.%m.%Y",
    });

    return formats;
}

TimezoneInfo extract_timezone(std::string_view date_str, std::optional<int> default_offset) {
    std::string str = sv_to_string(date_str);
    TimezoneInfo result;
    result.date_str = str;
    result.offset_seconds = default_offset;

    // Try to match timezone with offset at end: +0100, -05:00, Z, etc.
    // Simplified regex without lookbehind (std::regex doesn't support it)
    std::regex tz_regex(R"(^(.+?)(Z|[\+\-]\d{2}:?\d{2})$)");

    std::smatch match;
    if (std::regex_search(str, match, tz_regex) && match[1].length() >= 8) {
        result.date_str = match[1].str();
        std::string tz_part = match[2].str();

        if (tz_part == "Z") {
            result.offset_seconds = 0;
        } else if (tz_part.length() >= 3) {
            // Parse +0100 or +01:00
            int sign = (tz_part[0] == '+') ? 1 : -1;
            std::string tz_digits = tz_part.substr(1);
            // Remove colon if present
            tz_digits.erase(std::remove(tz_digits.begin(), tz_digits.end(), ':'), tz_digits.end());

            if (tz_digits.length() >= 4) {
                int hours = std::stoi(tz_digits.substr(0, 2));
                int minutes = std::stoi(tz_digits.substr(2, 2));
                result.offset_seconds = sign * (hours * 3600 + minutes * 60);
            }
        }
        return result;
    }

    // Try to match named timezone: PST, EST, UTC, etc.
    std::regex named_tz_regex(R"(\d{1,2}:\d{1,2}(?:\.\d+)?(\s*[A-Z]+)$)");
    if (std::regex_search(str, match, named_tz_regex)) {
        std::string tz_name = match[1].str();
        // Trim whitespace
        tz_name.erase(0, tz_name.find_first_not_of(" \t"));
        tz_name.erase(tz_name.find_last_not_of(" \t") + 1);

        auto it = TIMEZONE_NAMES.find(tz_name);
        if (it != TIMEZONE_NAMES.end()) {
            result.offset_seconds = it->second * 3600;
            result.date_str = str.substr(0, str.length() - match[1].length());
        }
    }

    // If no timezone found and default is nullopt, set to 0 (UTC)
    if (!result.offset_seconds.has_value() && !default_offset.has_value()) {
        result.offset_seconds = 0;
    }

    return result;
}

std::optional<int64_t> parse_iso8601(std::string_view date_str, char delimiter, std::optional<int> timezone) {
    if (date_str.empty()) {
        return std::nullopt;
    }

    std::string str = sv_to_string(date_str);

    // Remove fractional seconds (anything after the decimal point)
    str = std::regex_replace(str, std::regex(R"(\.\d+)"), "");

    // Extract timezone
    TimezoneInfo tz_info = extract_timezone(str, timezone);
    str = tz_info.date_str;

    // Parse the date/time
    std::tm tm = {};
    std::string format;
    if (delimiter == 'T' || delimiter == ' ') {
        format = std::string("%Y-%m-%d") + delimiter + "%H:%M:%S";
    } else {
        format = "%Y-%m-%d %H:%M:%S";  // Default format
    }

    // Try with seconds
    std::istringstream ss(str);
    ss >> std::get_time(&tm, format.c_str());

    if (ss.fail()) {
        // Try without seconds
        tm = {};
        if (delimiter == 'T' || delimiter == ' ') {
            format = std::string("%Y-%m-%d") + delimiter + "%H:%M";
        } else {
            format = "%Y-%m-%d %H:%M";
        }
        ss.clear();
        ss.str(str);
        ss >> std::get_time(&tm, format.c_str());

        if (ss.fail()) {
            return std::nullopt;
        }
    }

    // Convert to timestamp (assuming UTC, then adjust for timezone)
    std::time_t timestamp = timegm(&tm);
    if (timestamp == -1) {
        return std::nullopt;
    }

    // Adjust for timezone offset
    if (tz_info.offset_seconds.has_value()) {
        timestamp -= tz_info.offset_seconds.value();
    }

    return static_cast<int64_t>(timestamp);
}

std::optional<std::string> unified_strdate(std::string_view date_str, bool day_first) {
    if (date_str.empty()) {
        return std::nullopt;
    }

    std::string str = sv_to_string(date_str);

    // Replace commas with spaces
    std::replace(str.begin(), str.end(), ',', ' ');

    // Remove AM/PM and timezone
    str = std::regex_replace(str, std::regex(R"(\s*(?:AM|PM)(?:\s+[A-Z]+)?)", std::regex::icase), "");

    // Extract and remove timezone
    TimezoneInfo tz_info = extract_timezone(str);
    str = tz_info.date_str;

    // Try each date format
    auto formats = date_formats(day_first);
    for (const auto& format : formats) {
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, format.c_str());

        if (!ss.fail() && tm.tm_year != 0) {
            // Successfully parsed, format as YYYYMMDD
            std::ostringstream oss;
            oss << std::setfill('0')
                << std::setw(4) << (tm.tm_year + 1900)
                << std::setw(2) << (tm.tm_mon + 1)
                << std::setw(2) << tm.tm_mday;
            return oss.str();
        }
    }

    return std::nullopt;
}

std::optional<int64_t> unified_timestamp(std::string_view date_str, bool day_first) {
    if (date_str.empty()) {
        return std::nullopt;
    }

    std::string str = sv_to_string(date_str);

    // Remove day names and normalize whitespace
    str = std::regex_replace(str,
        std::regex(R"((?:mon|tues?|wed(?:nes)?|thu(?:rs)?|fri|sat(?:ur)?|sun)(?:day)?)", std::regex::icase),
        "");
    str = std::regex_replace(str, std::regex(R"([,|])"), "");
    str = std::regex_replace(str, std::regex(R"(\s+)"), " ");

    // Check for PM (add 12 hours later)
    int pm_delta = contains_ignore_case(str, "PM") ? 12 : 0;

    // Extract timezone
    TimezoneInfo tz_info = extract_timezone(str);
    str = tz_info.date_str;

    // Remove AM/PM
    str = std::regex_replace(str, std::regex(R"(\s*(?:AM|PM)(?:\s+[A-Z]+)?)", std::regex::icase), "");

    // Remove unrecognized timezones from ISO 8601 alike timestamps
    std::regex tz_suffix_regex(R"(\d{1,2}:\d{1,2}(?:\.\d+)?(\s*[A-Z]+)$)");
    std::smatch tz_match;
    if (std::regex_search(str, tz_match, tz_suffix_regex)) {
        // Remove the timezone suffix
        str = str.substr(0, str.length() - tz_match[1].length());
    }

    // Python only supports microseconds, remove nanoseconds
    std::regex nano_regex(R"(^(\d{4,}-\d{1,2}-\d{1,2}T\d{1,2}:\d{1,2}:\d{1,2}\.\d{6})\d+$)");
    std::smatch match;
    if (std::regex_match(str, match, nano_regex)) {
        str = match[1].str();
    }

    // Try each date format
    auto formats = date_formats(day_first);
    for (const auto& format : formats) {
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, format.c_str());

        if (!ss.fail() && tm.tm_year != 0) {
            // Successfully parsed
            std::time_t timestamp = timegm(&tm);
            if (timestamp != -1) {
                // Adjust for timezone and PM
                int64_t result = static_cast<int64_t>(timestamp);
                if (tz_info.offset_seconds.has_value()) {
                    result -= tz_info.offset_seconds.value();
                }
                result += pm_delta * 3600;
                return result;
            }
        }
    }

    return std::nullopt;
}

int64_t datetime_round(int64_t timestamp, DatePrecision precision) {
    if (precision == DatePrecision::Microsecond) {
        return timestamp;
    }

    int64_t unit_seconds;
    switch (precision) {
        case DatePrecision::Second:
            unit_seconds = 1;
            break;
        case DatePrecision::Minute:
            unit_seconds = 60;
            break;
        case DatePrecision::Hour:
            unit_seconds = 3600;
            break;
        case DatePrecision::Day:
            unit_seconds = 86400;
            break;
        default:
            return timestamp;
    }

    // Round to nearest unit
    // Check if remainder is >= half the unit to decide rounding direction
    int64_t remainder = timestamp % unit_seconds;
    if (remainder < 0) {
        remainder += unit_seconds;  // Handle negative timestamps
    }

    // Use multiplication to avoid integer division issues: remainder * 2 >= unit_seconds
    if (remainder * 2 >= unit_seconds) {
        // Round up
        return timestamp - remainder + unit_seconds;
    } else {
        // Round down
        return timestamp - remainder;
    }
}

std::optional<std::string> strftime_or_none(const std::variant<int64_t, std::string>& timestamp,
                                             std::string_view format) {
    try {
        std::tm tm = {};

        if (std::holds_alternative<int64_t>(timestamp)) {
            // UNIX timestamp
            int64_t ts = std::get<int64_t>(timestamp);
            std::time_t time = static_cast<std::time_t>(ts);
            if (gmtime_r(&time, &tm) == nullptr) {
                return std::nullopt;
            }
        } else {
            // YYYYMMDD string
            const std::string& date_str = std::get<std::string>(timestamp);
            if (date_str.length() != 8) {
                return std::nullopt;
            }
            std::istringstream ss(date_str);
            ss >> std::get_time(&tm, "%Y%m%d");
            if (ss.fail()) {
                return std::nullopt;
            }
        }

        // Format the time
        std::ostringstream oss;
        std::string fmt_str = sv_to_string(format);

        // Support %s on all platforms (UNIX timestamp)
        if (fmt_str.find("%s") != std::string::npos) {
            std::time_t time = timegm(&tm);
            fmt_str = std::regex_replace(fmt_str, std::regex(R"((?<!%)(%%)*%s)"),
                "$1" + std::to_string(time));
        }

        oss << std::put_time(&tm, fmt_str.c_str());
        return oss.str();

    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int64_t> timeconvert(std::string_view timestr) {
    // RFC 2822 date parsing is complex and platform-specific
    // For now, we'll support basic RFC 2822 format
    // Full implementation would use a specialized parser

    if (timestr.empty()) {
        return std::nullopt;
    }

    std::string str = sv_to_string(timestr);

    // Try to parse RFC 2822 format: "Mon, 15 Jan 2023 10:30:45 +0100"
    // Simplified implementation - a full RFC 2822 parser would be more complex

    std::tm tm = {};

    // Try common RFC 2822 format
    std::istringstream ss(str);
    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");

    if (!ss.fail()) {
        // Extract timezone if present
        std::string remaining;
        std::getline(ss, remaining);
        TimezoneInfo tz_info = extract_timezone(remaining);

        std::time_t timestamp = timegm(&tm);
        if (timestamp != -1) {
            int64_t result = static_cast<int64_t>(timestamp);
            if (tz_info.offset_seconds.has_value()) {
                result -= tz_info.offset_seconds.value();
            }
            return result;
        }
    }

    return std::nullopt;
}

std::optional<int64_t> datetime_from_str(std::string_view date_str,
                                          std::string_view format,
                                          DatePrecision precision) {
    if (date_str.empty()) {
        return std::nullopt;
    }

    std::string str = sv_to_string(date_str);
    std::string fmt = sv_to_string(format);

    // Get current time for "now" and "today"
    auto now = std::chrono::system_clock::now();
    int64_t now_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    int64_t today_timestamp = datetime_round(now_timestamp, precision);

    if (str == "now" || str == "today") {
        return today_timestamp;
    }

    if (str == "yesterday") {
        return today_timestamp - 86400;
    }

    // Check for relative date format: "date+5days", "now-2hours", etc.
    std::regex relative_regex(
        R"(^(.+?)([\+\-])(\d+)(microsecond|second|minute|hour|day|week|month|year)s?$)"
    );
    std::smatch match;

    if (std::regex_match(str, match, relative_regex)) {
        // Parse the base date recursively
        auto base_time = datetime_from_str(match[1].str(), format, precision);
        if (!base_time.has_value()) {
            return std::nullopt;
        }

        int sign = (match[2].str() == "+") ? 1 : -1;
        int amount = std::stoi(match[3].str());
        std::string unit = match[4].str();

        int64_t delta = 0;
        if (unit == "microsecond") {
            // Not supported at second precision
            delta = 0;
        } else if (unit == "second") {
            delta = amount;
        } else if (unit == "minute") {
            delta = amount * 60;
        } else if (unit == "hour") {
            delta = amount * 3600;
        } else if (unit == "day") {
            delta = amount * 86400;
        } else if (unit == "week") {
            delta = amount * 7 * 86400;
        } else if (unit == "month") {
            delta = amount * 30 * 86400; // Approximate
        } else if (unit == "year") {
            delta = amount * 365 * 86400; // Approximate
        }

        return base_time.value() + (sign * delta);
    }

    // Parse absolute date using the provided format
    std::tm tm = {};
    std::istringstream ss(str);
    ss >> std::get_time(&tm, fmt.c_str());

    if (ss.fail()) {
        return std::nullopt;
    }

    std::time_t timestamp = timegm(&tm);
    if (timestamp == -1) {
        return std::nullopt;
    }

    return datetime_round(static_cast<int64_t>(timestamp), precision);
}

std::optional<std::string> hyphenate_date(std::string_view date_str) {
    if (date_str.length() != 8) {
        return std::nullopt;
    }

    std::string str = sv_to_string(date_str);

    // Check if all characters are digits
    if (!std::all_of(str.begin(), str.end(), ::isdigit)) {
        return std::nullopt;
    }

    // Format as YYYY-MM-DD
    return str.substr(0, 4) + "-" + str.substr(4, 2) + "-" + str.substr(6, 2);
}

double time_seconds(int days, int hours, int minutes, double seconds,
                    int milliseconds, int microseconds) {
    double total = 0.0;
    total += days * 86400.0;
    total += hours * 3600.0;
    total += minutes * 60.0;
    total += seconds;
    total += milliseconds / 1000.0;
    total += microseconds / 1000000.0;
    return total;
}

} // namespace ytdlp::utils
