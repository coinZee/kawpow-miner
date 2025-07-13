// src/kawpow_host.cpp

#include "kawpow.h"
#include "stratum.h"
#include "logging.h"
// #include <iostream>
#include <cuda_runtime.h> // Make sure you have this include


// In your C++ host file (e.g., kawpow_host.cpp or main.cpp)

// In your C++ host file (e.g., kawpow_host.cpp or main.cpp)
// Make sure you have this include at the top of the file:
void initialize_kawpow_constants() {
    const uint32_t h_ravencoin_kawpow[15] = {
        0x00000072, 0x00000041, 0x00000056, 0x00000045, 0x0000004E,
        0x00000043, 0x0000004F, 0x00000049, 0x0000004E, 0x0000004B,
        0x00000041, 0x00000057, 0x00000050, 0x0000004F, 0x00000057
    };

    // THE FIX:
    // 1. We completely REMOVE the problematic 'extern "C" __constant__' line.
    // 2. We pass the name of the GPU variable to the function AS A STRING.

    cudaError_t err = cudaMemcpyToSymbol("d_ravencoin_kawpow", h_ravencoin_kawpow, sizeof(h_ravencoin_kawpow));

    if (err != cudaSuccess) {
        // Using printf because this might be a C++ or C file
        printf("FATAL: Failed to copy KawPoW constants to GPU: %s\n", cudaGetErrorString(err));
        exit(1);
    }
    printf("Successfully initialized KawPoW constants on GPU.\n");
}

// Constructor
KawPow::KawPow(const Config& config) : config(config), continue_mining(false) {}

// Destructor
KawPow::~KawPow() {
    stop_mining();
}

void KawPow::set_stratum(Stratum* s) {
    stratum_client = s;
}

void KawPow::set_job(const std::string& job_id, const std::string& header_hash, const std::string& seed_hash, uint64_t block_number, const std::string& target) {
    LOG_INFO << "Setting new job for KawPow host.";
    
    stop_mining();

    current_job_id = job_id;
    current_header_hash = header_hash;
    current_seed_hash = seed_hash;
    current_block_number = block_number;
    current_target = target;
    
    continue_mining.store(true);
    start_mining_threads();
}

void KawPow::stop_mining() {
    if (continue_mining.load()) {
        LOG_INFO << "Stopping mining threads...";
        continue_mining.store(false);
        for (auto& t : mining_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        mining_threads.clear();
        LOG_INFO << "All mining threads stopped.";
    }
}

bool KawPow::should_continue() const {
    return continue_mining.load();
}

void KawPow::start_mining_threads() {
    LOG_INFO << "Starting mining threads for configured devices.";
    const auto& devices = config.getCudaDevices(); 
    for (const auto& device : devices) {
        // THE ONLY CHANGE IS HERE: device.id -> device.deviceId
        mining_threads.emplace_back(&KawPow::mining_thread_main, this, device.device_id);
    }
}

void KawPow::mining_thread_main(int device_id) {
    LOG_INFO << "Mining thread started for device " << device_id;
    uint64_t start_nonce = 0; 
    
    kawpow_cuda_search(
        current_header_hash.c_str(),
        current_seed_hash.c_str(),
        current_block_number,
        start_nonce,
        device_id,
        1024,
        this,
        current_target.c_str()
    );
}

void KawPow::submit_share(const std::string& nonce_hex, const std::string& mix_hash_hex) {
    if (stratum_client) {
        stratum_client->submit(current_job_id, nonce_hex, current_header_hash, mix_hash_hex);
    } else {
        LOG_ERROR << "Stratum client not set, cannot submit share.";
    }
}
