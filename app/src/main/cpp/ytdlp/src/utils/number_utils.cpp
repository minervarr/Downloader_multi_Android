#include "ytdlp/utils/number_utils.hpp"
#include <regex>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace ytdlp::utils {

namespace {

// Helper function to convert string to lowercase
std::string to_lower(std::string_view s) {
    std::string result(s);
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Helper function to lookup value from unit table
std::optional<int64_t> lookup_unit_table(
    const std::map<std::string, int64_t>& unit_table,
    std::string_view s,
    bool strict = false) {

    // Build regex for numeric part
    std::string num_pattern = strict ? R"(\d+(?:\.\d+)?)" : R"(\d+(?:[,.]\d+)?)";

    // Build regex for units
    std::string units_pattern = "(";
    bool first = true;
    for (const auto& [unit, _] : unit_table) {
        if (!first) units_pattern += "|";
        first = false;
        // Escape special regex characters
        std::string escaped_unit;
        for (char c : unit) {
            if (c == '.' || c == '+' || c == '*' || c == '?' || c == '^' ||
                c == '$' || c == '(' || c == ')' || c == '[' || c == ']' ||
                c == '{' || c == '}' || c == '|' || c == '\\') {
                escaped_unit += '\\';
            }
            escaped_unit += c;
        }
        units_pattern += escaped_unit;
    }
    units_pattern += ")";

    // Create full regex pattern
    std::string pattern = "^(" + num_pattern + ")\\s*" + units_pattern + "\\b";
    std::regex re(pattern, std::regex::icase);
    std::smatch match;
    std::string str(s);

    if (!std::regex_search(str, match, re)) {
        return std::nullopt;
    }

    // Parse the number
    std::string num_str = match[1].str();
    std::replace(num_str.begin(), num_str.end(), ',', '.');
    double num;
    try {
        num = std::stod(num_str);
    } catch (...) {
        return std::nullopt;
    }

    // Find the unit multiplier
    std::string unit_str = match[2].str();
    for (const auto& [unit, mult] : unit_table) {
        if (to_lower(unit) == to_lower(unit_str)) {
            return static_cast<int64_t>(std::round(num * mult));
        }
    }

    return std::nullopt;
}

} // anonymous namespace

std::optional<int64_t> parse_filesize(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    // Try parsing as plain number first
    std::regex plain_number(R"(^\d+$)");
    if (std::regex_match(std::string(s), plain_number)) {
        try {
            return std::stoll(std::string(s));
        } catch (...) {
            return std::nullopt;
        }
    }

    // File size unit table (both decimal and binary units)
    static const std::map<std::string, int64_t> FILESIZE_UNITS = {
        // Bytes
        {"B", 1},
        {"b", 1},
        {"bytes", 1},
        // Kilobytes (decimal vs binary)
        {"KiB", 1024},
        {"KB", 1000},
        {"kB", 1024},
        {"Kb", 1000},
        {"kb", 1000},
        {"kilobytes", 1000},
        {"kibibytes", 1024},
        // Megabytes
        {"MiB", 1024LL * 1024},
        {"MB", 1000LL * 1000},
        {"mB", 1024LL * 1024},
        {"Mb", 1000LL * 1000},
        {"mb", 1000LL * 1000},
        {"megabytes", 1000LL * 1000},
        {"mebibytes", 1024LL * 1024},
        // Gigabytes
        {"GiB", 1024LL * 1024 * 1024},
        {"GB", 1000LL * 1000 * 1000},
        {"gB", 1024LL * 1024 * 1024},
        {"Gb", 1000LL * 1000 * 1000},
        {"gb", 1000LL * 1000 * 1000},
        {"gigabytes", 1000LL * 1000 * 1000},
        {"gibibytes", 1024LL * 1024 * 1024},
        // Terabytes
        {"TiB", 1024LL * 1024 * 1024 * 1024},
        {"TB", 1000LL * 1000 * 1000 * 1000},
        {"tB", 1024LL * 1024 * 1024 * 1024},
        {"Tb", 1000LL * 1000 * 1000 * 1000},
        {"tb", 1000LL * 1000 * 1000 * 1000},
        {"terabytes", 1000LL * 1000 * 1000 * 1000},
        {"tebibytes", 1024LL * 1024 * 1024 * 1024},
        // Petabytes
        {"PiB", 1024LL * 1024 * 1024 * 1024 * 1024},
        {"PB", 1000LL * 1000 * 1000 * 1000 * 1000},
        {"pB", 1024LL * 1024 * 1024 * 1024 * 1024},
        {"Pb", 1000LL * 1000 * 1000 * 1000 * 1000},
        {"pb", 1000LL * 1000 * 1000 * 1000 * 1000},
        {"petabytes", 1000LL * 1000 * 1000 * 1000 * 1000},
        {"pebibytes", 1024LL * 1024 * 1024 * 1024 * 1024},
    };

    return lookup_unit_table(FILESIZE_UNITS, s);
}

std::optional<int64_t> parse_count(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str(s);

    // Remove leading non-digit characters followed by space
    std::regex leading_non_digit(R"(^[^\d]+\s)");
    str = std::regex_replace(str, leading_non_digit, "");

    // Trim whitespace
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);

