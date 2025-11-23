/**
 * @file downloader_app.cpp
 * @brief Main application implementation
 */

#include "downloader_app.hpp"
#include <android/log.h>
#include <algorithm>

// ytdlp includes
#include <ytdlp/core/youtube_dl.hpp>
#include <ytdlp/extractor/zoom.hpp>

#ifdef DOWNLOADER_USE_VULKAN
#include "vulkan/vulkan_renderer.hpp"
#endif

#define LOG_TAG "DownloaderApp"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace downloader {
namespace app {

DownloaderApp::DownloaderApp(const AppConfig& config)
    : m_config(config) {
    LOGI("DownloaderApp created");
    LOGI("  Cache dir: %s", config.cache_dir.c_str());
    LOGI("  Files dir: %s", config.files_dir.c_str());
    LOGI("  Download dir: %s", config.download_dir.c_str());

    // Initialize ytdlp
    try {
        ytdlp::core::YoutubeDLParams params;
        params.quiet = false;
        m_ytdl = std::make_unique<ytdlp::core::YoutubeDL>(params);
        LOGI("ytdlp-cpp initialized");
    } catch (const std::exception& e) {
        LOGE("Failed to initialize ytdlp: %s", e.what());
    }
}

DownloaderApp::~DownloaderApp() {
    cleanup_vulkan();
    LOGI("DownloaderApp destroyed");
}

// =============================================================================
// Vulkan Rendering
// =============================================================================

bool DownloaderApp::init_vulkan(ANativeWindow* window, int width, int height) {
#ifdef DOWNLOADER_USE_VULKAN
    if (!window) {
        LOGE("Null window provided to init_vulkan");
        return false;
    }

    m_window = window;

    try {
        m_renderer = std::make_unique<vulkan::VulkanRenderer>();

        vulkan::RendererConfig renderer_config;
        renderer_config.width = static_cast<uint32_t>(width);
        renderer_config.height = static_cast<uint32_t>(height);
        renderer_config.enable_validation = m_config.enable_validation;
        renderer_config.app_name = "Downloader Multi";

        if (!m_renderer->initialize(window, renderer_config)) {
            LOGE("Failed to initialize Vulkan renderer");
            m_renderer.reset();
            return false;
        }

        LOGI("Vulkan renderer initialized: %dx%d", width, height);
        return true;
    } catch (const std::exception& e) {
        LOGE("Exception during Vulkan init: %s", e.what());
        m_renderer.reset();
        return false;
    }
#else
    LOGI("Vulkan support not compiled in");
    return false;
#endif
}

void DownloaderApp::cleanup_vulkan() {
#ifdef DOWNLOADER_USE_VULKAN
    if (m_renderer) {
        m_renderer->cleanup();
        m_renderer.reset();
        LOGI("Vulkan renderer cleaned up");
    }
#endif
}

void DownloaderApp::render_frame() {
#ifdef DOWNLOADER_USE_VULKAN
    if (!m_renderer || !m_renderer->is_initialized()) {
        return;
    }

    if (!m_renderer->begin_frame()) {
        return;
    }

    // Draw UI elements
    // Title
    m_renderer->draw_text("Downloader Multi", 20, 30, 0xFFFFFFFF, 24.0f);

    // Draw active downloads
    float y = 80;
    for (const auto& download : m_downloads) {
        // Download title
        m_renderer->draw_text(download.title.empty() ? download.url : download.title,
                              20, y, 0xFFCCCCCC, 16.0f);
        y += 25;

        // Progress bar
        m_renderer->draw_progress_bar(20, y, 400, 20, download.progress,
                                       0xFF333333, 0xFF00AA00);
        y += 30;

        // Status
        std::string status_text;
        switch (download.status) {
            case DownloadStatus::Pending:    status_text = "Pending"; break;
            case DownloadStatus::Downloading: status_text = "Downloading"; break;
            case DownloadStatus::Paused:     status_text = "Paused"; break;
            case DownloadStatus::Completed:  status_text = "Completed"; break;
            case DownloadStatus::Failed:     status_text = "Failed"; break;
            case DownloadStatus::Cancelled:  status_text = "Cancelled"; break;
        }
        m_renderer->draw_text(status_text + " - " +
                              std::to_string(static_cast<int>(download.progress * 100)) + "%",
                              20, y, 0xFF888888, 14.0f);
        y += 40;
    }

    // If no downloads, show message
    if (m_downloads.empty()) {
        m_renderer->draw_text("No active downloads", 20, 100, 0xFF666666, 18.0f);
        m_renderer->draw_text("Paste a video URL to start", 20, 130, 0xFF444444, 14.0f);
    }

    m_renderer->end_frame();
#endif
}

void DownloaderApp::resize(int width, int height) {
#ifdef DOWNLOADER_USE_VULKAN
    if (m_renderer) {
        m_renderer->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        LOGI("Renderer resized to %dx%d", width, height);
    }
#endif
}

// =============================================================================
// Video Extraction
// =============================================================================

std::string DownloaderApp::extract_video_info(const std::string& url) {
    if (!m_ytdl) {
        return "{\"error\": \"ytdlp not initialized\"}";
    }

    LOGI("Extracting info for: %s", url.c_str());

    try {
        // Check which extractor to use
        if (ytdlp::extractor::ZoomIE::suitable(url)) {
            ytdlp::extractor::ZoomIE zoom(m_ytdl.get());
            auto info = zoom.extract(url);
            return info.dump();
        }

        // No suitable extractor found
        return "{\"error\": \"No extractor found for this URL\"}";
    } catch (const std::exception& e) {
        LOGE("Extraction error: %s", e.what());
        return std::string("{\"error\": \"") + e.what() + "\"}";
    }
}

bool DownloaderApp::is_url_supported(const std::string& url) {
    // Check all supported extractors
    if (ytdlp::extractor::ZoomIE::suitable(url)) {
        return true;
    }

    // Add more extractors here as they become available
    return false;
}

// =============================================================================
// Download Management
// =============================================================================

int64_t DownloaderApp::start_download(const std::string& url, const std::string& output_path) {
    if (!m_ytdl) {
        LOGE("ytdlp not initialized");
        return -1;
    }

    // Create download info
    DownloadInfo info;
    info.id = m_next_download_id++;
    info.url = url;
    info.output_path = output_path;
    info.status = DownloadStatus::Pending;

    m_downloads.push_back(info);

    LOGI("Download started: id=%lld, url=%s", (long long)info.id, url.c_str());

    // TODO: Start actual download in background thread
    // For now, just mark as downloading
    m_downloads.back().status = DownloadStatus::Downloading;

    return info.id;
}

float DownloaderApp::get_download_progress(int64_t id) {
    auto it = std::find_if(m_downloads.begin(), m_downloads.end(),
                           [id](const DownloadInfo& d) { return d.id == id; });
    if (it != m_downloads.end()) {
        return it->progress;
    }
    return 0.0f;
}

bool DownloaderApp::cancel_download(int64_t id) {
    auto it = std::find_if(m_downloads.begin(), m_downloads.end(),
                           [id](const DownloadInfo& d) { return d.id == id; });
    if (it != m_downloads.end()) {
        it->status = DownloadStatus::Cancelled;
        LOGI("Download cancelled: id=%lld", (long long)id);
        return true;
    }
    return false;
}

std::string DownloaderApp::get_download_status(int64_t id) {
    auto it = std::find_if(m_downloads.begin(), m_downloads.end(),
                           [id](const DownloadInfo& d) { return d.id == id; });
    if (it == m_downloads.end()) {
        return "not_found";
    }

    switch (it->status) {
        case DownloadStatus::Pending:     return "pending";
        case DownloadStatus::Downloading: return "downloading";
        case DownloadStatus::Paused:      return "paused";
        case DownloadStatus::Completed:   return "completed";
        case DownloadStatus::Failed:      return "failed";
        case DownloadStatus::Cancelled:   return "cancelled";
        default:                          return "unknown";
    }
}

const DownloadInfo* DownloaderApp::get_download_info(int64_t id) const {
    auto it = std::find_if(m_downloads.begin(), m_downloads.end(),
                           [id](const DownloadInfo& d) { return d.id == id; });
    if (it != m_downloads.end()) {
        return &(*it);
    }
    return nullptr;
}

void DownloaderApp::set_progress_callback(ProgressCallback callback) {
    m_progress_callback = std::move(callback);
}

// =============================================================================
// Cookie Management
// =============================================================================

bool DownloaderApp::load_cookies(const std::string& cookie_file) {
    if (!m_ytdl) {
        LOGE("ytdlp not initialized");
        return false;
    }

    try {
        m_ytdl->load_cookies(cookie_file);
        LOGI("Cookies loaded from: %s", cookie_file.c_str());
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to load cookies: %s", e.what());
        return false;
    }
}

} // namespace app
} // namespace downloader
