/**
 * Filesystem Utilities Implementation
 */

#include "ytdlp/utils/filesystem_utils.hpp"
#include "ytdlp/utils/string_utils.hpp"

#include <fstream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#endif

namespace ytdlp::utils {

// ============================================================================
// Path Manipulation
// ============================================================================

std::string expand_path(std::string_view path) {
    std::string result(path);

    // Expand ~ to home directory
    if (starts_with(result, "~")) {
        std::string home = get_home_directory();
        if (result.length() == 1) {
            result = home;
        } else if (result[1] == '/' || result[1] == '\\') {
            result = home + result.substr(1);
        }
    }

    // Expand environment variables
    size_t pos = 0;
    while ((pos = result.find('$', pos)) != std::string::npos) {
        size_t end = pos + 1;

        // Handle ${VAR} syntax
        bool braced = false;
        if (end < result.length() && result[end] == '{') {
            braced = true;
            ++end;
        }

        // Find end of variable name
        while (end < result.length() &&
               (std::isalnum(result[end]) || result[end] == '_')) {
            ++end;
        }

        if (braced && end < result.length() && result[end] == '}') {
            ++end;
        }

        // Extract variable name
        size_t var_start = pos + 1 + (braced ? 1 : 0);
        size_t var_len = end - var_start - (braced ? 1 : 0);
        std::string var_name = result.substr(var_start, var_len);

        // Get environment variable value
        const char* var_value = std::getenv(var_name.c_str());
        if (var_value) {
            result.replace(pos, end - pos, var_value);
            pos += std::strlen(var_value);
        } else {
            pos = end;
        }
    }

    return result;
}

std::string absolute_path(std::string_view path) {
    try {
        return fs::absolute(fs::path(path)).string();
    } catch (...) {
        return std::string(path);
    }
}

std::string normalize_path(std::string_view path) {
    try {
        return fs::weakly_canonical(fs::path(path)).string();
    } catch (...) {
        return std::string(path);
    }
}

std::string join_path(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return "";
    }

    fs::path result(parts[0]);
    for (size_t i = 1; i < parts.size(); ++i) {
        result /= parts[i];
    }
    return result.string();
}

std::string join_path(std::string_view base, std::string_view relative) {
    fs::path result(base);
    result /= relative;
    return result.string();
}

std::string dirname(std::string_view path) {
    return fs::path(path).parent_path().string();
}

std::string basename(std::string_view path) {
    return fs::path(path).filename().string();
}

std::string get_extension(std::string_view path) {
    return fs::path(path).extension().string();
}

std::string get_stem(std::string_view path) {
    return fs::path(path).stem().string();
}

std::string replace_extension(std::string_view path, std::string_view new_ext) {
    fs::path p(path);

    // Ensure extension has leading dot
    std::string ext(new_ext);
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }

    p.replace_extension(ext);
    return p.string();
}

std::string prepend_extension(std::string_view path, std::string_view ext) {
    fs::path p(path);
    std::string stem = p.stem().string();
    std::string extension = p.extension().string();

    // Ensure ext has leading dot
    std::string prepend_ext(ext);
    if (!prepend_ext.empty() && prepend_ext[0] != '.') {
        prepend_ext = "." + prepend_ext;
    }

    return (p.parent_path() / (stem + prepend_ext + extension)).string();
}

// ============================================================================
// File/Directory Operations
// ============================================================================

bool path_exists(std::string_view path) {
    std::error_code ec;
    return fs::exists(fs::path(path), ec);
}

bool is_file(std::string_view path) {
    std::error_code ec;
    return fs::is_regular_file(fs::path(path), ec);
}

bool is_directory(std::string_view path) {
    std::error_code ec;
    return fs::is_directory(fs::path(path), ec);
}

bool make_directory(std::string_view path) {
    std::error_code ec;
    fs::create_directories(fs::path(path), ec);
    return !ec;
}

bool remove_file(std::string_view path) {
    std::error_code ec;
    fs::remove(fs::path(path), ec);
    return !ec;
}

bool remove_directory(std::string_view path) {
    std::error_code ec;
    fs::remove_all(fs::path(path), ec);
    return !ec;
}

std::optional<int64_t> file_size(std::string_view path) {
    std::error_code ec;
    auto size = fs::file_size(fs::path(path), ec);
    if (ec) {
        return std::nullopt;
    }
    return static_cast<int64_t>(size);
}

std::optional<int64_t> file_mtime(std::string_view path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(fs::path(path), ec);
    if (ec) {
        return std::nullopt;
    }

    // Convert to Unix timestamp
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    return std::chrono::duration_cast<std::chrono::seconds>(
        sctp.time_since_epoch()
    ).count();
}

std::vector<std::string> list_directory(std::string_view path, bool files_only) {
    std::vector<std::string> result;
    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(fs::path(path), ec)) {
        if (ec) break;

        if (files_only && !entry.is_regular_file()) {
            continue;
        }

        result.push_back(entry.path().filename().string());
    }

    return result;
}

std::vector<std::string> list_directory_full(std::string_view path, bool files_only) {
    std::vector<std::string> result;
    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(fs::path(path), ec)) {
        if (ec) break;

        if (files_only && !entry.is_regular_file()) {
            continue;
        }

        result.push_back(entry.path().string());
    }

    return result;
}

