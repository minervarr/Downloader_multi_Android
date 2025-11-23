/**
 * Library Verification Test
 *
 * This file tests that all third-party libraries are properly linked
 * and can be used together. It's a simple smoke test to verify the
 * build system configuration.
 */

#include <iostream>
#include <string>

// CURL - HTTP networking
#include <curl/curl.h>

// OpenSSL - Cryptography
#include <openssl/ssl.h>
#include <openssl/evp.h>

// nlohmann/json - JSON parsing
#include <nlohmann/json.hpp>

// fmt - String formatting
#include <fmt/core.h>
#include <fmt/color.h>

// spdlog - Logging
#include <spdlog/spdlog.h>

// cxxopts - CLI parsing
#include <cxxopts.hpp>

// pugixml - XML/HTML parsing
#include <pugixml.hpp>

// Boost - Regex, filesystem
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

// PCRE2 - Alternative regex
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

int main(int argc, char* argv[]) {
    fmt::print(fmt::fg(fmt::color::green), "\n");
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "========================================\n");
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "  yt-dlp C++ Library Verification\n");
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "========================================\n\n");

    bool all_ok = true;

    // Test CURL
    fmt::print("Testing CURL... ");
    curl_version_info_data* curl_info = curl_version_info(CURLVERSION_NOW);
    if (curl_info) {
        fmt::print(fmt::fg(fmt::color::green), "✓ {}\n", curl_info->version);
    } else {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Test OpenSSL
    fmt::print("Testing OpenSSL... ");
    const char* openssl_version = OpenSSL_version(OPENSSL_VERSION);
    if (openssl_version) {
        fmt::print(fmt::fg(fmt::color::green), "✓ {}\n", openssl_version);
    } else {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Test nlohmann/json
    fmt::print("Testing nlohmann/json... ");
    try {
        nlohmann::json j = {
            {"name", "yt-dlp-cpp"},
            {"version", "0.1.0"},
            {"languages", {"C++", "C++17"}}
        };
        std::string name = j["name"];
        fmt::print(fmt::fg(fmt::color::green), "✓ v{}.{}.{}\n",
                   NLOHMANN_JSON_VERSION_MAJOR,
                   NLOHMANN_JSON_VERSION_MINOR,
                   NLOHMANN_JSON_VERSION_PATCH);
    } catch (...) {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Test fmt
    fmt::print("Testing fmt... ");
    std::string formatted = fmt::format("Hello from fmt {}.{}.{}",
                                       FMT_VERSION / 10000,
                                       (FMT_VERSION % 10000) / 100,
                                       FMT_VERSION % 100);
    fmt::print(fmt::fg(fmt::color::green), "✓ v{}.{}.{}\n",
               FMT_VERSION / 10000,
               (FMT_VERSION % 10000) / 100,
               FMT_VERSION % 100);

    // Test spdlog
    fmt::print("Testing spdlog... ");
    spdlog::set_level(spdlog::level::off); // Suppress output
    spdlog::info("Test message");
    fmt::print(fmt::fg(fmt::color::green), "✓ v{}.{}.{}\n",
               SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

    // Test cxxopts
    fmt::print("Testing cxxopts... ");
    try {
        cxxopts::Options options("test", "Test program");
        options.add_options()
            ("h,help", "Print help");
        fmt::print(fmt::fg(fmt::color::green), "✓ OK\n");
    } catch (...) {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Test pugixml
    fmt::print("Testing pugixml... ");
    try {
        pugi::xml_document doc;
        doc.load_string("<test>Hello</test>");
        auto node = doc.child("test");
        if (node && std::string(node.child_value()) == "Hello") {
            fmt::print(fmt::fg(fmt::color::green), "✓ v{}.{}\n",
                       PUGIXML_VERSION / 100, PUGIXML_VERSION % 100);
        } else {
            fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
            all_ok = false;
        }
    } catch (...) {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Test Boost.Regex
    fmt::print("Testing Boost.Regex... ");
    try {
        boost::regex pattern(R"(\d+)");
        std::string text = "Version 123";
        boost::smatch match;
        if (boost::regex_search(text, match, pattern)) {
            fmt::print(fmt::fg(fmt::color::green), "✓ v{}.{}.{}\n",
                       BOOST_VERSION / 100000,
                       BOOST_VERSION / 100 % 1000,
                       BOOST_VERSION % 100);
        } else {
            fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
            all_ok = false;
        }
    } catch (...) {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Test Boost.Filesystem
    fmt::print("Testing Boost.Filesystem... ");
    try {
        boost::filesystem::path p = "/tmp/test";
        if (p.string() == "/tmp/test") {
            fmt::print(fmt::fg(fmt::color::green), "✓\n");
        } else {
            fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
            all_ok = false;
        }
    } catch (...) {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Test PCRE2
    fmt::print("Testing PCRE2... ");
    const char* pattern_str = "\\d+";
    int errornumber;
    PCRE2_SIZE erroroffset;
    pcre2_code* re = pcre2_compile(
        (PCRE2_SPTR)pattern_str,
        PCRE2_ZERO_TERMINATED,
        0,
        &errornumber,
        &erroroffset,
        NULL
    );
    if (re) {
        fmt::print(fmt::fg(fmt::color::green), "✓ v{}.{}\n",
                   PCRE2_MAJOR, PCRE2_MINOR);
        pcre2_code_free(re);
    } else {
        fmt::print(fmt::fg(fmt::color::red), "✗ Failed\n");
        all_ok = false;
    }

    // Summary
    fmt::print("\n");
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "========================================\n");
    if (all_ok) {
        fmt::print(fmt::fg(fmt::color::green) | fmt::emphasis::bold,
                   "  ✓ All libraries verified successfully!\n");
    } else {
        fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold,
                   "  ✗ Some libraries failed verification\n");
    }
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold,
               "========================================\n\n");

    return all_ok ? 0 : 1;
}
