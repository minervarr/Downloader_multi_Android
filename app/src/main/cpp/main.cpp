/**
 * @file main.cpp
 * @brief SDL3-based main entry point for Downloader Multi
 *
 * This uses SDL3's SDL_main for a pure C++ Android application
 * with minimal Java code (only SDL's required activity).
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <memory>
#include <string>

#include "app/downloader_app.hpp"
#include "vulkan/vulkan_renderer.hpp"

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "DownloaderMulti"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#define LOGE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#define LOGD(...) printf(__VA_ARGS__); printf("\n")
#endif

// Application state
struct AppState {
    SDL_Window* window = nullptr;
    std::unique_ptr<downloader::app::DownloaderApp> app;
    bool running = true;
    int width = 1280;
    int height = 720;
};

static AppState g_state;

// Get Android internal storage path
static std::string get_internal_path() {
#ifdef __ANDROID__
    const char* path = SDL_GetPrefPath("io.nava", "downloader_multi");
    if (path) {
        std::string result(path);
        SDL_free((void*)path);
        return result;
    }
#endif
    return "./";
}

// Initialize the application
static bool init_app() {
    LOGI("Initializing Downloader Multi...");

    // Initialize SDL with video and events
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        LOGE("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    // Create window with Vulkan support
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Downloader Multi");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, g_state.width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, g_state.height);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);

    g_state.window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if (!g_state.window) {
        LOGE("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Get actual window size
    SDL_GetWindowSize(g_state.window, &g_state.width, &g_state.height);
    LOGI("Window created: %dx%d", g_state.width, g_state.height);

    // Initialize downloader app
    std::string base_path = get_internal_path();
    downloader::app::AppConfig config;
    config.cache_dir = base_path + "cache/";
    config.files_dir = base_path + "files/";
    config.download_dir = base_path + "downloads/";
    config.enable_vulkan = true;

    try {
        g_state.app = std::make_unique<downloader::app::DownloaderApp>(config);
        LOGI("DownloaderApp created");
    } catch (const std::exception& e) {
        LOGE("Failed to create DownloaderApp: %s", e.what());
        SDL_DestroyWindow(g_state.window);
        SDL_Quit();
        return false;
    }

    // Initialize Vulkan through SDL
#ifdef DOWNLOADER_USE_VULKAN
    // Get native window handle for Vulkan
    SDL_PropertiesID win_props = SDL_GetWindowProperties(g_state.window);

#ifdef __ANDROID__
    ANativeWindow* native_window = (ANativeWindow*)SDL_GetPointerProperty(
        win_props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);

    if (native_window) {
        if (!g_state.app->init_vulkan(native_window, g_state.width, g_state.height)) {
            LOGE("Failed to initialize Vulkan");
            // Continue without Vulkan - fall back to basic rendering
        }
    }
#endif
#endif

    LOGI("Application initialized successfully");
    return true;
}

// Handle SDL events
static void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                LOGI("Quit event received");
                g_state.running = false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                g_state.width = event.window.data1;
                g_state.height = event.window.data2;
                LOGI("Window resized: %dx%d", g_state.width, g_state.height);
                if (g_state.app) {
                    g_state.app->resize(g_state.width, g_state.height);
                }
                break;

            case SDL_EVENT_WINDOW_MINIMIZED:
                LOGD("Window minimized");
                break;

            case SDL_EVENT_WINDOW_RESTORED:
                LOGD("Window restored");
                break;

            case SDL_EVENT_FINGER_DOWN:
                LOGD("Touch at %.2f, %.2f", event.tfinger.x, event.tfinger.y);
                // TODO: Handle touch input
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_AC_BACK) {
                    LOGI("Back/Escape pressed");
                    g_state.running = false;
                }
                break;

            default:
                break;
        }
    }
}

// Render a frame
static void render_frame() {
    if (g_state.app) {
        g_state.app->render_frame();
    }
}

// Cleanup
static void cleanup() {
    LOGI("Cleaning up...");

    if (g_state.app) {
        g_state.app->cleanup_vulkan();
        g_state.app.reset();
    }

    if (g_state.window) {
        SDL_DestroyWindow(g_state.window);
        g_state.window = nullptr;
    }

    SDL_Quit();
    LOGI("Cleanup complete");
}

// SDL main entry point
int SDL_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    LOGI("=== Downloader Multi v1.0.0 ===");
    LOGI("SDL3 + Vulkan + ytdlp-cpp");

    if (!init_app()) {
        LOGE("Failed to initialize application");
        return 1;
    }

    // Main loop
    LOGI("Entering main loop...");
    while (g_state.running) {
        handle_events();
        render_frame();

        // Cap framerate (~60 FPS)
        SDL_Delay(16);
    }

    cleanup();
    return 0;
}
