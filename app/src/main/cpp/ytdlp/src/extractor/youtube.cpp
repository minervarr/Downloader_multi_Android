#include "ytdlp/extractor/youtube.hpp"
#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include "ytdlp/utils/json_utils.hpp"
#include "ytdlp/utils/filesystem_utils.hpp"
#include "ytdlp/utils/js_interpreter.hpp"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace ytdlp::extractor {

// Simplified YouTube URL patterns
const std::vector<std::regex> YoutubeIE::URL_PATTERNS = {
    // Standard youtube.com/watch?v=ID
    std::regex(R"((?:https?://)?(?:www\.)?youtube\.com/watch\?.*?v=([0-9A-Za-z_-]{11}))", std::regex::icase),
    // youtu.be/ID
    std::regex(R"((?:https?://)?youtu\.be/([0-9A-Za-z_-]{11}))", std::regex::icase),
    // youtube.com/v/ID or /embed/ID
    std::regex(R"((?:https?://)?(?:www\.)?youtube\.com/(?:v|embed)/([0-9A-Za-z_-]{11}))", std::regex::icase),
    // Just the ID
    std::regex(R"(^([0-9A-Za-z_-]{11})$)"),
};

// Format metadata (itag -> properties)
const std::map<int, std::map<std::string, nlohmann::json>> YoutubeIE::FORMAT_MAP = {
    // Progressive MP4 formats (video+audio combined)
    {18, {{"ext", "mp4"}, {"width", 640}, {"height", 360}, {"acodec", "aac"}, {"vcodec", "h264"}}},
    {22, {{"ext", "mp4"}, {"width", 1280}, {"height", 720}, {"acodec", "aac"}, {"vcodec", "h264"}}},
    {37, {{"ext", "mp4"}, {"width", 1920}, {"height", 1080}, {"acodec", "aac"}, {"vcodec", "h264"}}},
    {38, {{"ext", "mp4"}, {"width", 4096}, {"height", 3072}, {"acodec", "aac"}, {"vcodec", "h264"}}},

    // DASH video-only
    {133, {{"ext", "mp4"}, {"height", 240}, {"vcodec", "h264"}, {"acodec", "none"}}},
    {134, {{"ext", "mp4"}, {"height", 360}, {"vcodec", "h264"}, {"acodec", "none"}}},
    {135, {{"ext", "mp4"}, {"height", 480}, {"vcodec", "h264"}, {"acodec", "none"}}},
    {136, {{"ext", "mp4"}, {"height", 720}, {"vcodec", "h264"}, {"acodec", "none"}}},
    {137, {{"ext", "mp4"}, {"height", 1080}, {"vcodec", "h264"}, {"acodec", "none"}}},
    {160, {{"ext", "mp4"}, {"height", 144}, {"vcodec", "h264"}, {"acodec", "none"}}},
    {264, {{"ext", "mp4"}, {"height", 1440}, {"vcodec", "h264"}, {"acodec", "none"}}},
    {266, {{"ext", "mp4"}, {"height", 2160}, {"vcodec", "h264"}, {"acodec", "none"}}},

    // DASH audio-only
    {139, {{"ext", "m4a"}, {"acodec", "aac"}, {"abr", 48}, {"vcodec", "none"}}},
    {140, {{"ext", "m4a"}, {"acodec", "aac"}, {"abr", 128}, {"vcodec", "none"}}},
    {141, {{"ext", "m4a"}, {"acodec", "aac"}, {"abr", 256}, {"vcodec", "none"}}},

    // WebM formats
    {43, {{"ext", "webm"}, {"width", 640}, {"height", 360}, {"acodec", "vorbis"}, {"vcodec", "vp8"}}},
    {44, {{"ext", "webm"}, {"width", 854}, {"height", 480}, {"acodec", "vorbis"}, {"vcodec", "vp8"}}},
    {45, {{"ext", "webm"}, {"width", 1280}, {"height", 720}, {"acodec", "vorbis"}, {"vcodec", "vp8"}}},
    {46, {{"ext", "webm"}, {"width", 1920}, {"height", 1080}, {"acodec", "vorbis"}, {"vcodec", "vp8"}}},

    // WebM DASH video - VP9
    {247, {{"ext", "webm"}, {"height", 720}, {"vcodec", "vp9"}, {"acodec", "none"}}},
    {248, {{"ext", "webm"}, {"height", 1080}, {"vcodec", "vp9"}, {"acodec", "none"}}},
    {271, {{"ext", "webm"}, {"height", 1440}, {"vcodec", "vp9"}, {"acodec", "none"}}},
    {272, {{"ext", "webm"}, {"height", 4320}, {"vcodec", "vp9"}, {"acodec", "none"}}},  // 8K
    {313, {{"ext", "webm"}, {"height", 2160}, {"vcodec", "vp9"}, {"acodec", "none"}}},
    {315, {{"ext", "webm"}, {"height", 4320}, {"vcodec", "vp9"}, {"acodec", "none"}}},  // 8K 60fps

    // WebM DASH video - AV1
    {394, {{"ext", "mp4"}, {"height", 144}, {"vcodec", "av01.0.00M.08"}, {"acodec", "none"}}},
    {395, {{"ext", "mp4"}, {"height", 240}, {"vcodec", "av01.0.00M.08"}, {"acodec", "none"}}},
    {396, {{"ext", "mp4"}, {"height", 360}, {"vcodec", "av01.0.01M.08"}, {"acodec", "none"}}},
    {397, {{"ext", "mp4"}, {"height", 480}, {"vcodec", "av01.0.04M.08"}, {"acodec", "none"}}},
    {398, {{"ext", "mp4"}, {"height", 720}, {"vcodec", "av01.0.05M.08"}, {"acodec", "none"}}},
    {399, {{"ext", "mp4"}, {"height", 1080}, {"vcodec", "av01.0.08M.08"}, {"acodec", "none"}}},
    {400, {{"ext", "mp4"}, {"height", 1440}, {"vcodec", "av01.0.12M.08"}, {"acodec", "none"}}},
    {401, {{"ext", "mp4"}, {"height", 2160}, {"vcodec", "av01.0.12M.08"}, {"acodec", "none"}}},
    {402, {{"ext", "mp4"}, {"height", 4320}, {"vcodec", "av01.0.13M.08"}, {"acodec", "none"}}},  // 8K

    // WebM DASH audio
    {249, {{"ext", "webm"}, {"acodec", "opus"}, {"abr", 50}, {"vcodec", "none"}}},
    {250, {{"ext", "webm"}, {"acodec", "opus"}, {"abr", 70}, {"vcodec", "none"}}},
    {251, {{"ext", "webm"}, {"acodec", "opus"}, {"abr", 160}, {"vcodec", "none"}}},
};