    if (str.empty()) {
        return std::nullopt;
    }

    // Check if it's just a number with optional commas/periods
    std::regex just_number(R"(^[\d,.]+$)");
    if (std::regex_match(str, just_number)) {
        return str_to_int(str);
    }

    // Count unit table
    static const std::map<std::string, int64_t> COUNT_UNITS = {
        {"k", 1000},
        {"K", 1000},
        {"m", 1000 * 1000},
        {"M", 1000 * 1000},
        {"kk", 1000 * 1000},
        {"KK", 1000 * 1000},
        {"b", 1000 * 1000 * 1000},
        {"B", 1000 * 1000 * 1000},
    };

    auto result = lookup_unit_table(COUNT_UNITS, str);
    if (result.has_value()) {
        return result;
    }

    // Try extracting just the leading number
    std::regex leading_number(R"(([\d,.]+)(?:$|\s))");
    std::smatch match;
    if (std::regex_search(str, match, leading_number)) {
        return str_to_int(match[1].str());
    }

    return std::nullopt;
}

std::optional<double> parse_duration(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str(s);

    // Trim whitespace
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);

    if (str.empty()) {
        return std::nullopt;
    }

    // Try colon-separated format: [[HH:]MM:]SS[.ms]
    std::regex colon_format(
        R"(^(?:(?:(?:(\d+):)?(\d+):)?(\d+):)?(\d{1,2}|[0-9]+)([.:]\d+)?Z?$)"
    );
    std::smatch match;

    if (std::regex_match(str, match, colon_format)) {
        double days = match[1].matched ? std::stod(match[1].str()) : 0;
        double hours = match[2].matched ? std::stod(match[2].str()) : 0;
        double mins = match[3].matched ? std::stod(match[3].str()) : 0;
        double secs = match[4].matched ? std::stod(match[4].str()) : 0;
        double ms = 0;
        if (match[5].matched) {
            std::string ms_str = match[5].str();
            std::replace(ms_str.begin(), ms_str.end(), ':', '.');
            ms = std::stod(ms_str);
        }

        return days * 86400 + hours * 3600 + mins * 60 + secs + ms;
    }

    // Try ISO 8601 / text format: P?1d 2h 3m 4s
    // Simplified regex to avoid complex nested groups
    std::regex text_format(
        R"((?:P?(?:(?:\d+\s*y(?:ears?)?,?\s*)?(?:\d+\s*m(?:onths?)?,?\s*)?(?:\d+\s*w(?:eeks?)?,?\s*)?))?(?:(\d+)\s*d(?:ays?)?,?\s*)?T?(?:(\d+)\s*h(?:(?:ou)?rs?)?,?\s*)?(?:(\d+)\s*m(?:in(?:ute)?s?)?,?\s*)?(?:(\d+)(\.\d+)?\s*s(?:ec(?:ond)?s?)?\s*)?Z?$)",
        std::regex::icase
    );

    if (std::regex_match(str, match, text_format)) {
        double days = match[1].matched ? std::stod(match[1].str()) : 0;
        double hours = match[2].matched ? std::stod(match[2].str()) : 0;
        double mins = match[3].matched ? std::stod(match[3].str()) : 0;
        double secs = match[4].matched ? std::stod(match[4].str()) : 0;
        double ms = match[5].matched ? std::stod(match[5].str()) : 0;

        return days * 86400 + hours * 3600 + mins * 60 + secs + ms;
    }

    // Try simple "X hours" or "X mins" format
    std::regex simple_format(
        R"((?:(?:(\d+(?:\.\d+)?)\s*(?:hours?))|(?:(\d+(?:\.\d+)?)\s*(?:mins?\.?|minutes?))\s*)Z?$)",
        std::regex::icase
    );

    if (std::regex_match(str, match, simple_format)) {
        double hours = match[1].matched ? std::stod(match[1].str()) : 0;
        double mins = match[2].matched ? std::stod(match[2].str()) : 0;

        return hours * 3600 + mins * 60;
    }

    return std::nullopt;
}

