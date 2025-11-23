#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/extractor/zoom.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include "ytdlp/networking/cookie_jar.hpp"
#include <iostream>
#include <memory>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <vector>
#include <fmt/core.h>

using namespace ytdlp;

/**
 * Sanitize a string for use as filename.
 * Replaces unsafe characters, collapses separators, and limits length.
 */
std::string sanitize_filename(const std::string& name, size_t max_length = 100) {
    std::string result;
    result.reserve(name.size());

    for (char c : name) {
        // Replace unsafe filesystem characters and normalize separators
        if (c == '/' || c == '\\' || c == ':' || c == '*' ||
            c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            result += '_';
        } else if (c == ' ' || c == '\t') {
            result += '_';  // Whitespace to underscores
        } else {
            result += c;
        }
    }

    // Normalize separator patterns: replace _-_, -_-, _--, --_, etc. with single -
    std::regex separator_pattern(R"([_\-]+)");
    result = std::regex_replace(result, separator_pattern, "-");

    // Trim leading/trailing separators
    while (!result.empty() && (result.front() == '-' || result.front() == '_')) {
        result.erase(0, 1);
    }
    while (!result.empty() && (result.back() == '-' || result.back() == '_')) {
        result.pop_back();
    }

    // Limit length
    if (result.length() > max_length) {
        result = result.substr(0, max_length);
        // Don't cut in the middle of a word if possible
        size_t last_sep = result.rfind('-');
        if (last_sep == std::string::npos) {
            last_sep = result.rfind('_');
        }
        if (last_sep != std::string::npos && last_sep > max_length * 0.7) {
            result = result.substr(0, last_sep);
        }
    }

    return result;
}

/**
 * Extract timestamp from Zoom URL (format: GMT20251117-140940).
 * Returns ISO8601-like prefix: "2025-11-17_140940"
 */
std::string extract_zoom_timestamp(const std::string& url) {
    // Pattern: GMT followed by YYYYMMDD-HHMMSS
    std::regex timestamp_regex(R"(GMT(\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})(\d{2}))");
    std::smatch match;

    if (std::regex_search(url, match, timestamp_regex)) {
        // Format: YYYY-MM-DD_HHMMSS
        return fmt::format("{}-{}-{}_{}{}{}",
            match[1].str(), match[2].str(), match[3].str(),
            match[4].str(), match[5].str(), match[6].str());
    }

    // Fallback: use current date/time
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_now = std::localtime(&time_t_now);

    return fmt::format("{:04d}-{:02d}-{:02d}_{:02d}{:02d}{:02d}",
        tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
        tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
}

/**
 * Generate smart filename for video.
 * Format: YYYY-MM-DD_HHMMSS_<title>_<id_short>.ext
 */
std::string generate_smart_filename(
    const core::InfoDict& info,
    const std::string& download_url,
    const std::string& extension = "mp4"
) {
    std::string timestamp = extract_zoom_timestamp(download_url);

    std::string title = "recording";
    if (info.contains("title") && info["title"].is_string()) {
        title = sanitize_filename(info["title"].get<std::string>(), 80);
    }

    std::string short_id;
    if (info.contains("id") && info["id"].is_string()) {
        std::string full_id = info["id"].get<std::string>();
        size_t dot_pos = full_id.find('.');
        if (dot_pos != std::string::npos && dot_pos <= 12) {
            short_id = full_id.substr(0, dot_pos);
        } else {
            short_id = full_id.substr(0, std::min(size_t(12), full_id.length()));
        }
    }

    std::string filename = timestamp + "_" + title;
    if (!short_id.empty()) {
        filename += "_" + short_id;
    }
    filename += "." + extension;

    return filename;
}

/**
 * Download a file from URL to output path using streaming.
 */
bool download_file(const std::string& url, const std::string& output_path,
                   networking::CurlHttpClient& http_client) {
    fmt::print("Downloading from: {}\n", url);
    fmt::print("Saving to: {}\n", output_path);

    std::map<std::string, std::string> headers = {
        {"User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"},
        {"Accept", "*/*"},
        {"Accept-Language", "en-US,en;q=0.9"},
        {"Referer", "https://zoom.us/"}
    };

    auto start_time = std::chrono::steady_clock::now();
    int64_t last_bytes = 0;
    auto last_update = start_time;

    auto progress_callback = [&](int64_t downloaded, int64_t total) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count();

        if (elapsed > 500 || downloaded == total) {
            double speed = 0.0;
            if (elapsed > 0) {
                speed = (downloaded - last_bytes) / (elapsed / 1000.0);
            }

            if (total > 0) {
                double percent = (100.0 * downloaded) / total;
                double total_mb = total / (1024.0 * 1024.0);
                double speed_mb = speed / (1024.0 * 1024.0);

                int64_t remaining = total - downloaded;
                int eta_seconds = (speed > 0) ? static_cast<int>(remaining / speed) : 0;

                fmt::print("\r[download] {:.1f}% of {:.2f}MB at {:.2f}MB/s ETA {:02d}:{:02d}",
                          percent, total_mb, speed_mb, eta_seconds / 60, eta_seconds % 60);
                std::fflush(stdout);
            } else {
                double downloaded_mb = downloaded / (1024.0 * 1024.0);
                double speed_mb = speed / (1024.0 * 1024.0);
                fmt::print("\r[download] {:.2f}MB at {:.2f}MB/s", downloaded_mb, speed_mb);
                std::fflush(stdout);
            }

            last_bytes = downloaded;
            last_update = now;
        }
    };

    try {
        bool success = http_client.download_to_file(url, output_path, headers, progress_callback);

        if (success) {
            fmt::print("\nDownload complete!\n");
            return true;
        } else {
            fmt::print("\n");
            return false;
        }

    } catch (const std::exception& e) {
        fmt::print("\nDownload failed: {}\n", e.what());
        return false;
    }
}

