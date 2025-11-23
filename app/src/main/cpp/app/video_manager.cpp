/**
 * @file video_manager.cpp
 * @brief Video manager implementation
 */

#include "video_manager.hpp"
#include <android/log.h>
#include <nlohmann/json.hpp>
#include <algorithm>

#define LOG_TAG "VideoManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace downloader {
namespace app {

VideoManager::VideoManager() = default;

VideoManager::~VideoManager() {
    shutdown();
}

void VideoManager::initialize(const std::string& download_dir, int max_concurrent) {
    m_download_dir = download_dir;
    m_max_concurrent = max_concurrent;
    m_running = true;

    LOGI("VideoManager initialized: download_dir=%s, max_concurrent=%d",
         download_dir.c_str(), max_concurrent);

    // Start worker threads
    for (int i = 0; i < max_concurrent; ++i) {
        m_workers.emplace_back(&VideoManager::worker_thread, this);
    }
}

void VideoManager::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
    m_cv.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();

    LOGI("VideoManager shutdown");
}

VideoMetadata VideoManager::parse_metadata(const std::string& json_str) {
    VideoMetadata metadata;

    try {
        auto json = nlohmann::json::parse(json_str);

        metadata.id = json.value("id", "");
        metadata.title = json.value("title", "");
        metadata.description = json.value("description", "");
        metadata.thumbnail_url = json.value("thumbnail", "");
        metadata.duration = json.value("duration", 0);
        metadata.uploader = json.value("uploader", "");
        metadata.upload_date = json.value("upload_date", "");

        // Parse formats
        if (json.contains("formats") && json["formats"].is_array()) {
            for (const auto& fmt : json["formats"]) {
                VideoFormat format;
                format.format_id = fmt.value("format_id", "");
                format.url = fmt.value("url", "");
                format.ext = fmt.value("ext", "");
                format.width = fmt.value("width", 0);
                format.height = fmt.value("height", 0);
                format.filesize = fmt.value("filesize", 0);
                format.bitrate = fmt.value("tbr", 0);
                format.codec = fmt.value("vcodec", "");
                format.quality = fmt.value("format_note", "");

                if (!format.url.empty()) {
                    metadata.formats.push_back(format);
                }
            }
        }

        LOGI("Parsed metadata: id=%s, title=%s, formats=%zu",
             metadata.id.c_str(), metadata.title.c_str(), metadata.formats.size());

    } catch (const std::exception& e) {
        LOGE("Failed to parse metadata: %s", e.what());
    }

    return metadata;
}

int64_t VideoManager::queue_download(const std::string& url, const std::string& output_path,
                                      const std::string& format_id) {
    auto task = std::make_shared<DownloadTask>();
    task->id = m_next_id++;
    task->url = url;
    task->output_path = output_path;
    task->format_id = format_id;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push_back(task);
        m_pending_queue.push(task);
    }
    m_cv.notify_one();

    LOGI("Download queued: id=%lld, url=%s", (long long)task->id, url.c_str());
    return task->id;
}

bool VideoManager::cancel_download(int64_t task_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
                           [task_id](const std::shared_ptr<DownloadTask>& t) {
                               return t->id == task_id;
                           });

    if (it != m_tasks.end()) {
        (*it)->cancelled = true;
        LOGI("Download cancelled: id=%lld", (long long)task_id);
        return true;
    }
    return false;
}

bool VideoManager::pause_download(int64_t task_id) {
    // TODO: Implement pause functionality
    LOGI("Pause not yet implemented for task: %lld", (long long)task_id);
    return false;
}

bool VideoManager::resume_download(int64_t task_id) {
    // TODO: Implement resume functionality
    LOGI("Resume not yet implemented for task: %lld", (long long)task_id);
    return false;
}

std::shared_ptr<DownloadTask> VideoManager::get_task(int64_t task_id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
                           [task_id](const std::shared_ptr<DownloadTask>& t) {
                               return t->id == task_id;
                           });

    return (it != m_tasks.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<DownloadTask>> VideoManager::get_all_tasks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks;
}

void VideoManager::set_progress_callback(DownloadProgressCallback callback) {
    m_progress_callback = std::move(callback);
}

void VideoManager::set_complete_callback(DownloadCompleteCallback callback) {
    m_complete_callback = std::move(callback);
}

const VideoFormat* VideoManager::select_best_format(const std::vector<VideoFormat>& formats,
                                                     int max_height) {
    if (formats.empty()) return nullptr;

    const VideoFormat* best = nullptr;
    int best_score = -1;

    for (const auto& fmt : formats) {
        // Skip if exceeds max height
        if (max_height > 0 && fmt.height > max_height) {
            continue;
        }

        // Score based on resolution and bitrate
        int score = fmt.height * 10 + fmt.width + fmt.bitrate / 1000;

        if (score > best_score) {
            best_score = score;
            best = &fmt;
        }
    }

    return best;
}

void VideoManager::worker_thread() {
    LOGD("Worker thread started");

    while (m_running) {
        std::shared_ptr<DownloadTask> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {
                return !m_running || !m_pending_queue.empty();
            });

            if (!m_running) break;

            if (!m_pending_queue.empty()) {
                task = m_pending_queue.front();
                m_pending_queue.pop();
            }
        }

        if (task) {
            process_task(task);
        }
    }

    LOGD("Worker thread exiting");
}

void VideoManager::process_task(std::shared_ptr<DownloadTask> task) {
    LOGI("Processing download: id=%lld, url=%s", (long long)task->id, task->url.c_str());

    // TODO: Implement actual download logic using ytdlp HTTP client
    // For now, simulate progress

    for (int i = 0; i <= 100 && !task->cancelled; i += 10) {
        task->progress = static_cast<float>(i) / 100.0f;
        task->downloaded_bytes = i * 1024;
        task->total_bytes = 100 * 1024;

        if (m_progress_callback) {
            m_progress_callback(task->id, task->progress.load(),
                              task->downloaded_bytes.load(), task->total_bytes.load());
        }

        // Simulate download time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (task->cancelled) {
        LOGI("Download cancelled: id=%lld", (long long)task->id);
        if (m_complete_callback) {
            m_complete_callback(task->id, false, "Cancelled");
        }
    } else {
        task->completed = true;
        LOGI("Download completed: id=%lld", (long long)task->id);
        if (m_complete_callback) {
            m_complete_callback(task->id, true, "");
        }
    }
}

} // namespace app
} // namespace downloader
