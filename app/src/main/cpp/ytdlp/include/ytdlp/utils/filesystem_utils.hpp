/**
 * Filesystem Utilities Header
 *
 * Provides cross-platform filesystem utilities including path manipulation,
 * file/directory operations, file I/O, and platform-specific helpers.
 */

#ifndef YTDLP_UTILS_FILESYSTEM_UTILS_HPP
#define YTDLP_UTILS_FILESYSTEM_UTILS_HPP

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>
#include <filesystem>

namespace ytdlp::utils {

// Type alias for convenience
namespace fs = std::filesystem;

// ============================================================================
// Path Manipulation
// ============================================================================

/**
 * Expand path with ~ and environment variables
 * @param path Path to expand
 * @return Expanded path
 */
std::string expand_path(std::string_view path);

/**
 * Convert path to absolute path
 * @param path Relative or absolute path
 * @return Absolute path
 */
std::string absolute_path(std::string_view path);

/**
 * Normalize path (resolve .., ., symbolic links)
 * @param path Path to normalize
 * @return Normalized path
 */
std::string normalize_path(std::string_view path);

/**
 * Join multiple path components
 * @param parts Path components to join
 * @return Joined path
 */
std::string join_path(const std::vector<std::string>& parts);

/**
 * Join two path components
 * @param base Base path
 * @param relative Path to append
 * @return Joined path
 */
std::string join_path(std::string_view base, std::string_view relative);

/**
 * Get directory name of path
 * @param path File path
 * @return Directory path
 */
std::string dirname(std::string_view path);

/**
 * Get base filename of path
 * @param path File path
 * @return Base filename
 */
std::string basename(std::string_view path);

/**
 * Get file extension (including dot)
 * @param path File path
 * @return Extension (e.g., ".txt")
 */
std::string get_extension(std::string_view path);

/**
 * Get filename without extension
 * @param path File path
 * @return Filename stem
 */
std::string get_stem(std::string_view path);

/**
 * Replace file extension
 * @param path File path
 * @param new_ext New extension (with or without leading dot)
 * @return Path with new extension
 */
std::string replace_extension(std::string_view path, std::string_view new_ext);

/**
 * Prepend extension to filename (before existing extension)
 * @param path File path
 * @param ext Extension to prepend
 * @return Path with prepended extension
 */
std::string prepend_extension(std::string_view path, std::string_view ext);

// ============================================================================
// File/Directory Operations
// ============================================================================

/**
 * Check if path exists
 * @param path Path to check
 * @return True if path exists
 */
bool path_exists(std::string_view path);

/**
 * Check if path is a regular file
 * @param path Path to check
 * @return True if path is a file
 */
bool is_file(std::string_view path);

/**
 * Check if path is a directory
 * @param path Path to check
 * @return True if path is a directory
 */
bool is_directory(std::string_view path);

/**
 * Create directory (and parent directories if needed)
 * @param path Directory path
 * @return True if successful
 */
bool make_directory(std::string_view path);

/**
 * Remove file
 * @param path File path
 * @return True if successful
 */
bool remove_file(std::string_view path);

/**
 * Remove directory and all contents
 * @param path Directory path
 * @return True if successful
 */
bool remove_directory(std::string_view path);

/**
 * Get file size in bytes
 * @param path File path
 * @return File size or nullopt if error
 */
std::optional<int64_t> file_size(std::string_view path);

/**
 * Get file modification time (Unix timestamp)
 * @param path File path
 * @return Modification time or nullopt if error
 */
std::optional<int64_t> file_mtime(std::string_view path);

/**
 * List files and directories in directory
 * @param path Directory path
 * @param files_only If true, only list regular files
 * @return Vector of filenames (not full paths)
 */
std::vector<std::string> list_directory(std::string_view path, bool files_only = false);

/**
 * List files and directories in directory (full paths)
 * @param path Directory path
 * @param files_only If true, only list regular files
 * @return Vector of full file paths
 */
std::vector<std::string> list_directory_full(std::string_view path, bool files_only = false);

/**
 * Find files recursively matching pattern
 * @param path Root directory
 * @param pattern Glob pattern (e.g., "*.mp4")
 * @return Vector of matching file paths
 */
std::vector<std::string> find_files(std::string_view path, std::string_view pattern = "");

// ============================================================================
// Filename Generation
// ============================================================================

/**
 * Generate subtitle filename
 * @param video_filename Original video filename
 * @param language Language code (e.g., "en")
 * @param format Subtitle format (e.g., "srt", ".vtt")
 * @return Subtitle filename
 */
std::string subtitles_filename(
    std::string_view video_filename,
    std::string_view language,
    std::string_view format
);

/**
 * Generate temporary filename
 * @param base_path Base filename
 * @param prefix Prefix for temp file
 * @return Unique temporary filename
 */
std::string temp_filename(std::string_view base_path, std::string_view prefix = "tmp");

/**
 * Get system temporary directory
 * @return Path to temp directory
 */
std::string get_temp_directory();

// ============================================================================
// File I/O Helpers
// ============================================================================

/**
 * Read entire file as string
 * @param path File path
 * @return File contents or nullopt if error
 */
std::optional<std::string> read_file(std::string_view path);

/**
 * Read entire file as binary data
 * @param path File path
 * @return Binary data or nullopt if error
 */
std::optional<std::vector<uint8_t>> read_binary_file(std::string_view path);

/**
 * Write string to file
 * @param path File path
 * @param content Content to write
 * @param append If true, append to file instead of overwriting
 * @return True if successful
 */
bool write_file(std::string_view path, std::string_view content, bool append = false);

/**
 * Write binary data to file
 * @param path File path
 * @param data Binary data to write
 * @return True if successful
 */
bool write_binary_file(std::string_view path, const std::vector<uint8_t>& data);

/**
 * Copy file
 * @param source Source file path
 * @param destination Destination file path
 * @param overwrite If true, overwrite existing file
 * @return True if successful
 */
bool copy_file(std::string_view source, std::string_view destination, bool overwrite = false);

/**
 * Move/rename file
 * @param source Source file path
 * @param destination Destination file path
 * @return True if successful
 */
bool move_file(std::string_view source, std::string_view destination);

// ============================================================================
// Platform-Specific Utilities
// ============================================================================

/**
 * Get user's home directory
 * @return Home directory path
 */
std::string get_home_directory();

/**
 * Get current working directory
 * @return Current directory path
 */
std::string get_current_directory();

/**
 * Set current working directory
 * @param path New working directory
 * @return True if successful
 */
bool set_current_directory(std::string_view path);

/**
 * Get platform path separator ('/' on Unix, '\\' on Windows)
 * @return Path separator character
 */
char get_path_separator();

/**
 * Convert path to native format for platform
 * @param path Path to convert
 * @return Path with platform-appropriate separators
 */
std::string to_native_path(std::string_view path);

/**
 * Convert path to Unix format (forward slashes)
 * @param path Path to convert
 * @return Path with forward slashes
 */
std::string to_unix_path(std::string_view path);

// ============================================================================
// Filesystem Encoding
// ============================================================================

/**
 * Get filesystem encoding
 * @return Encoding name (typically "UTF-8")
 */
std::string get_filesystem_encoding();

/**
 * Encode filename for filesystem
 * @param filename Filename to encode
 * @return Encoded filename
 */
std::string encode_filename(std::string_view filename);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_FILESYSTEM_UTILS_HPP
