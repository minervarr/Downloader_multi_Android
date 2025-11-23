/**
 * JSON Utilities Header
 *
 * Provides safe JSON access utilities wrapping nlohmann/json with Python-like API.
 * Includes file I/O, safe access, path navigation, validation, and merging.
 */

#ifndef YTDLP_UTILS_JSON_UTILS_HPP
#define YTDLP_UTILS_JSON_UTILS_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>

namespace ytdlp::utils {

// Type alias for convenience
using json = nlohmann::json;

// ============================================================================
// JSON File I/O
// ============================================================================

/**
 * Read and parse JSON file
 * @param path Path to JSON file
 * @return Parsed JSON object or nullopt if file cannot be read/parsed
 */
std::optional<json> read_json_file(std::string_view path);

/**
 * Write JSON object to file
 * @param obj JSON object to write
 * @param path Output file path
 * @param indent Indentation level (-1 for compact)
 * @return True if successful
 */
bool write_json_file(const json& obj, std::string_view path, int indent = 2);

/**
 * Parse JSON from string
 * @param str JSON string
 * @return Parsed JSON object or nullopt if invalid
 */
std::optional<json> parse_json(std::string_view str);

// ============================================================================
// Safe JSON Access (with defaults)
// ============================================================================

/**
 * Safely get string value from JSON object
 * @param j JSON object
 * @param key Key to look up
 * @param default_value Default value if key missing or not a string
 * @return String value or default
 */
std::string get_string(const json& j, std::string_view key, std::string_view default_value = "");

/**
 * Safely get integer value from JSON object
 * @param j JSON object
 * @param key Key to look up
 * @param default_value Default value if key missing or not a number
 * @return Integer value or default
 */
int get_int(const json& j, std::string_view key, int default_value = 0);

/**
 * Safely get 64-bit integer value from JSON object
 * @param j JSON object
 * @param key Key to look up
 * @param default_value Default value if key missing or not a number
 * @return 64-bit integer value or default
 */
int64_t get_int64(const json& j, std::string_view key, int64_t default_value = 0);

/**
 * Safely get double value from JSON object
 * @param j JSON object
 * @param key Key to look up
 * @param default_value Default value if key missing or not a number
 * @return Double value or default
 */
double get_double(const json& j, std::string_view key, double default_value = 0.0);

/**
 * Safely get boolean value from JSON object
 * @param j JSON object
 * @param key Key to look up
 * @param default_value Default value if key missing or not a boolean
 * @return Boolean value or default
 */
bool get_bool(const json& j, std::string_view key, bool default_value = false);

/**
 * Safely get array from JSON object
 * @param j JSON object
 * @param key Key to look up
 * @return Array or empty array if key missing or not an array
 */
json get_array(const json& j, std::string_view key);

/**
 * Safely get object from JSON object
 * @param j JSON object
 * @param key Key to look up
 * @return Object or empty object if key missing or not an object
 */
json get_object(const json& j, std::string_view key);

// ============================================================================
// Optional JSON Access (returns std::optional)
// ============================================================================

/**
 * Get string value as optional
 * @param j JSON object
 * @param key Key to look up
 * @return String value or nullopt if key missing or not a string
 */
std::optional<std::string> get_string_opt(const json& j, std::string_view key);

/**
 * Get integer value as optional
 * @param j JSON object
 * @param key Key to look up
 * @return Integer value or nullopt if key missing or not a number
 */
std::optional<int> get_int_opt(const json& j, std::string_view key);

/**
 * Get 64-bit integer value as optional
 * @param j JSON object
 * @param key Key to look up
 * @return 64-bit integer value or nullopt if key missing or not a number
 */
std::optional<int64_t> get_int64_opt(const json& j, std::string_view key);

/**
 * Get double value as optional
 * @param j JSON object
 * @param key Key to look up
 * @return Double value or nullopt if key missing or not a number
 */
std::optional<double> get_double_opt(const json& j, std::string_view key);

/**
 * Get boolean value as optional
 * @param j JSON object
 * @param key Key to look up
 * @return Boolean value or nullopt if key missing or not a boolean
 */
std::optional<bool> get_bool_opt(const json& j, std::string_view key);

// ============================================================================
// JSON Path Navigation (using JSON Pointer RFC 6901)
// ============================================================================

/**
 * Get value at JSON pointer path (e.g., "/result/redirectUrl")
 * @param j JSON object
 * @param path JSON pointer path
 * @return Value at path or empty JSON if not found
 */
json get_at_path(const json& j, std::string_view path);

/**
 * Get string at JSON pointer path
 * @param j JSON object
 * @param path JSON pointer path
 * @param default_value Default value if path not found or not a string
 * @return String value or default
 */
inline std::string get_string_at_path(const json& j, std::string_view path, std::string_view default_value = "") {
    auto val = get_at_path(j, path);
    return val.is_string() ? val.get<std::string>() : std::string(default_value);
}

/**
 * Check if JSON pointer path exists
 * @param j JSON object
 * @param path JSON pointer path
 * @return True if path exists
 */
bool has_path(const json& j, std::string_view path);

/**
 * Set value at JSON pointer path
 * @param j JSON object to modify
 * @param path JSON pointer path
 * @param value Value to set
 * @return True if successful
 */
bool set_at_path(json& j, std::string_view path, const json& value);

// ============================================================================
// Type Checking
// ============================================================================

/**
 * Check if key exists and is a string
 */
bool is_string(const json& j, std::string_view key);

/**
 * Check if key exists and is a number
 */
bool is_number(const json& j, std::string_view key);

/**
 * Check if key exists and is a boolean
 */
bool is_bool(const json& j, std::string_view key);

/**
 * Check if key exists and is an array
 */
bool is_array(const json& j, std::string_view key);

/**
 * Check if key exists and is an object
 */
bool is_object(const json& j, std::string_view key);

/**
 * Check if key exists and is null
 */
bool is_null(const json& j, std::string_view key);

// ============================================================================
// Array Helpers
// ============================================================================

/**
 * Convert JSON array to vector of strings
 * @param j JSON array
 * @return Vector of strings (non-string elements skipped)
 */
std::vector<std::string> to_string_vector(const json& j);

/**
 * Convert JSON array to vector of integers
 * @param j JSON array
 * @return Vector of integers (non-number elements skipped)
 */
std::vector<int> to_int_vector(const json& j);

/**
 * Get size of array at key
 * @param j JSON object
 * @param key Key to array
 * @return Size of array or 0 if not an array
 */
size_t get_array_size(const json& j, std::string_view key);

// ============================================================================
// JSON Merging
// ============================================================================

/**
 * Recursively merge two JSON objects
 * @param base Base JSON object
 * @param overlay Overlay JSON object (takes precedence)
 * @return Merged JSON object
 */
json merge(const json& base, const json& overlay);

/**
 * Shallow update of JSON object (non-recursive)
 * @param target Target JSON object (modified in place)
 * @param source Source JSON object
 * @return Reference to target
 */
json& update(json& target, const json& source);

// ============================================================================
// JSON Formatting
// ============================================================================

/**
 * Convert JSON to string
 * @param j JSON object
 * @param indent Indentation level (-1 for compact)
 * @return JSON string
 */
std::string to_string(const json& j, int indent = 2);

/**
 * Convert JSON to compact string (no formatting)
 * @param j JSON object
 * @return Compact JSON string
 */
std::string to_compact_string(const json& j);

/**
 * Print JSON to stdout
 * @param j JSON object
 * @param indent Indentation level
 */
void print_json(const json& j, int indent = 2);

// ============================================================================
// Validation
// ============================================================================

/**
 * Check if JSON object has all required keys
 * @param j JSON object
 * @param required_keys List of required key names
 * @return True if all keys present
 */
bool has_required_keys(const json& j, const std::vector<std::string>& required_keys);

/**
 * Get list of missing required keys
 * @param j JSON object
 * @param required_keys List of required key names
 * @return Vector of missing key names (empty if all present)
 */
std::vector<std::string> get_missing_keys(
    const json& j,
    const std::vector<std::string>& required_keys
);

/**
 * Validate JSON object structure against schema
 * @param j JSON object
 * @param schema_keys Map of key names to expected types
 * @return True if structure matches schema
 */
bool validate_structure(
    const json& j,
    const std::map<std::string, json::value_t>& schema_keys
);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_JSON_UTILS_HPP
