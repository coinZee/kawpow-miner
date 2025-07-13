#pragma once

#include <cstdint>
#include <vector>

void keccak_f1600(uint64_t* state);
std::vector<uint8_t> keccak_256(const std::vector<uint8_t>& input);