YoutubeIE::YoutubeIE(core::YoutubeDL* downloader)
    : InfoExtractor(downloader) {
}

std::string YoutubeIE::ie_key() const {
    return "Youtube";
}

std::string YoutubeIE::ie_name() const {
    return "youtube.com";
}

bool YoutubeIE::suitable(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
}

std::string YoutubeIE::extract_id(const std::string& url) {
    for (const auto& pattern : URL_PATTERNS) {
        std::smatch match;
        if (std::regex_search(url, match, pattern)) {
            if (match.size() > 1) {
                return match[1].str();
            }
        }
    }
    throw std::runtime_error("Unable to extract YouTube video ID from URL: " + url);
}

nlohmann::json YoutubeIE::_call_innertube_api(
    const std::string& video_id,
    const std::string& client_name
) {
    // YouTube InnerTube API endpoint
    const std::string api_url = "https://www.youtube.com/youtubei/v1/player?key=AIzaSyAO_FJ2SlqU8Q4STEHLGCilw_Y9_11qcW8";

    // Build InnerTube request body
    nlohmann::json client_config = {
        {"clientName", client_name},
        {"hl", "en"},
        {"gl", "US"}
    };

    // Add appropriate client version based on client type
    if (client_name == "ANDROID") {
        client_config["clientVersion"] = "20.10.38";
        // NOTE: Do NOT add androidSdkVersion - it triggers Po-Token requirement!
        // This makes it "android_sdkless" which bypasses YouTube's restrictions
    } else if (client_name == "IOS") {
        client_config["clientVersion"] = "19.09.3";
        client_config["deviceModel"] = "iPhone14,3";
        client_config["osName"] = "iOS";
        client_config["osVersion"] = "15.6.0.19G71";
    } else if (client_name == "TVHTML5") {
        client_config["clientVersion"] = "7.20250923.13.00";
    } else if (client_name == "WEB_SAFARI") {
        client_config["clientName"] = "WEB";  // Safari uses WEB client name
        client_config["clientVersion"] = "2.20250925.01.00";
    } else if (client_name == "MWEB") {
        client_config["clientVersion"] = "2.20240726.08.00";
    } else {
        // WEB client
        client_config["clientVersion"] = "2.20250925.01.00";
    }

    nlohmann::json context = {
        {"client", client_config}
    };

    nlohmann::json request_body = {
        {"videoId", video_id},
        {"context", context}
    };

    // Make POST request using downloader's HTTP client
    if (!downloader()) {
        throw std::runtime_error("No downloader instance available");
    }

    // Get HTTP client from downloader
    auto& http_client = downloader()->http_client();

    // Convert JSON to bytes
    std::string json_str = request_body.dump();
    std::vector<uint8_t> post_data(json_str.begin(), json_str.end());

    // Set client-specific user-agent (critical for avoiding 403 errors!)
    std::string user_agent;
    if (client_name == "ANDROID") {
        user_agent = "com.google.android.youtube/20.10.38 (Linux; U; Android 11) gzip";
    } else if (client_name == "TVHTML5") {
        user_agent = "Mozilla/5.0 (ChromiumStylePlatform) Cobalt/Version";
    } else if (client_name == "WEB_SAFARI") {
        user_agent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.5 Safari/605.1.15,gzip(gfe)";
    } else {
        user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36";
    }

    // Make POST request with proper content type
    std::map<std::string, std::string> headers = {
        {"Content-Type", "application/json"},
        {"User-Agent", user_agent}
    };

    to_screen("[" + ie_key() + "] " + video_id + ": Downloading player API JSON");
    auto response = http_client.post(api_url, post_data, headers);

    if (!response.is_success()) {
        throw std::runtime_error("HTTP Error " + std::to_string(response.status()));
    }

    std::string response_text = response.read_all();
    return nlohmann::json::parse(response_text);
}

