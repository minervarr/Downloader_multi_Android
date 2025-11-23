#ifndef YTDLP_CORE_YOUTUBE_DL_HPP
#define YTDLP_CORE_YOUTUBE_DL_HPP

#include <string>
#include <map>
#include <optional>

namespace ytdlp {
    namespace networking {
        class CurlHttpClient;
    }
}

namespace ytdlp::core {

struct YoutubeDLParams {
    bool quiet = false;
    std::optional<std::string> videopassword;
    std::string cookies_file;
    // Add more parameters as needed
};

class YoutubeDL {
public:
    explicit YoutubeDL(const YoutubeDLParams& params = YoutubeDLParams{});
    ~YoutubeDL();

    networking::CurlHttpClient& http_client();

    std::optional<std::string> get_param_videopassword() const;

    void report_warning(const std::string& msg) const;
    void to_stdout(const std::string& msg) const;

    void load_cookies(const std::string& cookie_file);

private:
    YoutubeDLParams params_;
    networking::CurlHttpClient* http_client_ = nullptr;
};

} // namespace ytdlp::core

#endif // YTDLP_CORE_YOUTUBE_DL_HPP
