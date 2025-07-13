// include/cuda_miner.h
#pragma once

#include "config.h"
#include "stratum.h"
#include <atomic>
#include <thread>
#include <vector>
#include <cstdint>

class CudaMiner {
public:
    CudaMiner(const Config& config);
    ~CudaMiner();

    bool initialize();
    void start(StratumClient& stratum_client);
    void stop();
    void set_job(const StratumJob& job);

private:
    void work_thread(StratumClient& stratum_client);

    const Config& config_;
    std::atomic<bool> running_{false};
    std::thread worker_;

    // Job data
    std::atomic<uint64_t> current_height_{0};
    std::string current_header_hash_;
    std::string current_job_id_;
    uint64_t current_target_ = 0;
    std::mutex job_mutex_;

    // CUDA resources
    uint32_t* d_dag = nullptr;
    uint64_t dag_size_bytes_ = 0;
    uint32_t* d_light_cache = nullptr;
    uint64_t light_cache_size_bytes_ = 0;
    uint32_t* d_result_buffer = nullptr; // [found, nonce_lo, nonce_hi, mix_hash[7]]
};