std::map<std::string, nlohmann::json> YoutubeIE::_get_format_metadata(int itag) {
    auto it = FORMAT_MAP.find(itag);
    if (it != FORMAT_MAP.end()) {
        return it->second;
    }
    return {};
}

std::vector<core::InfoDict> YoutubeIE::_extract_formats(
    const nlohmann::json& streaming_data,
    const std::string& video_id,
    const std::string& player_url
) {
    std::vector<core::InfoDict> formats;

    // Helper lambda to decrypt n-parameter in URL
    auto decrypt_url = [&](std::string url) -> std::string {
        if (player_url.empty()) {
            return url;  // No player URL, can't decrypt
        }

        // Check if URL contains 'n=' parameter
        size_t n_pos = url.find("&n=");
        if (n_pos == std::string::npos) {
            n_pos = url.find("?n=");
        }

        if (n_pos != std::string::npos) {
            // Extract n-parameter value
            size_t value_start = n_pos + 3;  // Skip "&n=" or "?n="
            size_t value_end = url.find('&', value_start);
            if (value_end == std::string::npos) {
                value_end = url.length();
            }

            std::string encrypted_n = url.substr(value_start, value_end - value_start);

            // Decrypt it
            std::string decrypted_n = _decrypt_nsig(encrypted_n, video_id, player_url);

            // Replace in URL
            url.replace(value_start, encrypted_n.length(), decrypted_n);
        }

        return url;
    };

    // Extract progressive formats (video+audio combined)
    if (streaming_data.contains("formats") && streaming_data["formats"].is_array()) {
        for (const auto& fmt : streaming_data["formats"]) {
            if (!fmt.contains("url") || !fmt["url"].is_string()) {
                continue;  // Skip formats without direct URL (need signature decryption)
            }

            core::InfoDict format;
            std::string url = fmt["url"].get<std::string>();
            format["url"] = decrypt_url(url);

            if (fmt.contains("itag") && fmt["itag"].is_number()) {
                int itag = fmt["itag"].get<int>();
                format["format_id"] = std::to_string(itag);

                // Add metadata from FORMAT_MAP
                auto metadata = _get_format_metadata(itag);
                for (const auto& [key, value] : metadata) {
                    format[key] = value;
                }
            }

            // Add filesize if available
            if (fmt.contains("contentLength")) {
                if (fmt["contentLength"].is_string()) {
                    format["filesize"] = std::stoll(fmt["contentLength"].get<std::string>());
                } else if (fmt["contentLength"].is_number()) {
                    format["filesize"] = fmt["contentLength"].get<int64_t>();
                }
            }

            formats.push_back(format);
        }
    }

    // Extract adaptive formats (DASH - separate video/audio)
    if (streaming_data.contains("adaptiveFormats") && streaming_data["adaptiveFormats"].is_array()) {
        for (const auto& fmt : streaming_data["adaptiveFormats"]) {
            if (!fmt.contains("url") || !fmt["url"].is_string()) {
                continue;  // Skip formats needing signature decryption
            }

            core::InfoDict format;
            std::string url = fmt["url"].get<std::string>();
            format["url"] = decrypt_url(url);

            if (fmt.contains("itag") && fmt["itag"].is_number()) {
                int itag = fmt["itag"].get<int>();
                format["format_id"] = std::to_string(itag);

                auto metadata = _get_format_metadata(itag);
                for (const auto& [key, value] : metadata) {
                    format[key] = value;
                }
            }

            if (fmt.contains("contentLength")) {
                if (fmt["contentLength"].is_string()) {
                    format["filesize"] = std::stoll(fmt["contentLength"].get<std::string>());
                } else if (fmt["contentLength"].is_number()) {
                    format["filesize"] = fmt["contentLength"].get<int64_t>();
                }
            }

            // Mark as DASH format
            format["protocol"] = "https";

            formats.push_back(format);
        }
    }

    return formats;
}

