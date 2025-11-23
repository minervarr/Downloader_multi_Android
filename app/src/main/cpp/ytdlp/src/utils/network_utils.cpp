#include "ytdlp/utils/network_utils.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <regex>
#include <sstream>
#include <random>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <vector>

namespace ytdlp::utils {

namespace user_agents {
    const char* CHROME_WINDOWS = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36";
    const char* CHROME_MAC = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36";
    const char* CHROME_LINUX = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36";
    const char* FIREFOX_WINDOWS = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:133.0) Gecko/20100101 Firefox/133.0";
    const char* SAFARI_MAC = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.6 Safari/605.1.15";
    const char* EDGE_WINDOWS = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36 Edg/137.0.0.0";
}

std::string random_user_agent() {
    // Target Chrome versions released within the last ~6 months
    const int MIN_VERSION = 134;
    const int MAX_VERSION = 140;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(MIN_VERSION, MAX_VERSION);

    int version = dis(gen);
    return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/" +
           std::to_string(version) + ".0.0.0 Safari/537.36";
}

std::map<std::string, std::string> std_headers() {
    return {
        {"User-Agent", random_user_agent()},
        {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
        {"Accept-Language", "en-us,en;q=0.5"},
        {"Sec-Fetch-Mode", "navigate"}
    };
}

std::map<std::string, std::string> parse_url(std::string_view url) {
    std::map<std::string, std::string> result;
    std::string url_str(url);

    // Parse scheme
    size_t scheme_end = url_str.find("://");
    if (scheme_end != std::string::npos) {
        result["scheme"] = url_str.substr(0, scheme_end);
        url_str = url_str.substr(scheme_end + 3);
    } else {
        result["scheme"] = "";
    }

    // Parse fragment
    size_t fragment_start = url_str.find('#');
    if (fragment_start != std::string::npos) {
        result["fragment"] = url_str.substr(fragment_start + 1);
        url_str = url_str.substr(0, fragment_start);
    } else {
        result["fragment"] = "";
    }

    // Parse query
    size_t query_start = url_str.find('?');
    if (query_start != std::string::npos) {
        result["query"] = url_str.substr(query_start + 1);
        url_str = url_str.substr(0, query_start);
    } else {
        result["query"] = "";
    }

    // Parse path and params (params use semicolon)
    size_t path_start = url_str.find('/');
    if (path_start != std::string::npos) {
        std::string path_and_params = url_str.substr(path_start);
        result["netloc"] = url_str.substr(0, path_start);

        size_t params_start = path_and_params.find(';');
        if (params_start != std::string::npos) {
            result["params"] = path_and_params.substr(params_start + 1);
            result["path"] = path_and_params.substr(0, params_start);
        } else {
            result["params"] = "";
            result["path"] = path_and_params;
        }
    } else {
        result["netloc"] = url_str;
        result["path"] = "";
        result["params"] = "";
    }

    return result;
}

std::string build_url(const std::map<std::string, std::string>& components) {
    std::ostringstream oss;

    // Scheme
    auto it = components.find("scheme");
    if (it != components.end() && !it->second.empty()) {
        oss << it->second << "://";
    }

    // Netloc
    it = components.find("netloc");
    if (it != components.end()) {
        oss << it->second;
    }

    // Path
    it = components.find("path");
    if (it != components.end() && !it->second.empty()) {
        if (it->second[0] != '/' && oss.tellp() > 0) {
            oss << '/';
        }
        oss << it->second;
    }

    // Params
    it = components.find("params");
    if (it != components.end() && !it->second.empty()) {
        oss << ';' << it->second;
    }

    // Query
    it = components.find("query");
    if (it != components.end() && !it->second.empty()) {
        oss << '?' << it->second;
    }

    // Fragment
    it = components.find("fragment");
    if (it != components.end() && !it->second.empty()) {
        oss << '#' << it->second;
    }

    return oss.str();
}

std::string remove_dot_segments(std::string_view path) {
    std::vector<std::string> output;
    std::vector<std::string> segments;

    // Split path by '/'
    std::string path_str(path);
    size_t start = 0;
    size_t end = path_str.find('/');

    while (end != std::string::npos) {
        segments.push_back(path_str.substr(start, end - start));
        start = end + 1;
        end = path_str.find('/', start);
    }
    segments.push_back(path_str.substr(start));

    // Process segments
    for (const auto& s : segments) {
        if (s == ".") {
            continue;  // Skip current directory markers
        } else if (s == "..") {
            if (!output.empty()) {
                output.pop_back();  // Go up one directory
            }
        } else {
            output.push_back(s);
        }
    }

    // Reconstruct path
    if (segments.size() > 0 && segments[0].empty() && (!output.empty() && !output[0].empty())) {
        output.insert(output.begin(), "");
    }

    if (segments.size() > 0 && (segments.back() == "." || segments.back() == "..")) {
        output.push_back("");
    }

    std::ostringstream result;
    for (size_t i = 0; i < output.size(); ++i) {
        if (i > 0) result << '/';
        result << output[i];
    }

    return result.str();
}

std::string escape_rfc3986(std::string_view s) {
    // RFC 3986 reserved characters that should NOT be encoded in URLs
    const std::string reserved = "%/;:@&=+$,!~*'()?#[]";

    std::ostringstream oss;
    for (unsigned char c : s) {
        if (std::isalnum(c) || reserved.find(c) != std::string::npos) {
            oss << c;
        } else {
            // Percent-encode
            oss << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
    }

    return oss.str();
}

std::string normalize_url(std::string_view url) {
    auto components = parse_url(url);

    // Normalize path by removing dot segments
    if (!components["path"].empty()) {
        components["path"] = remove_dot_segments(components["path"]);
    }

    // Escape non-ASCII in path, params, query, fragment
    if (!components["path"].empty()) {
        components["path"] = escape_rfc3986(components["path"]);
    }
    if (!components["params"].empty()) {
        components["params"] = escape_rfc3986(components["params"]);
    }
    if (!components["query"].empty()) {
        components["query"] = escape_rfc3986(components["query"]);
    }
    if (!components["fragment"].empty()) {
        components["fragment"] = escape_rfc3986(components["fragment"]);
    }

    // TODO: IDNA encoding for netloc (requires libidn or similar)
    // For now, just lowercase the netloc
    if (!components["netloc"].empty()) {
        std::transform(components["netloc"].begin(), components["netloc"].end(),
                      components["netloc"].begin(), ::tolower);
    }

    return build_url(components);
}

std::map<std::string, std::string> extract_query_params(std::string_view url) {
    std::map<std::string, std::string> params;
    auto components = parse_url(url);
    std::string query = components["query"];

    if (query.empty()) {
        return params;
    }

    // Split by '&'
    size_t start = 0;
    size_t end = query.find('&');

    while (end != std::string::npos) {
        std::string param = query.substr(start, end - start);
        size_t eq = param.find('=');
        if (eq != std::string::npos) {
            std::string key = param.substr(0, eq);
            std::string value = param.substr(eq + 1);
            params[url_decode(key)] = url_decode(value);
        }
        start = end + 1;
        end = query.find('&', start);
    }

    // Last parameter
    std::string param = query.substr(start);
    size_t eq = param.find('=');
    if (eq != std::string::npos) {
        std::string key = param.substr(0, eq);
        std::string value = param.substr(eq + 1);
        params[url_decode(key)] = url_decode(value);
    }

    return params;
}

std::string update_url_query(std::string_view url,
                               const std::map<std::string, std::string>& query_update) {
    auto components = parse_url(url);
    auto existing_params = extract_query_params(url);

    // Merge parameters
    for (const auto& [key, value] : query_update) {
        existing_params[key] = value;
    }

    // Build new query string
    std::ostringstream query_oss;
    bool first = true;
    for (const auto& [key, value] : existing_params) {
        if (!first) query_oss << '&';
        first = false;
        query_oss << url_encode(key) << '=' << url_encode(value);
    }

    components["query"] = query_oss.str();
    return build_url(components);
}

std::string update_url(std::string_view url,
                        std::optional<std::string> scheme,
                        std::optional<std::string> netloc,
                        std::optional<std::string> path,
                        std::optional<std::string> query,
                        std::optional<std::string> fragment) {
    auto components = parse_url(url);

    if (scheme.has_value()) components["scheme"] = scheme.value();
    if (netloc.has_value()) components["netloc"] = netloc.value();
    if (path.has_value()) components["path"] = path.value();
    if (query.has_value()) components["query"] = query.value();
    if (fragment.has_value()) components["fragment"] = fragment.value();

    return build_url(components);
}

void clean_headers(std::map<std::string, std::string>& headers) {
    // Remove yt-dlp specific headers
    auto it = headers.find("Youtubedl-No-Compression");
    if (it != headers.end()) {
        headers.erase(it);
        headers["Accept-Encoding"] = "identity";
    }

    headers.erase("Ytdl-socks-proxy");
}

void clean_proxies(std::map<std::string, std::string>& proxies,
                    std::map<std::string, std::string>& headers) {
    // Check for Ytdl-Request-Proxy header
    auto it = headers.find("Ytdl-Request-Proxy");
    if (it != headers.end()) {
        proxies.clear();
        proxies["all"] = it->second;
        headers.erase(it);
        return;
    }

    // Clean up proxy values
    for (auto& [key, value] : proxies) {
        if (value == "__noproxy__") {
            value = "";  // Clear proxy
            continue;
        }

        if (key == "no") {
            continue;  // Special case for NO_PROXY
        }

        if (value.empty()) {
            continue;
        }

        // Add http:// scheme if missing
        if (value.find("://") == std::string::npos) {
            // Remove leading slashes
            size_t start = value.find_first_not_of('/');
            if (start != std::string::npos) {
                value = "http://" + value.substr(start);
            } else {
                value = "http://" + value;
            }
        }

        // Normalize socks schemes
        if (value.find("socks5://") == 0) {
            value = "socks5h://" + value.substr(9);  // socks5 -> socks5h (DNS resolution through proxy)
        } else if (value.find("socks://") == 0) {
            value = "socks4://" + value.substr(8);  // socks -> socks4
        }
    }
}

std::string select_proxy(std::string_view url,
                          const std::map<std::string, std::string>& proxies) {
    auto components = parse_url(url);
    std::string scheme = components["scheme"];

    if (scheme.empty()) {
        scheme = "http";
    }

    // Check for scheme-specific proxy
    auto it = proxies.find(scheme);
    if (it != proxies.end() && !it->second.empty()) {
        return it->second;
    }

    // Check for 'all' proxy
    it = proxies.find("all");
    if (it != proxies.end() && !it->second.empty()) {
        return it->second;
    }

    return "";  // No proxy
}

std::string get_hostname(std::string_view url) {
    auto components = parse_url(url);
    std::string netloc = components["netloc"];

    // Remove port if present
    size_t colon = netloc.find(':');
    if (colon != std::string::npos) {
        return netloc.substr(0, colon);
    }

    return netloc;
}

int get_port(std::string_view url) {
    auto components = parse_url(url);
    std::string netloc = components["netloc"];

    size_t colon = netloc.find(':');
    if (colon != std::string::npos) {
        try {
            return std::stoi(netloc.substr(colon + 1));
        } catch (...) {
            return 0;
        }
    }

    // Default ports
    if (components["scheme"] == "https") return 443;
    if (components["scheme"] == "http") return 80;

    return 0;
}

std::string get_scheme(std::string_view url) {
    auto components = parse_url(url);
    return components["scheme"];
}

bool is_https(std::string_view url) {
    return get_scheme(url) == "https";
}

std::string to_https(std::string_view url) {
    if (is_https(url)) {
        return std::string(url);
    }

    return update_url(url, "https");
}

std::string url_join(std::string_view base, std::string_view path) {
    // If path is absolute URL, return it
    std::string path_str(path);
    if (path_str.find("://") != std::string::npos) {
        return path_str;
    }

    auto base_components = parse_url(base);

    // If path starts with /, it's absolute path
    if (!path_str.empty() && path_str[0] == '/') {
        base_components["path"] = path_str;
        base_components["query"] = "";
        base_components["fragment"] = "";
        return build_url(base_components);
    }

    // Relative path - join with base path
    std::string base_path = base_components["path"];

    // Remove everything after last /
    size_t last_slash = base_path.find_last_of('/');
    if (last_slash != std::string::npos) {
        base_path = base_path.substr(0, last_slash + 1);
    } else {
        base_path = "/";
    }

    base_components["path"] = remove_dot_segments(base_path + path_str);
    base_components["query"] = "";
    base_components["fragment"] = "";

    return build_url(base_components);
}

} // namespace ytdlp::utils
