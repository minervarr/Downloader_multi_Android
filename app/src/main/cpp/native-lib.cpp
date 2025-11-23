/**
 * @file native-lib.cpp
 * @brief Native library entry point and basic JNI functions
 *
 * This file provides basic JNI functions for compatibility with the original
 * Android project structure. Main functionality is in jni_bridge.cpp
 */

#include <jni.h>
#include <string>
#include <android/log.h>

#define LOG_TAG "NativeLib"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" {

/**
 * @brief Original stringFromJNI function for compatibility
 */
JNIEXPORT jstring JNICALL
Java_io_nava_downloader_1multi_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

    std::string hello = "Downloader Multi v1.0.0 - SDL3 + Vulkan + ytdlp-cpp";
    LOGI("stringFromJNI called");
    return env->NewStringUTF(hello.c_str());
}

/**
 * @brief Get library build info
 */
JNIEXPORT jstring JNICALL
Java_io_nava_downloader_1multi_MainActivity_getBuildInfo(
        JNIEnv* env,
        jobject /* this */) {

    std::string info = "Build: ";

#ifdef DOWNLOADER_USE_VULKAN
    info += "Vulkan=ON ";
#else
    info += "Vulkan=OFF ";
#endif

#ifdef DOWNLOADER_USE_SDL3
    info += "SDL3=ON ";
#else
    info += "SDL3=OFF ";
#endif

#ifdef YTDLP_NO_CURL
    info += "curl=STUB ";
#else
    info += "curl=ON ";
#endif

#ifdef YTDLP_NO_OPENSSL
    info += "OpenSSL=STUB ";
#else
    info += "OpenSSL=ON ";
#endif

    info += "C++17";

    LOGI("Build info: %s", info.c_str());
    return env->NewStringUTF(info.c_str());
}

} // extern "C"