std::string format_bytes(int64_t bytes) {
    if (bytes <= 0) {
        return "N/A";
    }

    const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};
    const int num_units = sizeof(units) / sizeof(units[0]);

    double value = static_cast<double>(bytes);
    int unit_index = 0;

    while (value >= 1024.0 && unit_index < num_units - 1) {
        value /= 1024.0;
        unit_index++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value << units[unit_index];
    return oss.str();
}

std::map<std::string, int> parse_resolution(std::string_view s, bool lenient) {
    std::map<std::string, int> result;

    if (s.empty()) {
        return result;
    }

    std::string str(s);
    std::smatch match;

    // Try WIDTHxHEIGHT format (e.g., "1920x1080", "1280×720")
    // Note: std::regex doesn't support lookbehind/lookahead, so we check boundaries manually
    // Use alternation for × (U+00D7) since std::regex doesn't handle UTF-8 in char classes well
    std::regex width_height_regex(R"((\d+)\s*(?:[xX,]|×)\s*(\d+))");
    if (std::regex_search(str, match, width_height_regex)) {
        // If not lenient, check word boundaries manually
        if (lenient ||
            (match.prefix().length() == 0 || !std::isalnum(static_cast<unsigned char>(match.prefix().str().back()))) &&
            (match.suffix().length() == 0 || !std::isalnum(static_cast<unsigned char>(match.suffix().str().front())))) {
            result["width"] = std::stoi(match[1].str());
            result["height"] = std::stoi(match[2].str());
            return result;
        }
    }

    // Try NNNp/NNNi format (e.g., "1080p", "720P", "1080i")
    std::regex p_format(R"((\d+)[pPiI])");
    if (std::regex_search(str, match, p_format)) {
        // Check word boundaries manually
        if ((match.prefix().length() == 0 || !std::isalnum(static_cast<unsigned char>(match.prefix().str().back()))) &&
            (match.suffix().length() == 0 || !std::isalnum(static_cast<unsigned char>(match.suffix().str().front())))) {
            result["height"] = std::stoi(match[1].str());
            return result;
        }
    }

    // Try 4k/8k format
    std::regex k_format(R"(\b([48])[kK]\b)");
    if (std::regex_search(str, match, k_format)) {
        int k_value = std::stoi(match[1].str());
        result["height"] = k_value * 540;  // 4k = 2160p, 8k = 4320p
        return result;
    }

    return result;
}

std::string format_duration(double seconds) {
    if (seconds < 0) {
        return "0:00";
    }

    int total_secs = static_cast<int>(seconds);
    int hours = total_secs / 3600;
    int mins = (total_secs % 3600) / 60;
    int secs = total_secs % 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << ":"
            << std::setfill('0') << std::setw(2) << mins << ":"
            << std::setfill('0') << std::setw(2) << secs;
    } else {
        oss << mins << ":"
            << std::setfill('0') << std::setw(2) << secs;
    }

    return oss.str();
}

std::optional<int64_t> str_to_int(std::string_view s) {
    if (s.empty()) {
        return std::nullopt;
    }

    std::string str(s);

    // Remove commas and periods used as thousands separators
    str.erase(std::remove(str.begin(), str.end(), ','), str.end());

    // Try to convert to integer
    try {
        size_t pos;
        int64_t value = std::stoll(str, &pos);
        // Check if we consumed the whole string (or just whitespace)
        while (pos < str.length() && std::isspace(str[pos])) {
            pos++;
        }
        if (pos == str.length()) {
            return value;
        }
    } catch (...) {
        // Fall through
    }

    return std::nullopt;
}

} // namespace ytdlp::utils
