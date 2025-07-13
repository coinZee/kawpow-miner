#ifndef KAWPOW_H
#define KAWPOW_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include "config.h"

class Stratum; // Forward declaration

class KawPow {
public:
    KawPow(const Config& config);
    ~KawPow();

    void set_stratum(Stratum* s);
    void set_job(const std::string& job_id, const std::string& header_hash, const std::string& seed_hash, uint64_t block_number, const std::string& target);
    void stop_mining();
    bool should_continue() const;

    // This is called from the CUDA code when a share is found
    // In kawpow.h, inside the KawPow class
    void submit_share(const std::string& nonce_hex, const std::string& mix_hash_hex);
private:
    void start_mining_threads();
    void mining_thread_main(int device_id);

    const Config& config;
    Stratum* stratum_client = nullptr;
    
    std::atomic<bool> continue_mining;
    std::vector<std::thread> mining_threads;

    // Current job parameters
    std::string current_job_id;
    std::string current_header_hash;
    std::string current_seed_hash;
    uint64_t current_block_number = 0;
    std::string current_target;
};

// This C-style function is what you will call from your C++ code to launch the CUDA part.
// It needs to be declared `extern "C"` to avoid C++ name mangling.
extern "C" void kawpow_cuda_search(
    const char* header_hash, 
    const char* seed_hash, 
    uint64_t block_number, 
    uint64_t start_nonce, 
    int device_id, 
    int intensity, 
    KawPow* kawpow_instance, // Pass a pointer to the class instance
    const char* target
);

#endif // KAWPOW_H