/**
 * Read URLs from a batch file (one URL per line).
 * Ignores empty lines and lines starting with # (comments).
 */
std::vector<std::string> read_batch_file(const std::string& filename) {
    std::vector<std::string> urls;
    std::ifstream file(filename);

    if (!file) {
        throw std::runtime_error("Cannot open batch file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;  // Empty line

        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);

        // Skip comments
        if (line.empty() || line[0] == '#') continue;

        urls.push_back(line);
    }

    return urls;
}

/**
 * Process a single URL: extract info and optionally download.
 * Returns true on success, false on failure.
 */
bool process_url(
    const std::string& url,
    const std::string& custom_output,
    extractor::ZoomIE& zoom,
    core::YoutubeDL& ydl,
    bool info_only,
    bool quiet
) {
    try {
        // Check if URL is supported
        if (!zoom.suitable(url)) {
            fmt::print(stderr, "Error: URL is not a valid Zoom recording URL: {}\n", url);
            return false;
        }

        if (!quiet) {
            fmt::print("Extracting video info...\n");
        }

        // Extract video information
        core::InfoDict info = zoom.extract(url);

        if (!quiet || info_only) {
            fmt::print("\nVideo Information:\n");
            fmt::print("------------------\n");

            if (info.contains("title")) {
                fmt::print("Title: {}\n", info["title"].get<std::string>());
            }
            if (info.contains("id")) {
                fmt::print("ID: {}\n", info["id"].get<std::string>());
            }
            if (info.contains("duration")) {
                int duration = info["duration"].get<int>();
                fmt::print("Duration: {}:{:02d}\n", duration / 60, duration % 60);
            }

            if (info.contains("formats") && info["formats"].is_array()) {
                fmt::print("\nAvailable formats: {}\n", info["formats"].size());

                auto formats = info["formats"].get<std::vector<core::InfoDict>>();
                for (size_t i = 0; i < formats.size() && i < 10; i++) {
                    const auto& f = formats[i];
                    fmt::print("  Format #{}: ", i+1);
                    if (f.contains("format_id")) {
                        fmt::print("{} ", f["format_id"].get<std::string>());
                    }
                    if (f.contains("ext")) {
                        fmt::print("{} ", f["ext"].get<std::string>());
                    }
                    if (f.contains("height")) {
                        fmt::print("{}p ", f["height"].get<int>());
                    }
                    fmt::print("\n");
                }
            }
        }

        if (info_only) {
            return true;
        }

        // Get download URL
        std::string download_url;
        if (info.contains("url") && info["url"].is_string()) {
            download_url = info["url"].get<std::string>();
        } else if (info.contains("formats") && info["formats"].is_array()) {
            auto formats = info["formats"].get<std::vector<core::InfoDict>>();
            for (const auto& f : formats) {
                if (f.contains("url") && f["url"].is_string()) {
                    download_url = f["url"].get<std::string>();
                    break;
                }
            }
        }

        if (download_url.empty()) {
            fmt::print(stderr, "Error: No download URL found\n");
            return false;
        }

        // Generate output filename
        std::string output = custom_output;
        if (output.empty()) {
            output = generate_smart_filename(info, download_url, "mp4");
        }

        if (!quiet) {
            fmt::print("\nDownloading to: {}\n\n", output);
        }

        // Download the video
        bool success = download_file(download_url, output, ydl.http_client());

        if (success) {
            if (!quiet) {
                fmt::print("\n✓ Download successful!\n");
                fmt::print("  Saved to: {}\n", output);
            }
            return true;
        } else {
            fmt::print(stderr, "\n✗ Download failed\n");
            return false;
        }

    } catch (const std::exception& e) {
        fmt::print(stderr, "Error processing {}: {}\n", url, e.what());
        return false;
    }
}

