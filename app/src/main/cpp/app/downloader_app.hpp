/**
 * @file downloader_app.hpp
 * @brief Main application class for Downloader Multi
 */

#ifndef DOWNLOADER_APP_HPP
#define DOWNLOADER_APP_HPP

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <android/native_window.h>

// Forward declarations
namespace ytdlp {
namespace core {
    class YoutubeDL;
}
}

namespace downloader {

namespace vulkan {
    class VulkanRenderer;
}

namespace app {

/**
 * @brief Application configuration
 */
struct AppConfig {
    std::string cache_dir;
    std::string files_dir;
    std::string download_dir;
    bool enable_vulkan = true;
    bool enable_validation = false;
};

/**
 * @brief Download status
 */
enum class DownloadStatus {
    Pending,
    Downloading,
    Paused,
    Completed,
    Failed,
    Cancelled
};

/**
 * @brief Download information
 */
struct DownloadInfo {
    int64_t id;
    std::string url;
    std::string output_path;
    std::string title;
    float progress = 0.0f;
    int64_t downloaded_bytes = 0;
    int64_t total_bytes = 0;
    DownloadStatus status = DownloadStatus::Pending;
    std::string error_message;
};

/**
 * @brief Progress callback type
 */
using ProgressCallback = std::function<void(int64_t id, float progress, int64_t downloaded, int64_t total)>;

/**
 * @brief Main application class
 */
class DownloaderApp {
public:
    explicit DownloaderApp(const AppConfig& config);
    ~DownloaderApp();

    // Non-copyable
    DownloaderApp(const DownloaderApp&) = delete;
    DownloaderApp& operator=(const DownloaderApp&) = delete;

    // ==========================================================================
    // Vulkan Rendering
    // ==========================================================================

    /**
     * @brief Initialize Vulkan renderer
     * @param window Android native window
     * @param width Window width
     * @param height Window height
     * @return true if successful
     */
    bool init_vulkan(ANativeWindow* window, int width, int height);

    /**
     * @brief Cleanup Vulkan resources
     */
    void cleanup_vulkan();

    /**
     * @brief Render a frame
     */
    void render_frame();

    /**
     * @brief Handle window resize
     */
    void resize(int width, int height);

    // ==========================================================================
    // Video Extraction
    // ==========================================================================

    /**
     * @brief Extract video information from URL
     * @param url Video URL
     * @return JSON string with video info
     */
    std::string extract_video_info(const std::string& url);

    /**
     * @brief Check if URL is supported
     * @param url URL to check
     * @return true if supported
     */
    bool is_url_supported(const std::string& url);

    // ==========================================================================
    // Download Management
    // ==========================================================================

    /**
     * @brief Start a download
     * @param url Video URL
     * @param output_path Output file path
     * @return Download ID, or -1 on error
     */
    int64_t start_download(const std::string& url, const std::string& output_path);

    /**
     * @brief Get download progress
     * @param id Download ID
     * @return Progress (0.0 to 1.0)
     */
    float get_download_progress(int64_t id);

    /**
     * @brief Cancel a download
     * @param id Download ID
     * @return true if cancelled
     */
    bool cancel_download(int64_t id);

    /**
     * @brief Get download status string
     * @param id Download ID
     * @return Status string
     */
    std::string get_download_status(int64_t id);

    /**
     * @brief Get download info
     * @param id Download ID
     * @return Download info (or nullptr if not found)
     */
    const DownloadInfo* get_download_info(int64_t id) const;

    /**
     * @brief Set progress callback
     * @param callback Callback function
     */
    void set_progress_callback(ProgressCallback callback);

    // ==========================================================================
    // Cookie Management
    // ==========================================================================

    /**
     * @brief Load cookies from file (Netscape format)
     * @param cookie_file Path to cookie file
     * @return true if successful
     */
    bool load_cookies(const std::string& cookie_file);

    // ==========================================================================
    // Configuration
    // ==========================================================================

    /**
     * @brief Get current configuration
     */
    const AppConfig& config() const { return m_config; }

private:
    AppConfig m_config;

    // ytdlp-cpp instance
    std::unique_ptr<ytdlp::core::YoutubeDL> m_ytdl;

    // Vulkan renderer
#ifdef DOWNLOADER_USE_VULKAN
    std::unique_ptr<vulkan::VulkanRenderer> m_renderer;
#endif

    // Downloads
    std::vector<DownloadInfo> m_downloads;
    int64_t m_next_download_id = 1;
    ProgressCallback m_progress_callback;

    // Native window reference
    ANativeWindow* m_window = nullptr;
};

} // namespace app
} // namespace downloader

#endif // DOWNLOADER_APP_HPP