std::vector<std::string> find_files(std::string_view path, std::string_view pattern) {
    std::vector<std::string> result;
    std::error_code ec;

    // Simple glob matching - just check extension for now
    // Full glob support would require a glob library
    std::string ext_pattern;
    if (starts_with(pattern, "*.")) {
        ext_pattern = std::string(pattern.substr(1)); // ".mp4" from "*.mp4"
    }

    for (const auto& entry : fs::recursive_directory_iterator(fs::path(path), ec)) {
        if (ec) break;

        if (!entry.is_regular_file()) {
            continue;
        }

        if (!ext_pattern.empty()) {
            if (entry.path().extension() == ext_pattern) {
                result.push_back(entry.path().string());
            }
        } else {
            // No pattern, return all files
            result.push_back(entry.path().string());
        }
    }

    return result;
}

// ============================================================================
// Filename Generation
// ============================================================================

std::string subtitles_filename(
    std::string_view video_filename,
    std::string_view language,
    std::string_view format
) {
    fs::path p(video_filename);
    std::string stem = p.stem().string();

    // Ensure format has leading dot
    std::string fmt(format);
    if (!fmt.empty() && fmt[0] != '.') {
        fmt = "." + fmt;
    }

    return (p.parent_path() / (stem + "." + std::string(language) + fmt)).string();
}

std::string temp_filename(std::string_view base_path, std::string_view prefix) {
    fs::path p(base_path);
    std::string stem = p.stem().string();
    std::string ext = p.extension().string();

    // Generate unique suffix using timestamp and random number
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::srand(static_cast<unsigned>(now));
    int random = std::rand();

    std::string suffix = std::to_string(now) + "_" + std::to_string(random);
    std::string filename = stem + "." + std::string(prefix) + "." + suffix + ext;

    return (p.parent_path() / filename).string();
}

std::string get_temp_directory() {
    return fs::temp_directory_path().string();
}

// ============================================================================
// File I/O Helpers
// ============================================================================

std::optional<std::string> read_file(std::string_view path) {
    std::ifstream file{std::string(path)};  // Use braced init to avoid most vexing parse
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::string content{std::istreambuf_iterator<char>{file},
                        std::istreambuf_iterator<char>{}};
    return content;
}

std::optional<std::vector<uint8_t>> read_binary_file(std::string_view path) {
    std::ifstream file{std::string(path), std::ios::binary};
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::vector<uint8_t> data{std::istreambuf_iterator<char>{file},
                              std::istreambuf_iterator<char>{}};
    return data;
}

bool write_file(std::string_view path, std::string_view content, bool append) {
    std::ios_base::openmode mode = std::ios::out;
    if (append) {
        mode |= std::ios::app;
    }

    std::ofstream file(std::string(path), mode);
    if (!file.is_open()) {
        return false;
    }

    file << content;
    return file.good();
}

bool write_binary_file(std::string_view path, const std::vector<uint8_t>& data) {
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool copy_file(std::string_view source, std::string_view destination, bool overwrite) {
    std::error_code ec;
    auto options = overwrite ?
        fs::copy_options::overwrite_existing :
        fs::copy_options::none;

    fs::copy_file(fs::path(source), fs::path(destination), options, ec);
    return !ec;
}

bool move_file(std::string_view source, std::string_view destination) {
    std::error_code ec;
    fs::rename(fs::path(source), fs::path(destination), ec);
    return !ec;
}

// ============================================================================
// Platform-Specific Utilities
// ============================================================================

std::string get_home_directory() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path);
    }

    // Fallback to environment variables
    const char* home = std::getenv("USERPROFILE");
    if (home) {
        return std::string(home);
    }

    const char* homedrive = std::getenv("HOMEDRIVE");
    const char* homepath = std::getenv("HOMEPATH");
    if (homedrive && homepath) {
        return std::string(homedrive) + std::string(homepath);
    }

    return "C:\\";
#else
    // Unix-like systems
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home);
    }

    // Fallback to passwd database
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return std::string(pw->pw_dir);
    }

    return "/";
#endif
}

std::string get_current_directory() {
    std::error_code ec;
    return fs::current_path(ec).string();
}

bool set_current_directory(std::string_view path) {
    std::error_code ec;
    fs::current_path(fs::path(path), ec);
    return !ec;
}

char get_path_separator() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

std::string to_native_path(std::string_view path) {
    std::string result(path);

#ifdef _WIN32
    // Convert / to \\ on Windows
    std::replace(result.begin(), result.end(), '/', '\\');
#else
    // Convert \\ to / on Unix
    std::replace(result.begin(), result.end(), '\\', '/');
#endif

    return result;
}

std::string to_unix_path(std::string_view path) {
    std::string result(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

// ============================================================================
// Filesystem Encoding
// ============================================================================

std::string get_filesystem_encoding() {
    // Modern systems typically use UTF-8
    // Windows uses UTF-16 internally but std::filesystem handles conversion
    return "UTF-8";
}

std::string encode_filename(std::string_view filename) {
    // In C++17, std::filesystem handles encoding automatically
    // Just return the string as-is
    return std::string(filename);
}

} // namespace ytdlp::utils