core::InfoDict YoutubeIE::_real_extract(const std::string& url) {
    // Extract video ID
    std::string video_id = extract_id(url);

    report_extraction(video_id);

    // Call InnerTube API - try multiple clients in Python yt-dlp's priority order
    // Order matters! android_sdkless (ANDROID without SDK version) bypasses Po-Token
    nlohmann::json player_response;
    std::vector<std::string> clients = {"ANDROID", "TVHTML5", "WEB_SAFARI", "WEB"};

    bool success = false;
    std::string last_error;

    for (const auto& client : clients) {
        try {
            player_response = _call_innertube_api(video_id, client);

            // Check if we got valid streaming data
            if (player_response.contains("streamingData")) {
                const auto& streaming_data = player_response["streamingData"];
                if ((streaming_data.contains("formats") && !streaming_data["formats"].empty()) ||
                    (streaming_data.contains("adaptiveFormats") && !streaming_data["adaptiveFormats"].empty())) {
                    success = true;
                    break;
                }
            }
        } catch (const std::exception& e) {
            last_error = e.what();
            continue;
        }
    }

    if (!success) {
        throw std::runtime_error(
            "Failed to get video info from YouTube API: " + last_error
        );
    }

    // Check for playability errors
    if (player_response.contains("playabilityStatus")) {
        const auto& status = player_response["playabilityStatus"];
        if (status.contains("status") && status["status"].is_string()) {
            std::string status_str = status["status"].get<std::string>();
            if (status_str != "OK") {
                std::string reason = "Unknown error";
                if (status.contains("reason") && status["reason"].is_string()) {
                    reason = status["reason"].get<std::string>();
                }
                throw std::runtime_error(
                    "Video unavailable: " + reason + " (status: " + status_str + ")"
                );
            }
        }
    }

    // Extract video details
    core::InfoDict info;
    info["id"] = video_id;
    info["extractor"] = ie_key();
    info["extractor_key"] = ie_key();
    info["webpage_url"] = "https://www.youtube.com/watch?v=" + video_id;

    // Get video details
    if (player_response.contains("videoDetails")) {
        const auto& details = player_response["videoDetails"];

        if (details.contains("title") && details["title"].is_string()) {
            info["title"] = details["title"].get<std::string>();
        }

        if (details.contains("author") && details["author"].is_string()) {
            info["uploader"] = details["author"].get<std::string>();
        }

        if (details.contains("lengthSeconds")) {
            if (details["lengthSeconds"].is_string()) {
                info["duration"] = std::stoi(details["lengthSeconds"].get<std::string>());
            } else if (details["lengthSeconds"].is_number()) {
                info["duration"] = details["lengthSeconds"].get<int>();
            }
        }

        if (details.contains("shortDescription") && details["shortDescription"].is_string()) {
            info["description"] = details["shortDescription"].get<std::string>();
        }

        if (details.contains("viewCount")) {
            if (details["viewCount"].is_string()) {
                info["view_count"] = std::stoll(details["viewCount"].get<std::string>());
            } else if (details["viewCount"].is_number()) {
                info["view_count"] = details["viewCount"].get<int64_t>();
            }
        }
    }

    // Extract formats
    if (!player_response.contains("streamingData")) {
        throw std::runtime_error("No streaming data found - video may be unavailable or require authentication");
    }

    // Extract player URL for n-parameter decryption
    std::string player_url;
    try {
        player_url = _extract_player_url(video_id);
        to_screen("[" + ie_key() + "] Player URL extracted successfully");
    } catch (const std::exception& e) {
        report_warning(
            "Could not extract player URL: " + std::string(e.what()) + "\n"
            "         N-parameter decryption will be skipped",
            video_id
        );
    }

    to_screen("[" + ie_key() + "] Extracting formats...");
    auto formats = _extract_formats(player_response["streamingData"], video_id, player_url);
    to_screen("[" + ie_key() + "] Extracted " + std::to_string(formats.size()) + " formats");

    if (formats.empty()) {
        throw std::runtime_error("No formats found - video may require signature decryption or authentication");
    }

    info["formats"] = formats;

    return info;
}

