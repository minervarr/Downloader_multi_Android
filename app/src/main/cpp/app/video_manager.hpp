/**
 * @file video_manager.hpp
 * @brief Video download and management
 */

#ifndef VIDEO_MANAGER_HPP
#define VIDEO_MANAGER_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

namespace downloader {
namespace app {

/**
 * @brief Video format information
 */
struct VideoFormat {
    std::string format_id;
    std::string url;
    std::string ext;
    int width = 0;
    int height = 0;
    int64_t filesize = 0;
    int bitrate = 0;
    std::string codec;
    std::string quality;  // e.g., "1080p", "720p"
};

/**
 * @brief Video metadata
 */
struct VideoMetadata {
    std::string id;
    std::string title;
    std::string description;
    std::string thumbnail_url;
    int duration = 0;  // seconds
    std::string uploader;
    std::string upload_date;
    std::vector<VideoFormat> formats;
};

/**
 * @brief Download task
 */
struct DownloadTask {
    int64_t id;
    std::string url;
    std::string output_path;
    std::string format_id;  // Selected format
    std::atomic<float> progress{0.0f};
    std::atomic<int64_t> downloaded_bytes{0};
    std::atomic<int64_t> total_bytes{0};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> completed{false};
    std::atomic<bool> failed{false};
    std::string error_message;
};

/**
 * @brief Download progress callback
 */
using DownloadProgressCallback = std::function<void(int64_t task_id, float progress,
                                                     int64_t downloaded, int64_t total)>;

/**
 * @brief Download completion callback
 */
using DownloadCompleteCallback = std::function<void(int64_t task_id, bool success,
                                                     const std::string& error)>;

/**
 * @brief Video manager for handling downloads
 */
class VideoManager {
public:
    VideoManager();
    ~VideoManager();

    // Non-copyable
    VideoManager(const VideoManager&) = delete;
    VideoManager& operator=(const VideoManager&) = delete;

    /**
     * @brief Initialize video manager
     * @param download_dir Default download directory
     * @param max_concurrent Maximum concurrent downloads
     */
    void initialize(const std::string& download_dir, int max_concurrent = 3);

    /**
     * @brief Shutdown video manager
     */
    void shutdown();

    /**
     * @brief Parse video metadata from JSON
     * @param json_str JSON string from extractor
     * @return Parsed metadata
     */
    VideoMetadata parse_metadata(const std::string& json_str);

    /**
     * @brief Queue a download task
     * @param url Video URL
     * @param output_path Output file path
     * @param format_id Format ID to download (empty for best)
     * @return Task ID
     */
    int64_t queue_download(const std::string& url, const std::string& output_path,
                           const std::string& format_id = "");

    /**
     * @brief Cancel a download
     * @param task_id Task ID
     * @return true if cancelled
     */
    bool cancel_download(int64_t task_id);

    /**
     * @brief Pause a download
     * @param task_id Task ID
     * @return true if paused
     */
    bool pause_download(int64_t task_id);

    /**
     * @brief Resume a paused download
     * @param task_id Task ID
     * @return true if resumed
     */
    bool resume_download(int64_t task_id);

    /**
     * @brief Get download task
     * @param task_id Task ID
     * @return Task (or nullptr if not found)
     */
    std::shared_ptr<DownloadTask> get_task(int64_t task_id);

    /**
     * @brief Get all tasks
     */
    std::vector<std::shared_ptr<DownloadTask>> get_all_tasks();

    /**
     * @brief Set progress callback
     */
    void set_progress_callback(DownloadProgressCallback callback);

    /**
     * @brief Set completion callback
     */
    void set_complete_callback(DownloadCompleteCallback callback);

    /**
     * @brief Select best format from available formats
     * @param formats Available formats
     * @param max_height Maximum height (0 for no limit)
     * @return Best format or nullptr
     */
    static const VideoFormat* select_best_format(const std::vector<VideoFormat>& formats,
                                                  int max_height = 0);

private:
    void worker_thread();
    void process_task(std::shared_ptr<DownloadTask> task);

private:
    std::string m_download_dir;
    int m_max_concurrent = 3;
    std::atomic<bool> m_running{false};

    // Task management
    std::vector<std::shared_ptr<DownloadTask>> m_tasks;
    std::queue<std::shared_ptr<DownloadTask>> m_pending_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    // Worker threads
    std::vector<std::thread> m_workers;

    // Callbacks
    DownloadProgressCallback m_progress_callback;
    DownloadCompleteCallback m_complete_callback;

    // ID generator
    std::atomic<int64_t> m_next_id{1};
};

} // namespace app
} // namespace downloader

#endif // VIDEO_MANAGER_HPP
