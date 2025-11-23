#include "ytdlp/utils/encode_utils.hpp"
#include "ytdlp/utils/string_utils.hpp"
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <regex>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <ctime>

namespace ytdlp::utils {

namespace {

// Base64 encoding/decoding characters
const char BASE64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

const char BASE64_URL_SAFE_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

// Standard base62 table (matches Python yt-dlp)
const char BASE62_CHARS[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

inline bool is_base64_char(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/') || (c == '-') || (c == '_'));
}

} // anonymous namespace

std::string base_n_table(int n, std::optional<std::string> custom_table) {
    if (custom_table.has_value()) {
        return custom_table.value();
    }

    if (n == 62) {
        return BASE62_CHARS;
    } else if (n == 64) {
        return BASE64_CHARS;
    } else if (n <= 36) {
        // For bases 2-36, use 0-9 and A-Z
        std::string table;
        for (int i = 0; i < std::min(n, 10); ++i) {
            table += char('0' + i);
        }
        for (int i = 10; i < n; ++i) {
            table += char('A' + i - 10);
        }
        return table;
    } else {
        // Default to base62 for other bases
        return std::string(BASE62_CHARS, n);
    }
}

std::string encode_base_n(int64_t num, int n, std::optional<std::string> table) {
    std::string char_table = base_n_table(n, table);
    int64_t base = char_table.length();

    if (num == 0) {
        return std::string(1, char_table[0]);
    }

    std::string result;
    while (num > 0) {
        result = char_table[num % base] + result;
        num /= base;
    }
    return result;
}

std::optional<int64_t> decode_base_n(std::string_view str, int n,
                                      std::optional<std::string> table) {
    if (str.empty()) {
        return std::nullopt;
    }

    std::string char_table = base_n_table(n, table);
    int64_t base = char_table.length();

    // Create lookup map
    std::map<char, int64_t> lookup;
    for (size_t i = 0; i < char_table.length(); ++i) {
        lookup[char_table[i]] = i;
    }

    int64_t result = 0;
    for (char c : str) {
        auto it = lookup.find(c);
        if (it == lookup.end()) {
            return std::nullopt;  // Invalid character
        }
        result = result * base + it->second;
    }

    return result;
}

std::string base64_encode(const std::vector<uint8_t>& data) {
    std::string result;
    int val = 0;
    int valb = -6;

    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        result.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    // Add padding
    while (result.size() % 4) {
        result.push_back('=');
    }

    return result;
}

std::string base64_encode(std::string_view data) {
    std::vector<uint8_t> bytes(data.begin(), data.end());
    return base64_encode(bytes);
}

std::optional<std::vector<uint8_t>> base64_decode(std::string_view encoded) {
    std::vector<uint8_t> result;
    int val = 0;
    int valb = -8;

    for (unsigned char c : encoded) {
        if (c == '=') {
            break;  // Padding
        }

        if (!is_base64_char(c)) {
            return std::nullopt;
        }

        // Find position in BASE64_CHARS or BASE64_URL_SAFE_CHARS
        const char* pos = strchr(BASE64_CHARS, c);
        if (!pos) {
            pos = strchr(BASE64_URL_SAFE_CHARS, c);
            if (!pos) {
                return std::nullopt;
            }
        }
        int char_val = pos - BASE64_CHARS;
        if (char_val < 0 || char_val >= 64) {
            // Try URL-safe variant
            pos = strchr(BASE64_URL_SAFE_CHARS, c);
            if (!pos) {
                return std::nullopt;
            }
            char_val = pos - BASE64_URL_SAFE_CHARS;
        }

        val = (val << 6) + char_val;
        valb += 6;

        if (valb >= 0) {
            result.push_back(uint8_t((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    return result;
}

std::string encode_data_uri(const std::vector<uint8_t>& data, std::string_view mime_type) {
    return std::string("data:") + std::string(mime_type) + ";base64," + base64_encode(data);
}

std::vector<uint8_t> hmac_sha256(const std::vector<uint8_t>& key,
                                  const std::vector<uint8_t>& message) {
    std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);

    HMAC(EVP_sha256(),
         key.data(), key.size(),
         message.data(), message.size(),
         digest.data(), nullptr);

    return digest;
}

std::vector<uint8_t> hmac_sha256(std::string_view key, std::string_view message) {
    std::vector<uint8_t> key_bytes(key.begin(), key.end());
    std::vector<uint8_t> msg_bytes(message.begin(), message.end());
    return hmac_sha256(key_bytes, msg_bytes);
}

std::string jwt_encode(const nlohmann::json& payload_data,
                       std::string_view key,
                       std::optional<nlohmann::json> headers) {
    // Create header
    nlohmann::json header_data = {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };

    if (headers.has_value()) {
        // Merge headers, allowing re-ordering if both alg and typ are present
        if (headers->contains("alg") && headers->contains("typ")) {
            header_data = headers.value();
        } else {
            header_data.update(headers.value());
        }
    }

    // Serialize to JSON with compact formatting
    std::string header_json = header_data.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    std::string payload_json = payload_data.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);

    // Base64url encode (base64 without padding and with URL-safe chars)
    auto base64url_encode = [](std::string_view data) {
        std::vector<uint8_t> bytes(data.begin(), data.end());
        std::string encoded = base64_encode(bytes);
        // Convert to URL-safe and remove padding
        std::replace(encoded.begin(), encoded.end(), '+', '-');
        std::replace(encoded.begin(), encoded.end(), '/', '_');
        // Remove padding
        encoded.erase(std::find(encoded.begin(), encoded.end(), '='), encoded.end());
        return encoded;
    };

    std::string header_b64 = base64url_encode(header_json);
    std::string payload_b64 = base64url_encode(payload_json);

    // Create signature
    std::string to_sign = header_b64 + "." + payload_b64;
    auto signature = hmac_sha256(key, to_sign);
    std::string signature_b64 = base64url_encode(
        std::string(reinterpret_cast<const char*>(signature.data()), signature.size())
    );

    return header_b64 + "." + payload_b64 + "." + signature_b64;
}

std::optional<nlohmann::json> jwt_decode_hs256(std::string_view jwt) {
    // Split JWT into parts
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = jwt.find('.');

    while (end != std::string::npos) {
        parts.push_back(std::string(jwt.substr(start, end - start)));
        start = end + 1;
        end = jwt.find('.', start);
    }
    parts.push_back(std::string(jwt.substr(start)));

    if (parts.size() != 3) {
        return std::nullopt;
    }

    // Decode payload (second part)
    std::string payload_b64 = parts[1];

    // Add padding if needed
    while (payload_b64.length() % 4 != 0) {
        payload_b64 += '=';
    }

    // Convert URL-safe characters back to standard base64
    std::replace(payload_b64.begin(), payload_b64.end(), '-', '+');
    std::replace(payload_b64.begin(), payload_b64.end(), '_', '/');

    auto payload_bytes = base64_decode(payload_b64);
    if (!payload_bytes.has_value()) {
        return std::nullopt;
    }

    // Parse JSON
    try {
        std::string payload_str(payload_bytes->begin(), payload_bytes->end());
        return nlohmann::json::parse(payload_str);
    } catch (...) {
        return std::nullopt;
    }
}

std::pair<std::string, std::string> multipart_encode(
    const std::map<std::string, std::string>& data,
    std::optional<std::string> boundary) {

    // Generate boundary if not provided
    std::string bound;
    if (boundary.has_value()) {
        bound = boundary.value();
    } else {
        // Generate random boundary using time-based seed
        static std::mt19937 gen(static_cast<unsigned>(std::time(nullptr)));
        std::uniform_int_distribution<unsigned> dis(0x10000000, 0xffffffff);
        std::ostringstream oss;
        oss << "---------------" << std::hex << dis(gen);
        bound = oss.str();
    }

    std::string content_type = "multipart/form-data; boundary=" + bound;

    // Build multipart body
    std::ostringstream body;
    for (const auto& [key, value] : data) {
        body << "--" << bound << "\r\n";
        body << "Content-Disposition: form-data; name=\"" << key << "\"\r\n\r\n";
        body << value << "\r\n";
    }
    body << "--" << bound << "--\r\n";

    return std::make_pair(body.str(), content_type);
}

std::string caesar(std::string_view s, std::string_view alphabet, int shift) {
    if (shift == 0) {
        return std::string(s);
    }

    int len = alphabet.length();
    std::string result;
    result.reserve(s.length());

    for (char c : s) {
        size_t pos = alphabet.find(c);
        if (pos != std::string::npos) {
            // Character found in alphabet, shift it
            int new_pos = (static_cast<int>(pos) + shift) % len;
            if (new_pos < 0) {
                new_pos += len;
            }
            result += alphabet[new_pos];
        } else {
            // Character not in alphabet, keep as-is
            result += c;
        }
    }

    return result;
}

std::string rot47(std::string_view s) {
    const std::string alphabet = R"(!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)";
    return caesar(s, alphabet, 47);
}

std::optional<std::string> decode_packed_codes(std::string_view code) {
    // Match packed JavaScript pattern: }('...', base, count, 'symbols'.split('|'))
    std::regex packed_regex(R"(\}\('(.+)',(\d+),(\d+),'([^']+)'\.split\('\|'\))");
    std::smatch match;
    std::string code_str(code);

    if (!std::regex_search(code_str, match, packed_regex)) {
        return std::nullopt;
    }

    std::string obfuscated_code = match[1].str();
    int base = std::stoi(match[2].str());
    int count = std::stoi(match[3].str());
    std::string symbols_str = match[4].str();

    // Split symbols by '|'
    std::vector<std::string> symbols;
    size_t start = 0;
    size_t end = symbols_str.find('|');
    while (end != std::string::npos) {
        symbols.push_back(symbols_str.substr(start, end - start));
        start = end + 1;
        end = symbols_str.find('|', start);
    }
    symbols.push_back(symbols_str.substr(start));

    // Build symbol table
    std::map<std::string, std::string> symbol_table;
    for (int i = 0; i < count && i < static_cast<int>(symbols.size()); ++i) {
        std::string base_n_count = encode_base_n(i, base);
        symbol_table[base_n_count] = symbols[i].empty() ? base_n_count : symbols[i];
    }

    // Replace symbols in obfuscated code
    std::string result = obfuscated_code;
    std::regex word_regex(R"(\b(\w+)\b)");
    std::string temp;
    std::sregex_iterator it(result.begin(), result.end(), word_regex);
    std::sregex_iterator end_it;

    // Build replacement result
    size_t last_pos = 0;
    std::ostringstream oss;

    for (; it != end_it; ++it) {
        std::smatch m = *it;
        oss << result.substr(last_pos, m.position() - last_pos);

        std::string word = m[1].str();
        auto sym_it = symbol_table.find(word);
        if (sym_it != symbol_table.end()) {
            oss << sym_it->second;
        } else {
            oss << word;
        }
        last_pos = m.position() + m.length();
    }
    oss << result.substr(last_pos);

    return oss.str();
}

uint32_t urshift(int32_t val, int n) {
    // Unsigned right shift (zero-fill right shift)
    // Shifting by >= 32 bits is undefined, so handle explicitly
    if (n >= 32) {
        return 0;
    }
    uint32_t unsigned_val = static_cast<uint32_t>(val);
    return unsigned_val >> n;
}

std::optional<std::pair<size_t, std::string>> detect_bom(const std::vector<uint8_t>& data) {
    // Check for BOMs in order of longest first
    const std::vector<std::pair<std::vector<uint8_t>, std::string>> BOMS = {
        {{0x00, 0x00, 0xFE, 0xFF}, "utf-32-be"},
        {{0xFF, 0xFE, 0x00, 0x00}, "utf-32-le"},
        {{0xEF, 0xBB, 0xBF}, "utf-8"},
        {{0xFF, 0xFE}, "utf-16-le"},
        {{0xFE, 0xFF}, "utf-16-be"},
    };

    for (const auto& [bom, encoding] : BOMS) {
        if (data.size() >= bom.size()) {
            bool match = true;
            for (size_t i = 0; i < bom.size(); ++i) {
                if (data[i] != bom[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return std::make_pair(bom.size(), encoding);
            }
        }
    }

    return std::nullopt;
}

std::string remove_bom(std::string_view data) {
    if (data.empty()) {
        return std::string(data);
    }

    // Check for UTF-8 BOM
    if (data.size() >= 3 &&
        static_cast<unsigned char>(data[0]) == 0xEF &&
        static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF) {
        return std::string(data.substr(3));
    }

    // Check for UTF-16 BOM
    if (data.size() >= 2) {
        if ((static_cast<unsigned char>(data[0]) == 0xFF &&
             static_cast<unsigned char>(data[1]) == 0xFE) ||
            (static_cast<unsigned char>(data[0]) == 0xFE &&
             static_cast<unsigned char>(data[1]) == 0xFF)) {
            return std::string(data.substr(2));
        }
    }

    return std::string(data);
}

} // namespace ytdlp::utils
