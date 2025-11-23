# =============================================================================
# Downloader Multi - Android NDK Build Configuration
# SDL3 + Vulkan + ytdlp-cpp Integration
# =============================================================================

cmake_minimum_required(VERSION 3.22.1)
project("downloader_multi" VERSION 1.0.0 LANGUAGES CXX C)

# =============================================================================
# C++ Standard Configuration
# =============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Android-specific settings
set(CMAKE_ANDROID_STL_TYPE c++_shared)

# =============================================================================
# Build Options
# =============================================================================
option(DOWNLOADER_USE_VULKAN "Enable Vulkan renderer" ON)
option(DOWNLOADER_USE_SDL3 "Enable SDL3 integration" ON)
option(DOWNLOADER_BUILD_TESTS "Build tests" OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

message(STATUS "")
message(STATUS "=== Downloader Multi Android Build ===")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Android ABI: ${ANDROID_ABI}")
message(STATUS "  Android NDK: ${ANDROID_NDK}")
message(STATUS "")

# =============================================================================
# External Dependencies (Header-only for Android)
# =============================================================================
message(STATUS "=== Configuring Dependencies ===")

# fmt - Header only mode for Android
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/fmt/include")
    add_library(fmt INTERFACE)
    target_include_directories(fmt INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/fmt/include
    )
    target_compile_definitions(fmt INTERFACE FMT_HEADER_ONLY=1)
    add_library(fmt::fmt ALIAS fmt)
    message(STATUS "  [OK] fmt (header-only)")
else()
    message(STATUS "  [SKIP] fmt - will use bundled or fetch")
endif()

# spdlog - Header only mode for Android
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog/include")
    add_library(spdlog INTERFACE)
    target_include_directories(spdlog INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog/include
    )
    target_compile_definitions(spdlog INTERFACE
        SPDLOG_HEADER_ONLY=1
        SPDLOG_FMT_EXTERNAL=1
    )
    if(TARGET fmt::fmt)
        target_link_libraries(spdlog INTERFACE fmt::fmt)
    endif()
    add_library(spdlog::spdlog ALIAS spdlog)
    message(STATUS "  [OK] spdlog (header-only)")
else()
    message(STATUS "  [SKIP] spdlog - will use bundled or fetch")
endif()

# nlohmann/json - Header only
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/json/include")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/json/include
    )
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    message(STATUS "  [OK] nlohmann_json (header-only)")
else()
    message(STATUS "  [SKIP] nlohmann_json - will use bundled or fetch")
endif()

# =============================================================================
# Prebuilt Libraries for Android (curl, openssl)
# =============================================================================

# Check for prebuilt curl
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/curl/${ANDROID_ABI}")
    add_library(curl_imported SHARED IMPORTED)
    set_target_properties(curl_imported PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/extern/curl/${ANDROID_ABI}/libcurl.so"
    )
    target_include_directories(curl_imported INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/curl/include"
    )
    add_library(CURL::libcurl ALIAS curl_imported)
    message(STATUS "  [OK] curl (prebuilt for ${ANDROID_ABI})")
    set(HAVE_CURL TRUE)
else()
    message(STATUS "  [STUB] curl - using stub implementation")
    set(HAVE_CURL FALSE)
endif()

# Check for prebuilt openssl
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/openssl/${ANDROID_ABI}")
    add_library(ssl_imported SHARED IMPORTED)
    set_target_properties(ssl_imported PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/extern/openssl/${ANDROID_ABI}/libssl.so"
    )
    add_library(crypto_imported SHARED IMPORTED)
    set_target_properties(crypto_imported PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/extern/openssl/${ANDROID_ABI}/libcrypto.so"
    )
    target_include_directories(ssl_imported INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/openssl/include"
    )
    add_library(OpenSSL::SSL ALIAS ssl_imported)
    add_library(OpenSSL::Crypto ALIAS crypto_imported)
    message(STATUS "  [OK] openssl (prebuilt for ${ANDROID_ABI})")
    set(HAVE_OPENSSL TRUE)
else()
    message(STATUS "  [STUB] openssl - using stub implementation")
    set(HAVE_OPENSSL FALSE)
endif()

# =============================================================================
# ytdlp-cpp Library (Video Extraction)
# =============================================================================
message(STATUS "")
message(STATUS "=== Building ytdlp-cpp Library ===")

set(YTDLP_SOURCES
    ytdlp/src/core/youtube_dl.cpp
    ytdlp/src/networking/curl_http_client.cpp
    ytdlp/src/networking/request.cpp
    ytdlp/src/networking/response.cpp
    ytdlp/src/networking/cookie.cpp
    ytdlp/src/networking/cookie_jar.cpp
    ytdlp/src/networking/http_header_dict.cpp
    ytdlp/src/extractor/info_extractor.cpp
    ytdlp/src/extractor/zoom.cpp
    ytdlp/src/utils/string_utils.cpp
    ytdlp/src/utils/json_utils.cpp
    ytdlp/src/utils/filesystem_utils.cpp
    ytdlp/src/utils/network_utils.cpp
    ytdlp/src/utils/date_utils.cpp
    ytdlp/src/utils/number_utils.cpp
    ytdlp/src/utils/parse_utils.cpp
    ytdlp/src/utils/encode_utils.cpp
)

