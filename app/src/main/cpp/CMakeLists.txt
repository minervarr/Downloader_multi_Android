# =============================================================================
# Downloader Multi - Android NDK Build Configuration
# Pure C++ with SDL3 + Vulkan + ytdlp-cpp (minimal Java)
# =============================================================================

cmake_minimum_required(VERSION 3.22.1)
project("downloader_multi" VERSION 1.0.0 LANGUAGES CXX C)

# =============================================================================
# C++ Standard Configuration
# =============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Build Options
# =============================================================================
option(DOWNLOADER_USE_VULKAN "Enable Vulkan renderer" ON)
option(DOWNLOADER_USE_SDL3 "Enable SDL3 integration" ON)
option(DOWNLOADER_BUILD_TESTS "Build tests" OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
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
# SDL3 (from submodule - must be first for Android)
# =============================================================================
message(STATUS "=== SDL3 Configuration ===")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/SDL3/CMakeLists.txt")
    # Configure SDL3 for Android
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_VULKAN ON CACHE BOOL "" FORCE)
    set(SDL_RENDER_VULKAN ON CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

    add_subdirectory(extern/SDL3)
    set(HAVE_SDL3 TRUE)
    message(STATUS "  [OK] SDL3 configured from submodule")
else()
    message(FATAL_ERROR "SDL3 submodule not found! Run: git submodule update --init --recursive")
endif()

# =============================================================================
# External Dependencies (Header-only from submodules)
# =============================================================================
message(STATUS "")
message(STATUS "=== External Dependencies (Submodules) ===")

# fmt - Header only mode
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/fmt/include")
    add_library(fmt INTERFACE)
    target_include_directories(fmt INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/fmt/include
    )
    target_compile_definitions(fmt INTERFACE FMT_HEADER_ONLY=1)
    add_library(fmt::fmt ALIAS fmt)
    message(STATUS "  [OK] fmt (submodule, header-only)")
else()
    message(FATAL_ERROR "fmt submodule not found! Run: git submodule update --init --recursive")
endif()

# spdlog - Header only mode
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog/include")
    add_library(spdlog INTERFACE)
    target_include_directories(spdlog INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog/include
    )
    target_compile_definitions(spdlog INTERFACE
        SPDLOG_HEADER_ONLY=1
        SPDLOG_FMT_EXTERNAL=1
    )
    target_link_libraries(spdlog INTERFACE fmt::fmt)
    add_library(spdlog::spdlog ALIAS spdlog)
    message(STATUS "  [OK] spdlog (submodule, header-only)")
else()
    message(FATAL_ERROR "spdlog submodule not found! Run: git submodule update --init --recursive")
endif()

# nlohmann/json - Header only
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/json/include")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/json/include
    )
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    message(STATUS "  [OK] nlohmann_json (submodule, header-only)")
else()
    message(FATAL_ERROR "nlohmann_json submodule not found! Run: git submodule update --init --recursive")
endif()

# =============================================================================
# Prebuilt Libraries (curl, openssl) - Optional
# =============================================================================
message(STATUS "")
message(STATUS "=== Optional Prebuilt Libraries ===")

# Check for prebuilt curl
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/curl/${ANDROID_ABI}/libcurl.so")
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
    message(STATUS "  [STUB] curl - networking disabled")
    set(HAVE_CURL FALSE)
endif()

# Check for prebuilt openssl
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/openssl/${ANDROID_ABI}/libssl.so")
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
    message(STATUS "  [STUB] openssl - crypto disabled")
    set(HAVE_OPENSSL FALSE)
endif()

# =============================================================================
# ytdlp-cpp Library (Video Extraction)
# =============================================================================
message(STATUS "")
message(STATUS "=== ytdlp-cpp Library ===")

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

target_compile_definitions(ytdlp_static PRIVATE YTDLP_ANDROID=1)

# Link header-only dependencies
target_link_libraries(ytdlp_static PUBLIC fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json)

# Link curl/openssl if available
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
    message(STATUS "=== Vulkan Renderer ===")

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
        message(STATUS "  [OK] Vulkan: ${VULKAN_LIB}")
    else()
        message(WARNING "  [WARN] Vulkan library not found")
    endif()

    target_link_libraries(vulkan_renderer PUBLIC android log)
    message(STATUS "  [OK] vulkan_renderer library")
endif()

# =============================================================================
# Main Application (Pure C++ with SDL3)
# =============================================================================
message(STATUS "")
message(STATUS "=== Main Application ===")

set(APP_SOURCES
    main.cpp
    app/downloader_app.cpp
    app/video_manager.cpp
)

# Create shared library (SDL3 handles the entry point)
add_library(${CMAKE_PROJECT_NAME} SHARED ${APP_SOURCES})

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app
)

# Compile definitions
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    DOWNLOADER_USE_SDL3=1
)

if(DOWNLOADER_USE_VULKAN)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE DOWNLOADER_USE_VULKAN=1)
endif()

# Link all components
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        SDL3::SDL3-static
        ytdlp::ytdlp
        android
        log
)

if(DOWNLOADER_USE_VULKAN AND TARGET vulkan_renderer)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE vulkan_renderer)
endif()

message(STATUS "  [OK] ${CMAKE_PROJECT_NAME} (SDL3-based)")

# =============================================================================
# Summary
# =============================================================================
message(STATUS "")
message(STATUS "=== Build Summary ===")
message(STATUS "  Project:     ${CMAKE_PROJECT_NAME}")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  SDL3:        ON (submodule)")
message(STATUS "  Vulkan:      ${DOWNLOADER_USE_VULKAN}")
message(STATUS "  curl:        ${HAVE_CURL}")
message(STATUS "  OpenSSL:     ${HAVE_OPENSSL}")
message(STATUS "  C++ Std:     C++17")
message(STATUS "")
