/**
 * Encoding Utilities Header
 *
 * Provides encoding/decoding utilities including base64, base-N, JWT, HMAC,
 * multipart encoding, Caesar cipher, and BOM handling.
 */

#ifndef YTDLP_UTILS_ENCODE_UTILS_HPP
#define YTDLP_UTILS_ENCODE_UTILS_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>
#include <utility>

namespace ytdlp::utils {

// ============================================================================
// Base-N Encoding/Decoding
// ============================================================================

/**
 * Get base-N character table
 * @param n Base (e.g., 62, 64)
 * @param custom_table Optional custom character table
 * @return Character table string
 */
std::string base_n_table(int n, std::optional<std::string> custom_table = std::nullopt);

/**
 * Encode number to base-N
 * @param num Number to encode
 * @param n Base
 * @param table Optional custom character table
 * @return Encoded string
 */
std::string encode_base_n(int64_t num, int n, std::optional<std::string> table = std::nullopt);

/**
 * Decode base-N string to number
 * @param str Encoded string
 * @param n Base
 * @param table Optional custom character table
 * @return Decoded number or nullopt if invalid
 */
std::optional<int64_t> decode_base_n(std::string_view str, int n,
                                      std::optional<std::string> table = std::nullopt);

// ============================================================================
// Base64 Encoding/Decoding
// ============================================================================

/**
 * Base64 encode binary data
 * @param data Binary data to encode
 * @return Base64-encoded string
 */
std::string base64_encode(const std::vector<uint8_t>& data);

/**
 * Base64 encode string
 * @param data String to encode
 * @return Base64-encoded string
 */
std::string base64_encode(std::string_view data);

/**
 * Base64 decode string
 * @param encoded Base64-encoded string
 * @return Decoded binary data or nullopt if invalid
 */
std::optional<std::vector<uint8_t>> base64_decode(std::string_view encoded);

/**
 * Encode data as data URI
 * @param data Binary data
 * @param mime_type MIME type (e.g., "image/png")
 * @return Data URI string
 */
std::string encode_data_uri(const std::vector<uint8_t>& data, std::string_view mime_type);

// ============================================================================
// Cryptographic Functions
// ============================================================================

/**
 * Compute HMAC-SHA256
 * @param key HMAC key (binary)
 * @param message Message to sign (binary)
 * @return HMAC digest (32 bytes)
 */
std::vector<uint8_t> hmac_sha256(const std::vector<uint8_t>& key,
                                  const std::vector<uint8_t>& message);

/**
 * Compute HMAC-SHA256
 * @param key HMAC key (string)
 * @param message Message to sign (string)
 * @return HMAC digest (32 bytes)
 */
std::vector<uint8_t> hmac_sha256(std::string_view key, std::string_view message);

// ============================================================================
// JWT (JSON Web Tokens)
// ============================================================================

/**
 * Encode JWT with HS256 signature
 * @param payload_data Payload JSON object
 * @param key Signing key
 * @param headers Optional custom headers
 * @return JWT string
 */
std::string jwt_encode(const nlohmann::json& payload_data,
                       std::string_view key,
                       std::optional<nlohmann::json> headers = std::nullopt);

/**
 * Decode JWT payload (without verification)
 * @param jwt JWT string
 * @return Decoded payload or nullopt if invalid
 */
std::optional<nlohmann::json> jwt_decode_hs256(std::string_view jwt);

// ============================================================================
// Multipart Encoding
// ============================================================================

/**
 * Encode form data as multipart/form-data
 * @param data Key-value pairs to encode
 * @param boundary Optional custom boundary
 * @return Pair of (body, content_type)
 */
std::pair<std::string, std::string> multipart_encode(
    const std::map<std::string, std::string>& data,
    std::optional<std::string> boundary = std::nullopt);

// ============================================================================
// Caesar Cipher and ROT47
// ============================================================================

/**
 * Caesar cipher shift
 * @param s String to shift
 * @param alphabet Alphabet to use
 * @param shift Shift amount
 * @return Shifted string
 */
std::string caesar(std::string_view s, std::string_view alphabet, int shift);

/**
 * ROT47 encoding/decoding
 * @param s String to encode/decode
 * @return ROT47 encoded/decoded string
 */
std::string rot47(std::string_view s);

// ============================================================================
// Packed JavaScript Code
// ============================================================================

/**
 * Decode packed JavaScript code
 * @param code Packed JavaScript code
 * @return Decoded code or nullopt if not packed
 */
std::optional<std::string> decode_packed_codes(std::string_view code);

// ============================================================================
// Bitwise Operations
// ============================================================================

/**
 * Unsigned right shift (JavaScript >>> operator)
 * @param val Value to shift
 * @param n Shift amount
 * @return Shifted value (zero-fill)
 */
uint32_t urshift(int32_t val, int n);

// ============================================================================
// BOM (Byte Order Mark) Detection
// ============================================================================

/**
 * Detect BOM in binary data
 * @param data Binary data to check
 * @return Pair of (BOM size, encoding name) or nullopt if no BOM
 */
std::optional<std::pair<size_t, std::string>> detect_bom(const std::vector<uint8_t>& data);

/**
 * Remove BOM from string
 * @param data String possibly containing BOM
 * @return String with BOM removed
 */
std::string remove_bom(std::string_view data);

} // namespace ytdlp::utils

#endif // YTDLP_UTILS_ENCODE_UTILS_HPP