// ============================================================================
// Player Management & N-Parameter Decryption
// ============================================================================

std::string YoutubeIE::_get_cache_dir() {
    std::string home = utils::get_home_directory();
    std::string cache_dir = utils::join_path(
        utils::join_path(
            utils::join_path(home, ".cache"),
            "yt-dlp-cpp"
        ),
        "player"
    );

    // Create directory if it doesn't exist
    std::filesystem::create_directories(cache_dir);

    return cache_dir;
}

std::string YoutubeIE::_extract_player_url(const std::string& video_id) {
    // Download YouTube watch page to extract player URL
    if (!downloader()) {
        throw std::runtime_error("No downloader instance available");
    }

    auto& http_client = downloader()->http_client();
    std::string watch_url = "https://www.youtube.com/watch?v=" + video_id;

    to_screen("[" + ie_key() + "] " + video_id + ": Downloading webpage to extract player URL");
    to_screen("[" + ie_key() + "] Making HTTP GET request...");

    auto response = http_client.get(watch_url);
    to_screen("[" + ie_key() + "] HTTP request completed, checking status...");

    if (!response.is_success()) {
        throw std::runtime_error("Failed to download YouTube webpage");
    }

    to_screen("[" + ie_key() + "] Reading response body...");
    std::string webpage = response.read_all();
    to_screen("[" + ie_key() + "] Response read complete, size: " + std::to_string(webpage.size()));

    // Extract player URL using simpler string search (avoid regex catastrophic backtracking)
    // Look for: "jsUrl":"/s/player/..." or "PLAYER_JS_URL":"/s/player/..."
    std::vector<std::string> search_patterns = {
        "\"jsUrl\":\"",
        "\"PLAYER_JS_URL\":\"",
        "/s/player/"
    };

    to_screen("[" + ie_key() + "] Searching for player URL...");
    for (const auto& search_str : search_patterns) {
        size_t pos = webpage.find(search_str);
        if (pos != std::string::npos) {
            size_t url_start = webpage.find("/s/player/", pos);
            if (url_start != std::string::npos) {
                size_t url_end = webpage.find("\"", url_start);
                if (url_end != std::string::npos) {
                    std::string player_path = webpage.substr(url_start, url_end - url_start);
                    to_screen("[" + ie_key() + "] Found player URL: " + player_path);
                    return "https://www.youtube.com" + player_path;
                }
            }
        }
    }

    throw std::runtime_error("Could not extract player URL from webpage");
}

