/**
 * Parse Utilities Implementation
 */

#include "ytdlp/utils/parse_utils.hpp"
#include "ytdlp/utils/string_utils.hpp"

#include <regex>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <charconv>

namespace ytdlp::utils {

// ============================================================================
// Duration Parsing
// ============================================================================

std::optional<double> parse_duration(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str = strip(s);
    if (str.empty()) {
        return std::nullopt;
    }

    // Check if it's a simple number (plain seconds)
    std::regex simple_number_regex(R"([0-9]+(?:\.[0-9]+)?)", std::regex::optimize);
    if (std::regex_match(str, simple_number_regex)) {
        try {
            return std::stod(str);
        } catch (...) {
            return std::nullopt;
        }
    }

    // Try time format with colons: HH:MM:SS, MM:SS
    std::regex time_regex(
        R"((?:([0-9]+):)?([0-9]+):([0-9]+)(?:\.([0-9]+))?)",
        std::regex::optimize
    );
    std::smatch match;

    if (std::regex_match(str, match, time_regex)) {
        double hours = 0, mins = 0, secs = 0, ms = 0;

        if (match[1].matched) {
            hours = std::stod(match[1].str());
            mins = std::stod(match[2].str());
            secs = std::stod(match[3].str());
        } else {
            // MM:SS format
            mins = std::stod(match[2].str());
            secs = std::stod(match[3].str());
        }

        if (match[4].matched) {
            std::string ms_str = "0." + match[4].str();
            ms = std::stod(ms_str);
        }

        return hours * 3600 + mins * 60 + secs + ms;
    }

    // Try ISO 8601 / descriptive format: PT1H30M45S, 90 minutes, etc.
    std::regex iso_regex(
        R"((?:P?(?:[0-9]+\s*[Yy](?:[Ee][Aa][Rr][Ss]?)?,?\s*)?(?:[0-9]+\s*[Mm](?:[Oo][Nn][Tt][Hh][Ss]?)?,?\s*)?)"
        R"((?:[0-9]+\s*[Ww](?:[Ee][Ee][Kk][Ss]?)?,?\s*)?(?:([0-9]+)\s*[Dd](?:[Aa][Yy][Ss]?)?,?\s*)?[Tt]?)?)"
        R"((?:([0-9]+)\s*[Hh](?:(?:[Oo][Uu])?[Rr][Ss]?)?,?\s*)?)"
        R"((?:([0-9]+)\s*[Mm](?:[Ii][Nn](?:[Uu][Tt][Ee])?[Ss]?)?,?\s*)?)"
        R"((?:([0-9]+)(?:\.([0-9]+))?\s*[Ss](?:[Ee][Cc](?:[Oo][Nn][Dd])?[Ss]?)?\s*)?[Zz]?)",
        std::regex::optimize
    );

    if (std::regex_match(str, match, iso_regex)) {
        double days = 0, hours = 0, mins = 0, secs = 0, ms = 0;

        if (match[1].matched) days = std::stod(match[1].str());
        if (match[2].matched) hours = std::stod(match[2].str());
        if (match[3].matched) mins = std::stod(match[3].str());
        if (match[4].matched) secs = std::stod(match[4].str());
        if (match[5].matched) {
            std::string ms_str = "0." + match[5].str();
            ms = std::stod(ms_str);
        }

        return days * 86400 + hours * 3600 + mins * 60 + secs + ms;
    }

    // Try simple "N hours" or "N minutes" format
    std::regex simple_regex(
        R"((?:([0-9.]+)\s*(?:[Hh][Oo][Uu][Rr][Ss]?)|([0-9.]+)\s*(?:[Mm][Ii][Nn][Ss]?\.?|[Mm][Ii][Nn][Uu][Tt][Ee][Ss]?))\s*[Zz]?)",
        std::regex::optimize
    );

    if (std::regex_match(str, match, simple_regex)) {
        if (match[1].matched) {
            return std::stod(match[1].str()) * 3600;
        }
        if (match[2].matched) {
            return std::stod(match[2].str()) * 60;
        }
    }

    return std::nullopt;
}

// ============================================================================
// File Size Parsing
// ============================================================================

std::optional<int64_t> parse_filesize(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str = strip(s);
    if (str.empty()) {
        return std::nullopt;
    }

    // Unit table (order matters: longer units first)
    // Note: YiB and ZiB are too large for int64_t (would overflow)
    static const std::vector<std::pair<std::string, int64_t>> units = {
        // Binary (IEC) units
        {"exbibytes", 1LL << 60}, {"EiB", 1LL << 60}, {"eB", 1LL << 60},
        {"pebibytes", 1LL << 50}, {"PiB", 1LL << 50}, {"pB", 1LL << 50},
        {"tebibytes", 1LL << 40}, {"TiB", 1LL << 40}, {"tB", 1LL << 40},
        {"gibibytes", 1LL << 30}, {"GiB", 1LL << 30}, {"gB", 1LL << 30},
        {"mebibytes", 1LL << 20}, {"MiB", 1LL << 20}, {"mB", 1LL << 20},
        {"kibibytes", 1LL << 10}, {"KiB", 1LL << 10}, {"kB", 1LL << 10},

        // Decimal (SI) units (up to exabyte - larger values would overflow int64_t)
        {"exabytes", 1000000000000000000LL}, {"EB", 1000000000000000000LL}, {"Eb", 1000000000000000000LL}, {"eb", 1000000000000000000LL},
        {"petabytes", 1000000000000000LL}, {"PB", 1000000000000000LL}, {"Pb", 1000000000000000LL}, {"pb", 1000000000000000LL},
        {"terabytes", 1000000000000LL}, {"TB", 1000000000000LL}, {"Tb", 1000000000000LL}, {"tb", 1000000000000LL},
        {"gigabytes", 1000000000LL}, {"GB", 1000000000LL}, {"Gb", 1000000000LL}, {"gb", 1000000000LL},
        {"megabytes", 1000000LL}, {"MB", 1000000LL}, {"Mb", 1000000LL}, {"mb", 1000000LL},
        {"kilobytes", 1000LL}, {"KB", 1000LL}, {"Kb", 1000LL}, {"kb", 1000LL},

        // Bytes
        {"bytes", 1}, {"B", 1}, {"b", 1}
    };

    // Try to match number + unit
    std::regex size_regex(R"(^\s*([0-9.]+)\s*([a-zA-Z]+)?\s*$)", std::regex::optimize);
    std::smatch match;

    if (std::regex_match(str, match, size_regex)) {
        double value = std::stod(match[1].str());

        if (match[2].matched) {
            std::string unit = match[2].str();

            // Find matching unit
            for (const auto& [unit_name, multiplier] : units) {
                if (unit == unit_name) {
                    return static_cast<int64_t>(value * multiplier);
                }
            }

            // Unit not found
            return std::nullopt;
        } else {
            // No unit, assume bytes
            return static_cast<int64_t>(value);
        }
    }

    return std::nullopt;
}

std::string format_filesize(int64_t bytes, bool decimal) {
    static const std::vector<std::pair<std::string, int64_t>> decimal_units = {
        {"EB", 1000000000000000000LL},
        {"PB", 1000000000000000LL},
        {"TB", 1000000000000LL},
        {"GB", 1000000000LL},
        {"MB", 1000000LL},
        {"KB", 1000LL},
        {"B", 1LL}
    };

    static const std::vector<std::pair<std::string, int64_t>> binary_units = {
        {"EiB", 1LL << 60},
        {"PiB", 1LL << 50},
        {"TiB", 1LL << 40},
        {"GiB", 1LL << 30},
        {"MiB", 1LL << 20},
        {"KiB", 1LL << 10},
        {"B", 1LL}
    };

    const auto& units = decimal ? decimal_units : binary_units;

    for (const auto& [unit, divisor] : units) {
        if (bytes >= divisor) {
            double value = static_cast<double>(bytes) / divisor;
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.2f%s", value, unit.c_str());
            return buffer;
        }
    }

    return "0B";
}

// ============================================================================
// Number Parsing
// ============================================================================

std::optional<int64_t> int_or_none(
    std::string_view v,
    int scale,
    std::optional<int64_t> default_value
) {
    if (v.empty()) {
        return default_value;
    }

    try {
        std::string str(v);
        int64_t result = std::stoll(str);
        return result * scale;
    } catch (...) {
        return default_value;
    }
}

std::optional<double> float_or_none(
    std::string_view v,
    double scale,
    std::optional<double> default_value
) {
    if (v.empty()) {
        return default_value;
    }

    try {
        std::string str(v);
        double result = std::stod(str);
        return result * scale;
    } catch (...) {
        return default_value;
    }
}

std::optional<int64_t> str_to_int(std::string_view int_str) {
    if (int_str.empty()) {
        return std::nullopt;
    }

    // Check for decimal point - if present, this is not an integer
    if (int_str.find('.') != std::string_view::npos) {
        return std::nullopt;
    }

    // Remove thousand separators (comma, space, underscore)
    std::string cleaned;
    cleaned.reserve(int_str.size());

    for (char c : int_str) {
        if (c != ',' && c != ' ' && c != '_') {
            cleaned += c;
        }
    }

    if (cleaned.empty()) {
        return std::nullopt;
    }

    try {
        return std::stoll(cleaned);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<int64_t> parse_count(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str = strip(s);
    if (str.empty()) {
        return std::nullopt;
    }

    // Remove leading non-digit characters
    std::regex prefix_regex(R"(^[^\d]+\s)");
    str = std::regex_replace(str, prefix_regex, "");
    str = strip(str);

    // Try parsing with suffix (K, M, B, T)
    std::regex count_regex(R"(([0-9,.]+)\s*([KMBTkmbt])?)", std::regex::optimize);
    std::smatch match;

    if (std::regex_match(str, match, count_regex)) {
        std::string num_str = match[1].str();

        // Handle floating point with suffix (e.g., "1.2K")
        if (num_str.find('.') != std::string::npos) {
            try {
                double base = std::stod(num_str);

                if (match[2].matched) {
                    char suffix = std::toupper(match[2].str()[0]);
                    switch (suffix) {
                        case 'K': return static_cast<int64_t>(base * 1000);
                        case 'M': return static_cast<int64_t>(base * 1000000);
                        case 'B': return static_cast<int64_t>(base * 1000000000);
                        case 'T': return static_cast<int64_t>(base * 1000000000000LL);
                        default: return static_cast<int64_t>(base);
                    }
                }

                return static_cast<int64_t>(base);
            } catch (...) {
                return std::nullopt;
            }
        } else {
            // Integer with suffix
            auto base = str_to_int(num_str);
            if (!base) {
                return std::nullopt;
            }

            if (match[2].matched) {
                char suffix = std::toupper(match[2].str()[0]);
                switch (suffix) {
                    case 'K': return *base * 1000;
                    case 'M': return *base * 1000000;
                    case 'B': return *base * 1000000000;
                    case 'T': return *base * 1000000000000LL;
                    default: return base;
                }
            }

            return base;
        }
    }

    // Try parsing as simple integer with thousand separators
    return str_to_int(str);
}

// ============================================================================
// Resolution Parsing
// ============================================================================

std::pair<std::optional<int>, std::optional<int>> parse_resolution(
    std::string_view resolution
) {
    if (resolution.empty()) {
        return {std::nullopt, std::nullopt};
    }

    std::string str(resolution);

    // Try WxH format (e.g., "1920x1080")
    std::regex wxh_regex(R"(([0-9]+)[xX×]([0-9]+))", std::regex::optimize);
    std::smatch match;

    if (std::regex_search(str, match, wxh_regex)) {
        int width = std::stoi(match[1].str());
        int height = std::stoi(match[2].str());
        return {width, height};
    }

    // Try Hp format (e.g., "1080p", "720p60")
    std::regex p_regex(R"(([0-9]+)[pP])", std::regex::optimize);

    if (std::regex_search(str, match, p_regex)) {
        int height = std::stoi(match[1].str());
        return {std::nullopt, height};
    }

    return {std::nullopt, std::nullopt};
}

// ============================================================================
// Bitrate Parsing
// ============================================================================

std::optional<int64_t> parse_bitrate(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str = strip(s);
    if (str.empty()) {
        return std::nullopt;
    }

    // Remove "bps" suffix if present
    std::regex bps_regex(R"(([0-9.]+)\s*([KMG])?(?:bps)?)", std::regex::icase | std::regex::optimize);
    std::smatch match;

    if (std::regex_match(str, match, bps_regex)) {
        double value = std::stod(match[1].str());

        if (match[2].matched) {
            char suffix = std::toupper(match[2].str()[0]);
            switch (suffix) {
                case 'K': value *= 1000; break;
                case 'M': value *= 1000000; break;
                case 'G': value *= 1000000000; break;
            }
        }

        return static_cast<int64_t>(value);
    }

    return std::nullopt;
}

// ============================================================================
// Age Limit Parsing
// ============================================================================

std::optional<int> parse_age_limit(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str(s);

    // Try extracting number
    std::regex age_regex(R"((?:PG-?)?([0-9]+)\+?)", std::regex::icase | std::regex::optimize);
    std::smatch match;

    if (std::regex_search(str, match, age_regex)) {
        return std::stoi(match[1].str());
    }

    return std::nullopt;
}

// ============================================================================
// Helper Functions
// ============================================================================

bool is_number(std::string_view s) {
    if (s.empty()) {
        return false;
    }

    std::regex num_regex(R"(^[+-]?[0-9]*\.?[0-9]+(?:[eE][+-]?[0-9]+)?$)", std::regex::optimize);
    std::string str(s);
    return std::regex_match(str, num_regex);
}

bool is_integer(std::string_view s) {
    if (s.empty()) {
        return false;
    }

    std::regex int_regex(R"(^[+-]?[0-9]+$)", std::regex::optimize);
    std::string str(s);
    return std::regex_match(str, int_regex);
}

} // namespace ytdlp::utils
