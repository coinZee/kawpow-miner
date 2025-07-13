#include "kawpow.h"
#include <thread>
#include <vector>

KawPow::KawPow(const Config& config) : config(config) {}

void KawPow::set_job(const std::string& job_id, const std::string& header_hash, uint64_t start_nonce) {
    current_job_id = job_id;
    current_header_hash = header_hash;
    current_start_nonce = start_nonce;
    search();
}

void KawPow::search() {
    std::vector<std::thread> threads;
    for (const auto& device : config.getCudaDevices()) {
        threads.emplace_back(kawpow_cuda_search, current_header_hash.c_str(), current_start_nonce, device.device_id, device.intensity);
    }

    for (auto& t : threads) {
        t.detach();
    }
}