std::string YoutubeIE::_download_player(const std::string& player_url) {
    // Extract player ID from URL
    std::regex id_pattern(R"(/s/player/([a-zA-Z0-9_-]+)/)");
    std::smatch match;

    std::string player_id;
    if (std::regex_search(player_url, match, id_pattern)) {
        player_id = match[1].str();
    } else {
        // Fallback: use hash of URL
        player_id = std::to_string(std::hash<std::string>{}(player_url));
    }

    // Check cache first
    std::string cache_dir = _get_cache_dir();
    std::string cache_file = utils::join_path(cache_dir, player_id + ".js");

    if (std::filesystem::exists(cache_file)) {
        to_screen("[" + ie_key() + "] Using cached player: " + player_id);
        auto cached_content = utils::read_file(cache_file);
        if (cached_content) {
            return *cached_content;
        }
    }

    // Download player JS
    to_screen("[" + ie_key() + "] Downloading player: " + player_id);

    if (!downloader()) {
        throw std::runtime_error("No downloader instance available");
    }

    auto& http_client = downloader()->http_client();
    to_screen("[" + ie_key() + "] Got HTTP client, fetching: " + player_url);

    auto response = http_client.get(player_url);
    to_screen("[" + ie_key() + "] HTTP request completed");

    if (!response.is_success()) {
        throw std::runtime_error("Failed to download player from: " + player_url +
                               " (HTTP " + std::to_string(response.status()) + ")");
    }

    std::string player_code = response.read_all();

    // Cache it
    utils::write_file(cache_file, player_code);
    to_screen("[" + ie_key() + "] Cached player to: " + cache_file);

    return player_code;
}

