/**
 * @file jni_bridge.cpp
 * @brief JNI bridge for Java-C++ communication
 *
 * This file provides the JNI interface between Java/Kotlin and the native
 * C++ code including ytdlp-cpp library, Vulkan renderer, and SDL3.
 */

#include <jni.h>
#include <string>
#include <memory>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

// ytdlp includes
#include <ytdlp/core/youtube_dl.hpp>
#include <ytdlp/extractor/zoom.hpp>
#include <ytdlp/utils/string_utils.hpp>
#include <ytdlp/utils/json_utils.hpp>

// Vulkan renderer
#ifdef DOWNLOADER_USE_VULKAN
#include "vulkan/vulkan_renderer.hpp"
#endif

// App components
#include "app/downloader_app.hpp"
#include "app/video_manager.hpp"

#define LOG_TAG "JNI_Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

namespace {
    // Global app instance
    std::unique_ptr<downloader::app::DownloaderApp> g_app;

    // Android paths (set from Java)
    std::string g_cache_dir;
    std::string g_files_dir;
    std::string g_external_dir;
}

extern "C" {

// =============================================================================
// Application Lifecycle
// =============================================================================

JNIEXPORT jboolean JNICALL
Java_io_nava_downloader_1multi_MainActivity_nativeInit(
        JNIEnv* env,
        jobject /* this */,
        jstring cache_dir,
        jstring files_dir,
        jstring external_dir) {

    // Store Android paths
    const char* cache = env->GetStringUTFChars(cache_dir, nullptr);
    const char* files = env->GetStringUTFChars(files_dir, nullptr);
    const char* external = env->GetStringUTFChars(external_dir, nullptr);

    g_cache_dir = cache;
    g_files_dir = files;
    g_external_dir = external;

    env->ReleaseStringUTFChars(cache_dir, cache);
    env->ReleaseStringUTFChars(files_dir, files);
    env->ReleaseStringUTFChars(external_dir, external);

    LOGI("Native init - cache: %s, files: %s, external: %s",
         g_cache_dir.c_str(), g_files_dir.c_str(), g_external_dir.c_str());

    // Create app instance
    try {
        downloader::app::AppConfig config;
        config.cache_dir = g_cache_dir;
        config.files_dir = g_files_dir;
        config.download_dir = g_external_dir + "/Downloads";

        g_app = std::make_unique<downloader::app::DownloaderApp>(config);
        LOGI("DownloaderApp created successfully");
        return JNI_TRUE;
    } catch (const std::exception& e) {
        LOGE("Failed to create DownloaderApp: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT void JNICALL
Java_io_nava_downloader_1multi_MainActivity_nativeCleanup(
        JNIEnv* /* env */,
        jobject /* this */) {
    LOGI("Native cleanup");
    g_app.reset();
}

// =============================================================================
// Vulkan Surface Management
// =============================================================================

#ifdef DOWNLOADER_USE_VULKAN
JNIEXPORT jboolean JNICALL
Java_io_nava_downloader_1multi_MainActivity_nativeInitVulkan(
        JNIEnv* env,
        jobject /* this */,
        jobject surface,
        jint width,
        jint height) {

    if (!g_app) {
        LOGE("App not initialized");
        return JNI_FALSE;
    }

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window from surface");
        return JNI_FALSE;
    }

    bool result = g_app->init_vulkan(window, width, height);
    LOGI("Vulkan init result: %d", result);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_io_nava_downloader_1multi_MainActivity_nativeCleanupVulkan(
        JNIEnv* /* env */,
        jobject /* this */) {
    if (g_app) {
        g_app->cleanup_vulkan();
    }
}

JNIEXPORT void JNICALL
Java_io_nava_downloader_1multi_MainActivity_nativeRenderFrame(
        JNIEnv* /* env */,
        jobject /* this */) {
    if (g_app) {
        g_app->render_frame();
    }
}

JNIEXPORT void JNICALL
Java_io_nava_downloader_1multi_MainActivity_nativeResize(
        JNIEnv* /* env */,
        jobject /* this */,
        jint width,
        jint height) {
    if (g_app) {
        g_app->resize(width, height);
    }
}
#endif

// =============================================================================
// Video Extraction (ytdlp-cpp)
// =============================================================================

JNIEXPORT jstring JNICALL
Java_io_nava_downloader_1multi_MainActivity_extractVideoInfo(
        JNIEnv* env,
        jobject /* this */,
        jstring url_j) {

    const char* url = env->GetStringUTFChars(url_j, nullptr);
    std::string url_str(url);
    env->ReleaseStringUTFChars(url_j, url);

    LOGI("Extracting video info for: %s", url_str.c_str());

    if (!g_app) {
        LOGE("App not initialized");
        return env->NewStringUTF("{\"error\": \"App not initialized\"}");
    }

    try {
        std::string result = g_app->extract_video_info(url_str);
        return env->NewStringUTF(result.c_str());
    } catch (const std::exception& e) {
        LOGE("Extraction failed: %s", e.what());
        std::string error = "{\"error\": \"" + std::string(e.what()) + "\"}";
        return env->NewStringUTF(error.c_str());
    }
}

JNIEXPORT jboolean JNICALL
Java_io_nava_downloader_1multi_MainActivity_isUrlSupported(
        JNIEnv* env,
        jobject /* this */,
        jstring url_j) {

    const char* url = env->GetStringUTFChars(url_j, nullptr);
    std::string url_str(url);
    env->ReleaseStringUTFChars(url_j, url);

    if (!g_app) {
        return JNI_FALSE;
    }

    return g_app->is_url_supported(url_str) ? JNI_TRUE : JNI_FALSE;
}

// =============================================================================
// Download Management
// =============================================================================

JNIEXPORT jlong JNICALL
Java_io_nava_downloader_1multi_MainActivity_startDownload(
        JNIEnv* env,
        jobject /* this */,
        jstring url_j,
        jstring output_path_j) {

    const char* url = env->GetStringUTFChars(url_j, nullptr);
    const char* output = env->GetStringUTFChars(output_path_j, nullptr);

    std::string url_str(url);
    std::string output_str(output);

    env->ReleaseStringUTFChars(url_j, url);
    env->ReleaseStringUTFChars(output_path_j, output);

    if (!g_app) {
        LOGE("App not initialized");
        return -1;
    }

    LOGI("Starting download: %s -> %s", url_str.c_str(), output_str.c_str());
    return g_app->start_download(url_str, output_str);
}

JNIEXPORT jfloat JNICALL
Java_io_nava_downloader_1multi_MainActivity_getDownloadProgress(
        JNIEnv* /* env */,
        jobject /* this */,
        jlong download_id) {

    if (!g_app) {
        return 0.0f;
    }

    return g_app->get_download_progress(download_id);
}

JNIEXPORT jboolean JNICALL
Java_io_nava_downloader_1multi_MainActivity_cancelDownload(
        JNIEnv* /* env */,
        jobject /* this */,
        jlong download_id) {

    if (!g_app) {
        return JNI_FALSE;
    }

    return g_app->cancel_download(download_id) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_io_nava_downloader_1multi_MainActivity_getDownloadStatus(
        JNIEnv* env,
        jobject /* this */,
        jlong download_id) {

    if (!g_app) {
        return env->NewStringUTF("error");
    }

    std::string status = g_app->get_download_status(download_id);
    return env->NewStringUTF(status.c_str());
}

// =============================================================================
// Cookie Management
// =============================================================================

JNIEXPORT jboolean JNICALL
Java_io_nava_downloader_1multi_MainActivity_loadCookies(
        JNIEnv* env,
        jobject /* this */,
        jstring cookie_file_j) {

    const char* cookie_file = env->GetStringUTFChars(cookie_file_j, nullptr);
    std::string cookie_file_str(cookie_file);
    env->ReleaseStringUTFChars(cookie_file_j, cookie_file);

    if (!g_app) {
        return JNI_FALSE;
    }

    LOGI("Loading cookies from: %s", cookie_file_str.c_str());
    return g_app->load_cookies(cookie_file_str) ? JNI_TRUE : JNI_FALSE;
}

// =============================================================================
// Utility Functions
// =============================================================================

JNIEXPORT jstring JNICALL
Java_io_nava_downloader_1multi_MainActivity_getVersion(
        JNIEnv* env,
        jobject /* this */) {
    return env->NewStringUTF("1.0.0-alpha");
}

JNIEXPORT jstring JNICALL
Java_io_nava_downloader_1multi_MainActivity_getSupportedSites(
        JNIEnv* env,
        jobject /* this */) {
    // Return JSON array of supported sites
    return env->NewStringUTF("[\"zoom.us\", \"bundesliga.com\"]");
}

} // extern "C"