add_library(ytdlp_static STATIC ${YTDLP_SOURCES})

target_include_directories(ytdlp_static
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/ytdlp/include
)

target_compile_definitions(ytdlp_static PRIVATE
    YTDLP_ANDROID=1
)

# Link dependencies (use stubs if prebuilts not available)
if(TARGET fmt::fmt)
    target_link_libraries(ytdlp_static PUBLIC fmt::fmt)
endif()
if(TARGET spdlog::spdlog)
    target_link_libraries(ytdlp_static PUBLIC spdlog::spdlog)
endif()
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(ytdlp_static PUBLIC nlohmann_json::nlohmann_json)
endif()
if(HAVE_CURL)
    target_link_libraries(ytdlp_static PUBLIC CURL::libcurl)
else()
    target_compile_definitions(ytdlp_static PRIVATE YTDLP_NO_CURL=1)
endif()
if(HAVE_OPENSSL)
    target_link_libraries(ytdlp_static PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else()
    target_compile_definitions(ytdlp_static PRIVATE YTDLP_NO_OPENSSL=1)
endif()

set_target_properties(ytdlp_static PROPERTIES
    OUTPUT_NAME ytdlp
    POSITION_INDEPENDENT_CODE ON
)

add_library(ytdlp::ytdlp ALIAS ytdlp_static)
message(STATUS "  [OK] ytdlp_static library")

# =============================================================================
# Vulkan Renderer
# =============================================================================
if(DOWNLOADER_USE_VULKAN)
    message(STATUS "")
    message(STATUS "=== Building Vulkan Renderer ===")

    set(VULKAN_SOURCES
        vulkan/vulkan_renderer.cpp
        vulkan/vulkan_context.cpp
        vulkan/vulkan_pipeline.cpp
        vulkan/vulkan_swapchain.cpp
    )

    add_library(vulkan_renderer STATIC ${VULKAN_SOURCES})

    target_include_directories(vulkan_renderer
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/vulkan
    )

    # Find Vulkan from Android NDK
    find_library(VULKAN_LIB vulkan)
    if(VULKAN_LIB)
        target_link_libraries(vulkan_renderer PUBLIC ${VULKAN_LIB})
        message(STATUS "  [OK] Vulkan library found: ${VULKAN_LIB}")
    else()
        message(WARNING "  [WARN] Vulkan library not found")
    endif()

    target_link_libraries(vulkan_renderer PUBLIC android log)
    message(STATUS "  [OK] vulkan_renderer library")
endif()

# =============================================================================
# SDL3 Integration
# =============================================================================
if(DOWNLOADER_USE_SDL3)
    message(STATUS "")
    message(STATUS "=== SDL3 Integration ===")

    # SDL3 source directory (to be populated)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sdl3/CMakeLists.txt")
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_VULKAN ON CACHE BOOL "" FORCE)
        add_subdirectory(sdl3)
        set(HAVE_SDL3 TRUE)
        message(STATUS "  [OK] SDL3 configured")
    else()
        message(STATUS "  [SKIP] SDL3 - source not found, using stub")
        set(HAVE_SDL3 FALSE)
    endif()
endif()

# =============================================================================
# Main Application Library
# =============================================================================
message(STATUS "")
message(STATUS "=== Building Main Application ===")

set(APP_SOURCES
    native-lib.cpp
    jni_bridge.cpp
    app/downloader_app.cpp
    app/video_manager.cpp
)

add_library(${CMAKE_PROJECT_NAME} SHARED ${APP_SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app
)

# Link all components
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        ytdlp::ytdlp
        android
        log
)

if(DOWNLOADER_USE_VULKAN AND TARGET vulkan_renderer)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE vulkan_renderer)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE DOWNLOADER_USE_VULKAN=1)
endif()

if(HAVE_SDL3 AND TARGET SDL3::SDL3-static)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SDL3::SDL3-static)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE DOWNLOADER_USE_SDL3=1)
endif()

message(STATUS "  [OK] ${CMAKE_PROJECT_NAME} shared library")

# =============================================================================
# Summary
# =============================================================================
message(STATUS "")
message(STATUS "=== Build Summary ===")
message(STATUS "  Project:     ${CMAKE_PROJECT_NAME}")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  Vulkan:      ${DOWNLOADER_USE_VULKAN}")
message(STATUS "  SDL3:        ${DOWNLOADER_USE_SDL3}")
message(STATUS "  curl:        ${HAVE_CURL}")
message(STATUS "  OpenSSL:     ${HAVE_OPENSSL}")
message(STATUS "")
