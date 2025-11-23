# Downloader Multi Android - Claude Code Guide

## Project Overview

Android video downloader application with:
- **ytdlp-cpp**: C++17 video extraction library (from Downloader_Multi)
- **Vulkan**: Native GPU-accelerated UI rendering
- **SDL3**: Cross-platform window/input management (optional)

## Quick Commands

```bash
# Build debug APK
./gradlew assembleDebug

# Build release APK
./gradlew assembleRelease

# Clean build
./gradlew clean

# Run tests
./gradlew test
```

## Project Structure

```
app/src/main/
├── java/io/nava/downloader_multi/
│   └── MainActivity.java       # Main activity with JNI bindings
├── cpp/
│   ├── CMakeLists.txt          # Native build configuration
│   ├── native-lib.cpp          # Basic JNI functions
│   ├── jni_bridge.cpp          # Full JNI bridge
│   ├── app/
│   │   ├── downloader_app.hpp/cpp  # Main app class
│   │   └── video_manager.hpp/cpp   # Download management
│   ├── vulkan/
│   │   ├── vulkan_renderer.hpp/cpp # Vulkan rendering
│   │   └── vulkan_*.cpp            # Vulkan components
│   ├── ytdlp/                      # ytdlp-cpp library
│   │   ├── include/ytdlp/          # Public headers
│   │   └── src/                    # Implementation
│   └── extern/                     # Dependencies
│       ├── fmt/                    # String formatting
│       ├── spdlog/                 # Logging
│       └── json/                   # JSON parsing
└── res/                            # Android resources
```

## Key Components

### ytdlp-cpp Library
- Namespace: `ytdlp::`
- Core: `ytdlp::core::YoutubeDL`, `ytdlp::core::InfoDict`
- Extractors: `ytdlp::extractor::ZoomIE` (production-ready)
- Networking: `ytdlp::networking::CurlHttpClient`
- Utils: `ytdlp::utils::string_utils`, `json_utils`, etc.

### Vulkan Renderer
- Namespace: `downloader::vulkan::`
- Main class: `VulkanRenderer`
- UI elements: Text, Rect, ProgressBar

### App Layer
- Namespace: `downloader::app::`
- Main class: `DownloaderApp`
- Download management: `VideoManager`

## Adding New Extractors

1. Create header in `ytdlp/include/ytdlp/extractor/`
2. Create implementation in `ytdlp/src/extractor/`
3. Add to `CMakeLists.txt` YTDLP_SOURCES
4. Register in `DownloaderApp::is_url_supported()`

## Dependencies

### Header-only (bundled)
- fmt (FMT_HEADER_ONLY)
- spdlog (SPDLOG_HEADER_ONLY)
- nlohmann/json

### System (need prebuilts for Android)
- libcurl - HTTP client
- OpenSSL - SSL/crypto

## Build Flags

| Flag | Description |
|------|-------------|
| `DOWNLOADER_USE_VULKAN` | Enable Vulkan renderer |
| `DOWNLOADER_USE_SDL3` | Enable SDL3 integration |
| `YTDLP_NO_CURL` | Stub curl (no networking) |
| `YTDLP_NO_OPENSSL` | Stub OpenSSL (no crypto) |

## JNI Interface

Key native methods in `MainActivity.java`:
- `nativeInit()` - Initialize app
- `extractVideoInfo()` - Extract video metadata
- `startDownload()` - Begin download
- `nativeInitVulkan()` - Initialize renderer

## Code Style

- C++17 standard
- RAII for resource management
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Android logging macros: `LOGI`, `LOGE`, `LOGD`