void print_usage(const char* program_name) {
    fmt::print("Usage: {} [options] <url>\n", program_name);
    fmt::print("       {} [options] -a <batch_file>\n\n", program_name);
    fmt::print("Options:\n");
    fmt::print("  -a, --batch-file <file>  Read URLs from file (one per line)\n");
    fmt::print("  -o, --output <file>      Output filename (default: auto-generated)\n");
    fmt::print("  -c, --cookies <file>     Netscape cookie file for authentication\n");
    fmt::print("  -i, --info               Print video info only (don't download)\n");
    fmt::print("  -q, --quiet              Quiet mode\n");
    fmt::print("  -h, --help               Show this help\n");
    fmt::print("\nAuto-generated filename format:\n");
    fmt::print("  YYYY-MM-DD_HHMMSS_<title>_<id>.mp4\n");
    fmt::print("  Example: 2025-11-17_140940_Finanzas_Empresariales-GI3101_k-O3Gvpp.mp4\n");
    fmt::print("\nBatch file format:\n");
    fmt::print("  One URL per line. Empty lines and lines starting with # are ignored.\n");
    fmt::print("\nExamples:\n");
    fmt::print("  {} https://zoom.us/rec/play/xxx\n", program_name);
    fmt::print("  {} -c cookies.txt -a urls.txt\n", program_name);
    fmt::print("  {} -c cookies.txt -o custom.mp4 https://zoom.us/rec/play/xxx\n", program_name);
}

int main(int argc, char** argv) {
    try {
        // Parse arguments
        std::string url;
        std::string output;
        std::string cookie_file;
        std::string batch_file;
        bool quiet = false;
        bool info_only = false;

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                print_usage(argv[0]);
                return 0;
            } else if (arg == "-q" || arg == "--quiet") {
                quiet = true;
            } else if (arg == "-i" || arg == "--info") {
                info_only = true;
            } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
                output = argv[++i];
            } else if ((arg == "-c" || arg == "--cookies") && i + 1 < argc) {
                cookie_file = argv[++i];
            } else if ((arg == "-a" || arg == "--batch-file") && i + 1 < argc) {
                batch_file = argv[++i];
            } else if (arg[0] != '-') {
                url = arg;
            }
        }

        // Need either URL or batch file
        if (url.empty() && batch_file.empty()) {
            print_usage(argv[0]);
            return 1;
        }

        // Build list of URLs
        std::vector<std::string> urls;
        if (!batch_file.empty()) {
            urls = read_batch_file(batch_file);
            if (urls.empty()) {
                fmt::print(stderr, "Error: No URLs found in batch file\n");
                return 1;
            }
            if (!quiet) {
                fmt::print("Loaded {} URLs from {}\n\n", urls.size(), batch_file);
            }
        }
        if (!url.empty()) {
            urls.push_back(url);
        }

        if (!quiet) {
            fmt::print("yt-dlp-cpp - Zoom Video Downloader (C++ Port)\n");
            fmt::print("=============================================\n\n");
        }

        // Initialize YoutubeDL
        core::YoutubeDLParams params;
        params.quiet = quiet;
        core::YoutubeDL ydl(params);

        // Load cookies if provided
        if (!cookie_file.empty()) {
            if (!quiet) {
                fmt::print("Loading cookies from: {}\n", cookie_file);
            }
            auto cookie_jar = std::make_shared<networking::CookieJar>();
            cookie_jar->load(cookie_file);
            ydl.http_client().set_cookie_jar(cookie_jar);
            if (!quiet) {
                fmt::print("Loaded {} cookies\n\n", cookie_jar->size());
            }
        }

        // Create Zoom extractor
        extractor::ZoomIE zoom(&ydl);

        // Process all URLs
        int success_count = 0;
        int fail_count = 0;

        for (size_t i = 0; i < urls.size(); i++) {
            const std::string& current_url = urls[i];

            if (urls.size() > 1 && !quiet) {
                fmt::print("\n[{}/{}] Processing: {}\n", i + 1, urls.size(), current_url);
                fmt::print("─────────────────────────────────────────────────\n");
            }

            // Only use custom output for single URL
            std::string current_output = (urls.size() == 1) ? output : "";

            if (process_url(current_url, current_output, zoom, ydl, info_only, quiet)) {
                success_count++;
            } else {
                fail_count++;
            }
        }

        // Summary for batch downloads
        if (urls.size() > 1 && !quiet) {
            fmt::print("\n═══════════════════════════════════════════════════\n");
            fmt::print("Batch complete: {} succeeded, {} failed\n", success_count, fail_count);
        }

        return (fail_count > 0) ? 1 : 0;

    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}
