#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

// ========== Keccak-f1600 (minimal) ==========
extern "C" {
#include "keccakf1600.h"
}

// ========== Byte Swap (Endian fix) ==========
uint32_t byteswap32(uint32_t x) {
    return (x << 24) | ((x << 8) & 0x00ff0000) |
           ((x >> 8) & 0x0000ff00) | (x >> 24);
}

// ========== Hex to Binary ==========
std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.length() / 2);
    for (std::size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// ========== Target Comparison ==========
bool compare_hash_to_target(const std::vector<uint8_t>& hash, const std::vector<uint8_t>& target) {
    for (int i = 0; i < 32; ++i) {
        if (hash[i] < target[i]) return true;
        if (hash[i] > target[i]) return false;
    }
    return true; // Equal
}

// ========== KawPoW Final Digest ==========
std::vector<uint8_t> compute_final_hash(
    const std::vector<uint8_t>& header_hash,
    const std::vector<uint8_t>& mix_hash,
    uint64_t nonce)
{
    uint8_t input[200] = {0};
    std::memcpy(input, header_hash.data(), 32);
    std::memcpy(input + 32, mix_hash.data(), 32);

    input[64] = 0x01;
    input[136] = 0x80;

    // Padding and length field
    std::vector<uint8_t> state(200, 0);
    std::memcpy(state.data(), input, 200);

    // Call Keccak-f1600 (you need to use your keccak lib here)
    keccakf((uint64_t*)state.data());

    return std::vector<uint8_t>(state.begin(), state.begin() + 32);
}

// ========== Main ==========
int main() {
    // std::string header_hex = "5c0aa6d68aa7eb3c6bb5d0b48be88f233e61f70cedbd7f3b06ac8a303a4127be";
    std::string header_hex = "4b5a80a1b6f15d8a6a993abc5c8273e1761a03da80a7e841bce0a53f2a6f8ebf";
    std::string mix_hash_hex = "5c0aa6d68aa7eb3c6bb5d0b48be88f233e61f70cedbd7f3b06ac8a303a4127be";
    std::string target_hex    = "1ffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    std::string nonce_hex     = "caaf9def";

    auto header_bytes = hex_to_bytes(header_hex);
    auto mix_bytes    = hex_to_bytes(mix_hash_hex);
    auto target_bytes = hex_to_bytes(target_hex);
    auto nonce        = std::stoull(nonce_hex, nullptr, 16);

    // Compute final hash
    auto final_hash = compute_final_hash(header_bytes, mix_bytes, nonce);

    std::cout << "Final Hash: ";
    for (auto b : final_hash) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    std::cout << "\n";

    // Validate
    if (compare_hash_to_target(final_hash, target_bytes)) {
        std::cout << "✅ Share is VALID!\n";
    } else {
        std::cout << "❌ Share is INVALID!\n";
    }

    return 0;
}

