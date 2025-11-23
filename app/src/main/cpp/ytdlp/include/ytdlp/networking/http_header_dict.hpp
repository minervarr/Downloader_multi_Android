#ifndef YTDLP_NETWORKING_HTTP_HEADER_DICT_HPP
#define YTDLP_NETWORKING_HTTP_HEADER_DICT_HPP

#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <optional>

namespace ytdlp::networking {

/**
 * HTTP Header Dictionary
 *
 * A case-insensitive dictionary for HTTP headers that preserves the original
 * casing of header names. Internally normalizes header names to Title-Case
 * for case-insensitive lookups while maintaining the original casing for
 * serialization.
 *
 * Features:
 * - Case-insensitive header access (Content-Type, content-type, CONTENT-TYPE all match)
 * - Preserves original header name casing
 * - Automatic whitespace trimming for values
 * - Dictionary-style operations (get, set, remove, update)
 */
class HTTPHeaderDict {
public:
    // Constructors
    HTTPHeaderDict() = default;
    explicit HTTPHeaderDict(const std::map<std::string, std::string>& headers);
    explicit HTTPHeaderDict(const std::vector<std::map<std::string, std::string>>& header_maps);

    // Header access
    void set(std::string_view key, std::string_view value);
    std::optional<std::string> get(std::string_view key) const;
    std::string get(std::string_view key, std::string_view default_value) const;
    bool contains(std::string_view key) const;

    // Header manipulation
    bool remove(std::string_view key);
    std::optional<std::string> pop(std::string_view key);
    std::string pop(std::string_view key, std::string_view default_value);
    std::string setdefault(std::string_view key, std::string_view value);

    // Bulk operations
    void update(const std::map<std::string, std::string>& other);
    void update(const HTTPHeaderDict& other);
    void clear();

    // Size queries
    size_t size() const;
    bool empty() const;

    // Conversion
    std::map<std::string, std::string> to_map() const;
    std::map<std::string, std::string> sensitive() const;  // Returns map with original casing

    // Operators
    std::string operator[](std::string_view key) const;
    HTTPHeaderDict operator|(const HTTPHeaderDict& other) const;
    HTTPHeaderDict operator|(const std::map<std::string, std::string>& other) const;

private:
    // Internal storage (title-case keys)
    std::map<std::string, std::string> headers_;

    // Maps title-case keys to original casing
    std::map<std::string, std::string> sensitive_map_;

    // Helper: Convert header name to Title-Case (e.g., "content-type" -> "Content-Type")
    static std::string to_title_case(std::string_view key);
};

} // namespace ytdlp::networking

#endif // YTDLP_NETWORKING_HTTP_HEADER_DICT_HPP
