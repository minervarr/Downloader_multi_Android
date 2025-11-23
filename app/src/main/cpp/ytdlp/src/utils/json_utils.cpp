/**
 * JSON Utilities Implementation
 */

#include "ytdlp/utils/json_utils.hpp"
#include "ytdlp/utils/filesystem_utils.hpp"

#include <iostream>
#include <fstream>

namespace ytdlp::utils {

// ============================================================================
// JSON File I/O
// ============================================================================

std::optional<json> read_json_file(std::string_view path) {
    auto content = read_file(path);
    if (!content) {
        return std::nullopt;
    }

    return parse_json(*content);
}

bool write_json_file(const json& obj, std::string_view path, int indent) {
    std::string json_str = to_string(obj, indent);
    return write_file(path, json_str);
}

std::optional<json> parse_json(std::string_view str) {
    try {
        return json::parse(str);
    } catch (const json::parse_error&) {
        return std::nullopt;
    }
}

// ============================================================================
// Safe JSON Access
// ============================================================================

std::string get_string(const json& j, std::string_view key, std::string_view default_value) {
    std::string k(key);
    if (j.contains(k) && j[k].is_string()) {
        return j[k].get<std::string>();
    }
    return std::string(default_value);
}

int get_int(const json& j, std::string_view key, int default_value) {
    std::string k(key);
    if (j.contains(k) && j[k].is_number()) {
        return j[k].get<int>();
    }
    return default_value;
}

int64_t get_int64(const json& j, std::string_view key, int64_t default_value) {
    std::string k(key);
    if (j.contains(k) && j[k].is_number()) {
        return j[k].get<int64_t>();
    }
    return default_value;
}

double get_double(const json& j, std::string_view key, double default_value) {
    std::string k(key);
    if (j.contains(k) && j[k].is_number()) {
        return j[k].get<double>();
    }
    return default_value;
}

bool get_bool(const json& j, std::string_view key, bool default_value) {
    std::string k(key);
    if (j.contains(k) && j[k].is_boolean()) {
        return j[k].get<bool>();
    }
    return default_value;
}

json get_array(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_array()) {
        return j[k];
    }
    return json::array();
}

json get_object(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_object()) {
        return j[k];
    }
    return json::object();
}

// ============================================================================
// Optional JSON Access
// ============================================================================

std::optional<std::string> get_string_opt(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_string()) {
        return j[k].get<std::string>();
    }
    return std::nullopt;
}

std::optional<int> get_int_opt(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_number()) {
        return j[k].get<int>();
    }
    return std::nullopt;
}

std::optional<int64_t> get_int64_opt(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_number()) {
        return j[k].get<int64_t>();
    }
    return std::nullopt;
}

std::optional<double> get_double_opt(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_number()) {
        return j[k].get<double>();
    }
    return std::nullopt;
}

std::optional<bool> get_bool_opt(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_boolean()) {
        return j[k].get<bool>();
    }
    return std::nullopt;
}

// ============================================================================
// JSON Path Navigation
// ============================================================================

json get_at_path(const json& j, std::string_view path) {
    try {
        json::json_pointer ptr{std::string{path}};
        return j.at(ptr);
    } catch (...) {
        return json();
    }
}

bool has_path(const json& j, std::string_view path) {
    try {
        json::json_pointer ptr{std::string{path}};
        return j.contains(ptr);
    } catch (...) {
        return false;
    }
}

bool set_at_path(json& j, std::string_view path, const json& value) {
    try {
        json::json_pointer ptr{std::string{path}};
        j[ptr] = value;
        return true;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// Type Checking
// ============================================================================

bool is_string(const json& j, std::string_view key) {
    std::string k(key);
    return j.contains(k) && j[k].is_string();
}

bool is_number(const json& j, std::string_view key) {
    std::string k(key);
    return j.contains(k) && j[k].is_number();
}

bool is_bool(const json& j, std::string_view key) {
    std::string k(key);
    return j.contains(k) && j[k].is_boolean();
}

bool is_array(const json& j, std::string_view key) {
    std::string k(key);
    return j.contains(k) && j[k].is_array();
}

bool is_object(const json& j, std::string_view key) {
    std::string k(key);
    return j.contains(k) && j[k].is_object();
}

bool is_null(const json& j, std::string_view key) {
    std::string k(key);
    return j.contains(k) && j[k].is_null();
}

// ============================================================================
// Array Helpers
// ============================================================================

std::vector<std::string> to_string_vector(const json& j) {
    std::vector<std::string> result;

    if (!j.is_array()) {
        return result;
    }

    for (const auto& item : j) {
        if (item.is_string()) {
            result.push_back(item.get<std::string>());
        }
    }

    return result;
}

std::vector<int> to_int_vector(const json& j) {
    std::vector<int> result;

    if (!j.is_array()) {
        return result;
    }

    for (const auto& item : j) {
        if (item.is_number()) {
            result.push_back(item.get<int>());
        }
    }

    return result;
}

size_t get_array_size(const json& j, std::string_view key) {
    std::string k(key);
    if (j.contains(k) && j[k].is_array()) {
        return j[k].size();
    }
    return 0;
}

// ============================================================================
// JSON Merging
// ============================================================================

json merge(const json& base, const json& overlay) {
    json result = base;

    if (!base.is_object() || !overlay.is_object()) {
        return overlay;
    }

    for (auto it = overlay.begin(); it != overlay.end(); ++it) {
        if (result.contains(it.key()) &&
            result[it.key()].is_object() &&
            it.value().is_object()) {
            // Recursively merge objects
            result[it.key()] = merge(result[it.key()], it.value());
        } else {
            // Overwrite or add
            result[it.key()] = it.value();
        }
    }

    return result;
}

json& update(json& target, const json& source) {
    if (!target.is_object() || !source.is_object()) {
        return target;
    }

    for (auto it = source.begin(); it != source.end(); ++it) {
        target[it.key()] = it.value();
    }

    return target;
}

// ============================================================================
// JSON Formatting
// ============================================================================

std::string to_string(const json& j, int indent) {
    if (indent < 0) {
        return j.dump();
    }
    return j.dump(indent);
}

std::string to_compact_string(const json& j) {
    return j.dump();
}

void print_json(const json& j, int indent) {
    std::cout << to_string(j, indent) << std::endl;
}

// ============================================================================
// Validation
// ============================================================================

bool has_required_keys(const json& j, const std::vector<std::string>& required_keys) {
    if (!j.is_object()) {
        return false;
    }

    for (const auto& key : required_keys) {
        if (!j.contains(key)) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> get_missing_keys(
    const json& j,
    const std::vector<std::string>& required_keys
) {
    std::vector<std::string> missing;

    if (!j.is_object()) {
        return required_keys;
    }

    for (const auto& key : required_keys) {
        if (!j.contains(key)) {
            missing.push_back(key);
        }
    }

    return missing;
}

bool validate_structure(
    const json& j,
    const std::map<std::string, json::value_t>& schema_keys
) {
    if (!j.is_object()) {
        return false;
    }

    for (const auto& [key, expected_type] : schema_keys) {
        if (!j.contains(key)) {
            return false;
        }

        if (j[key].type() != expected_type) {
            return false;
        }
    }

    return true;
}

} // namespace ytdlp::utils
