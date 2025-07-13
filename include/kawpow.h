#pragma once

#include "config.h"
#include <string>
#include <cstdint>

class KawPow {
public:
    KawPow(const Config& config);
    void set_job(const std::string& job_id, const std::string& header_hash, uint64_t start_nonce);
    void search();

private:
    const Config& config;
    std::string current_job_id;
    std::string current_header_hash;
    uint64_t current_start_nonce;
};

// Forward declaration for the CUDA search function
void kawpow_cuda_search(const char* header_hash, uint64_t start_nonce, int device_id, int intensity);

