#ifndef YTDLP_CORE_INFO_DICT_HPP
#define YTDLP_CORE_INFO_DICT_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <map>

namespace ytdlp::core {

// InfoDict is essentially a JSON object representing video/media metadata
using InfoDict = nlohmann::json;

} // namespace ytdlp::core

#endif // YTDLP_CORE_INFO_DICT_HPP