std::string YoutubeIE::_extract_n_function_name(const std::string& jscode) {
    // Port of Python yt-dlp's n-function name extraction
    // Patterns to find the n-parameter transformation function

    std::vector<std::regex> patterns = {
        // Pattern 1: .get("n"))&&(b=nfunc(b)
        std::regex(R"(\.get\("n"\)\)&&\(b=([a-zA-Z0-9$_]+)\()"),

        // Pattern 2: b=String.fromCharCode(110),c=a.get(b))&&(c=narray[idx](c)
        std::regex(R"(b=String\.fromCharCode\(110\).*?\.get\(b\)\)&&\([^=]+=([a-zA-Z0-9$_]+)(?:\[(\d+)\])?\()"),

        // Pattern 3: Generic fallback - function that returns something + "_w8_"
        std::regex(R"(;([a-zA-Z0-9$_]+)\s*=\s*function\([a-zA-Z0-9$_]+\)\s*\{[^}]*return\s*["'][\w-]+_w8_["'])"),
    };

    for (size_t i = 0; i < patterns.size(); i++) {
        std::smatch match;
        if (std::regex_search(jscode, match, patterns[i])) {
            if (match.size() > 1) {
                std::string func_name = match[1].str();

                // If there's an array index [idx], need to extract from array
                if (match.size() > 2 && !match[2].str().empty()) {
                    int idx = std::stoi(match[2].str());

                    // Find array definition: var funcname = [...]
                    std::string array_pattern_str = R"(var\s+)" + func_name + R"(\s*=\s*\[([^\]]+)\])";
                    std::regex array_pattern(array_pattern_str);
                    std::smatch array_match;

                    if (std::regex_search(jscode, array_match, array_pattern)) {
                        std::string array_content = array_match[1].str();

                        // Split by comma to get array elements
                        std::vector<std::string> elements;
                        std::istringstream ss(array_content);
                        std::string element;
                        while (std::getline(ss, element, ',')) {
                            element = utils::strip(element);
                            elements.push_back(element);
                        }

                        if (idx < static_cast<int>(elements.size())) {
                            func_name = elements[idx];
                        }
                    }
                }

                return func_name;
            }
        }
    }

    throw std::runtime_error("Could not extract n-function name from player code");
}

std::string YoutubeIE::_extract_n_function_code(const std::string& jscode, const std::string& function_name) {
    // Extract the complete function definition
    // Pattern: funcname=function(a){...} or function funcname(a){...}

    std::vector<std::string> patterns_str = {
        function_name + R"(\s*=\s*function\s*\(([^)]*)\)\s*\{)",
        R"(function\s+)" + function_name + R"(\s*\(([^)]*)\)\s*\{)",
    };

    for (const auto& pattern_str : patterns_str) {
        std::regex pattern(pattern_str);
        std::smatch match;

        if (std::regex_search(jscode, match, pattern)) {
            // Found function start - now extract the complete body
            size_t start_pos = match.position() + match.length();

            // Count braces to find function end
            int brace_count = 1;
            size_t pos = start_pos;

            while (pos < jscode.length() && brace_count > 0) {
                if (jscode[pos] == '{') {
                    brace_count++;
                } else if (jscode[pos] == '}') {
                    brace_count--;
                }
                pos++;
            }

            if (brace_count == 0) {
                // Extract complete function
                std::string func_code = jscode.substr(match.position(), pos - match.position());
                return func_code;
            }
        }
    }

    throw std::runtime_error("Could not extract n-function code for: " + function_name);
}

std::string YoutubeIE::_decrypt_nsig(const std::string& s, const std::string& video_id, const std::string& player_url) {
    try {
        // Download player code
        std::string player_code = _download_player(player_url);

        // Extract n-function name
        std::string func_name = _extract_n_function_name(player_code);
        to_screen("[" + ie_key() + "] Found n-function: " + func_name);

        // Extract n-function code
        std::string func_code = _extract_n_function_code(player_code, func_name);

        // Execute with QuickJS
        utils::JSInterpreter js;
        js.evaluate(func_code);

        std::string decrypted = js.call_function(func_name, s);

        to_screen("[" + ie_key() + "] Decrypted n-parameter: " + s + " -> " + decrypted);

        return decrypted;

    } catch (const std::exception& e) {
        report_warning(
            "N-parameter decryption failed: " + std::string(e.what()) + "\n"
            "         Some formats may be throttled or unavailable",
            video_id
        );
        // Return original value as fallback
        return s;
    }
}

} // namespace ytdlp::extractor
