# Downloader Multi Android - Claude Code Guide

## Project Overview

Pure C++ Android video downloader application with:
- **SDL3**: Cross-platform window/input management (from submodule)
- **ytdlp-cpp**: C++17 video extraction library
- **Vulkan**: Native GPU-accelerated UI rendering
- **Minimal Java**: Only SDL3's SDLActivity (single file)

## Quick Commands

```bash
# Initialize submodules (required after clone)
git submodule update --init --recursive

# Build debug APK
./gradlew assembleDebug

# Build release APK
./gradlew assembleRelease

# Clean build
./gradlew clean
```

## Project Structure

```
app/src/main/
├── java/io/nava/downloader_multi/
│   └── MainActivity.java       # Minimal - extends SDLActivity
├── cpp/
│   ├── CMakeLists.txt          # Native build configuration
│   ├── main.cpp                # SDL3 main entry point (SDL_main)
│   ├── app/
│   │   ├── downloader_app.hpp/cpp  # Main app class
│   │   └── video_manager.hpp/cpp   # Download management
│   ├── vulkan/
│   │   ├── vulkan_renderer.hpp/cpp # Vulkan rendering
│   │   └── vulkan_*.cpp            # Vulkan components
│   ├── ytdlp/                      # ytdlp-cpp library
│   │   ├── include/ytdlp/          # Public headers
│   │   └── src/                    # Implementation
│   └── extern/                     # Git submodules
│       ├── SDL3/                   # SDL3 (window/input/audio)
│       ├── fmt/                    # String formatting
│       ├── spdlog/                 # Logging
│       └── json/                   # JSON parsing
└── res/                            # Android resources
```

## Key Components

### SDL3 Entry Point (main.cpp)
- Uses `SDL_main()` as entry point
- Creates window with Vulkan support
- Handles event loop and input
- Manages application lifecycle

### ytdlp-cpp Library
- Namespace: `ytdlp::`
- Core: `ytdlp::core::YoutubeDL`, `ytdlp::core::InfoDict`
- Extractors: `ytdlp::extractor::ZoomIE` (production-ready)
- Networking: `ytdlp::networking::CurlHttpClient`

### Vulkan Renderer
- Namespace: `downloader::vulkan::`
- Main class: `VulkanRenderer`
- UI elements: Text, Rect, ProgressBar

### App Layer
- Namespace: `downloader::app::`
- Main class: `DownloaderApp`
- Download management: `VideoManager`

## Git Submodules

| Submodule | Path | Purpose |
|-----------|------|---------|
| SDL3 | `extern/SDL3` | Window/input/audio |
| fmt | `extern/fmt` | String formatting (header-only) |
| spdlog | `extern/spdlog` | Logging (header-only) |
| json | `extern/json` | JSON parsing (header-only) |

## Build Flags

| Flag | Description |
|------|-------------|
| `DOWNLOADER_USE_VULKAN` | Enable Vulkan renderer |
| `DOWNLOADER_USE_SDL3` | Enable SDL3 (always ON) |
| `YTDLP_NO_CURL` | Stub curl (no networking) |
| `YTDLP_NO_OPENSSL` | Stub OpenSSL (no crypto) |

## Code Style

- C++17 standard
- Pure C++ application logic
- RAII for resource management
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Android logging: `LOGI`, `LOGE`, `LOGD` macros
