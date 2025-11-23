#include "ytdlp/core/youtube_dl.hpp"
#include "ytdlp/networking/curl_http_client.hpp"
#include <iostream>

namespace ytdlp::core {

YoutubeDL::YoutubeDL(const YoutubeDLParams& params)
    : params_(params)
    , http_client_(new networking::CurlHttpClient()) {
}

YoutubeDL::~YoutubeDL() {
    delete http_client_;
}

networking::CurlHttpClient& YoutubeDL::http_client() {
    return *http_client_;
}

std::optional<std::string> YoutubeDL::get_param_videopassword() const {
    return params_.videopassword;
}

void YoutubeDL::report_warning(const std::string& msg) const {
    if (!params_.quiet) {
        std::cerr << "WARNING: " << msg << std::endl;
    }
}

void YoutubeDL::to_stdout(const std::string& msg) const {
    if (!params_.quiet) {
        std::cout << msg << std::endl;
    }
}

void YoutubeDL::load_cookies(const std::string& cookie_file) {
    // TODO: Implement cookie loading from file
    (void)cookie_file;
}

} // namespace ytdlp::core
