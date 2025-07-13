#include "kawpow.h"
#include "logging.h"

#include <cuda_runtime.h>
#include <iostream>
#include <mutex>
#include <cstdio>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <mutex>
extern "C" {
    #include "libethash/ethash.h"
}

// Define constants once at the top
#define PROGPOW_LANES 16
#define PROGPOW_REGS 32
#define PROGPOW_LOOP_COUNT 64
#define PROGPOW_DAG_ITEM_SIZE 64
#define PROGPOW_DAG_PARENTS 256

// --- FNV1a Hashing Helper ---
#define FNV_PRIME 0x01000193


__constant__ uint32_t d_ravencoin_kawpow[15];
// const uint64_t fixed_difficulty = 256; // tweak as needed


__device__ inline uint32_t fnv1a_32(uint32_t a, uint32_t b) { return (a ^ b) * FNV_PRIME; }
uint32_t fnv1a_32_cpu(uint32_t a, uint32_t b) { return (a ^ b) * FNV_PRIME; }

// --- Keccak Hashing & Helper Implementation for GPU ---
__device__ inline uint64_t rotate_left(uint64_t x, uint8_t n) { return (x << n) | (x >> (64 - n)); }
__device__ inline uint32_t rotate_right(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

// --- FIXED: Proper endianness handling ---
__device__ inline uint32_t byteswap_32(uint32_t x) { 
    return ((x << 24) & 0xff000000) | 
           ((x << 8)  & 0x00ff0000) | 
           ((x >> 8)  & 0x0000ff00) | 
           ((x >> 24) & 0x000000ff); 
}

__device__ inline uint64_t byteswap_64(uint64_t x) {
    return ((x << 56) & 0xff00000000000000ULL) |
           ((x << 40) & 0x00ff000000000000ULL) |
           ((x << 24) & 0x0000ff0000000000ULL) |
           ((x << 8)  & 0x000000ff00000000ULL) |
           ((x >> 8)  & 0x00000000ff000000ULL) |
           ((x >> 24) & 0x0000000000ff0000ULL) |
           ((x >> 40) & 0x000000000000ff00ULL) |
           ((x >> 56) & 0x00000000000000ffULL);
}

__device__ static void keccak_f1600(uint64_t* state) {
    const uint64_t keccak_round_constants[24] = {
        0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000,
        0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
        0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
        0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003,
        0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a,
        0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008
    };
    
    for (int i = 0; i < 24; ++i) {
        uint64_t C[5], D;
        for (int x = 0; x < 5; ++x) C[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
        for (int x = 0; x < 5; ++x) {
            D = C[(x + 4) % 5] ^ rotate_left(C[(x + 1) % 5], 1);
            for (int y = 0; y < 25; y += 5) state[y + x] ^= D;
        }
        uint64_t current = state[1];
        for (int x = 0; x < 24; ++x) {
            int r = ((x + 1) * (x + 2) / 2) % 64;
            int lane = (2 * (x % 5) + 3 * ((x / 5) % 5)) % 5;
            uint64_t temp = state[1 + lane];
            state[1 + lane] = rotate_left(current, r);
            current = temp;
        }
        // Chi step
        for (int y = 0; y < 25; y += 5) {
            uint64_t temp[5];
            for (int x = 0; x < 5; ++x) temp[x] = state[y + x];
            for (int x = 0; x < 5; ++x) state[y + x] = temp[x] ^ ((~temp[(x + 1) % 5]) & temp[(x + 2) % 5]);
        }
        state[0] ^= keccak_round_constants[i];
    }
}

// Global DAG cache

struct DagCache {
    void* d_dag = nullptr;
    size_t size = 0;
    uint64_t epoch = UINT64_MAX;
    std::mutex mutex;

    // prevent copying
    DagCache(const DagCache&) = delete;
    DagCache& operator=(const DagCache&) = delete;

    // allow moving
    DagCache(DagCache&&) = default;
    DagCache& operator=(DagCache&&) = default;

    // default constructor
    DagCache() = default;
};


static std::unordered_map<int, DagCache> g_dag_caches;
static std::mutex g_dag_mutex;

// ===================================================================================
// == DAG & Light Cache Generation
// ===================================================================================
uint64_t get_kawpow_dag_size(uint64_t epoch) {
    const uint64_t dag_size_initial = 1ULL * 1024 * 1024 * 1024;
    const uint64_t dag_size_growth = 8ULL * 1024 * 1024;
    return dag_size_initial + (epoch * dag_size_growth);
}

void* generate_kawpow_light_cache(uint64_t epoch, const char* seed_hash_hex, size_t& cache_size) {
    const uint32_t light_cache_initial_size = 16 * 1024 * 1024;
    const uint32_t light_cache_growth = 256 * 1024;
    const uint32_t words_per_item = 16;
    cache_size = light_cache_initial_size + (epoch * light_cache_growth);
    uint32_t num_cache_items = cache_size / sizeof(uint32_t);
    uint32_t* h_cache = (uint32_t*)malloc(cache_size);
    if (!h_cache) { LOG_ERROR << "Failed to allocate memory for light cache on CPU."; return nullptr; }
    
    // Parse seed hash correctly
    uint32_t seed[8];
    for (int i = 0; i < 8; ++i) {
        sscanf(seed_hash_hex + (i * 8), "%8x", &seed[i]);
    }
    
    // Initialize cache with seed
    for (int i = 0; i < 8; ++i) h_cache[i] = seed[i];
    for (uint32_t i = 8; i < words_per_item; ++i) h_cache[i] = fnv1a_32_cpu(h_cache[i - 8], h_cache[i - 7]);
    
    // Generate remaining cache items
    for (uint32_t i = words_per_item; i < num_cache_items; ++i) {
        h_cache[i] = fnv1a_32_cpu(h_cache[i - words_per_item], h_cache[i - (words_per_item - 1)]);
    }
    
    LOG_INFO << "Successfully generated " << cache_size / (1024*1024) << " MB light cache for epoch " << epoch;
    return h_cache;
}

// ===================================================================================
// == CORRECTED DAG Generation Kernel (Ethash/KawPoW Standard)
// ===================================================================================
__global__ void generate_dag_kernel(uint32_t* d_dag, const uint32_t* d_cache, uint32_t num_dag_items, uint32_t num_cache_items)
{
    const uint32_t node_index = blockIdx.x * blockDim.x + threadIdx.x;
    if (node_index >= num_dag_items) {
        return;
    }

    // Constants for standard Ethash DAG generation
    const uint32_t HASH_WORDS = 16; // 512 bits / 32 bits per word
    const uint32_t DAG_PARENTS = 256;

    // The number of 64-byte items in the light cache
    const uint32_t num_cache_words = num_cache_items / HASH_WORDS;

    // Calculate the starting cache index for this DAG node
    const uint32_t start_node_index = node_index % num_cache_words;
    const uint32_t* cache_node_ptr = d_cache + start_node_index * HASH_WORDS;

    // Initialize the mix state from the light cache
    uint32_t mix[HASH_WORDS];
    for (uint32_t i = 0; i < HASH_WORDS; ++i) {
        mix[i] = cache_node_ptr[i];
    }

    // Main DAG generation loop
    for (uint32_t i = 0; i < DAG_PARENTS; ++i) {
        // Calculate the index of the parent node to fetch from the light cache
        // This uses standard modular arithmetic, NOT the complex "laned" version.
        uint32_t parent_index = fnv1a_32(node_index ^ i, mix[i % HASH_WORDS]) % num_cache_words;
        const uint32_t* parent_ptr = d_cache + parent_index * HASH_WORDS;

        // Mix the parent data into the current state
        for (uint32_t j = 0; j < HASH_WORDS; ++j) {
            mix[j] = fnv1a_32(mix[j], parent_ptr[j]);
        }
    }

    // Final step: In ProgPoW/KawPoW, the DAG item is the mix itself.
    // (In standard Ethash, there would be a final hash here, but not for KawPoW's DAG).
    // Copy the final mix to the global DAG memory.
    uint32_t* dag_item_ptr = d_dag + node_index * HASH_WORDS;
    for (uint32_t i = 0; i < HASH_WORDS; ++i) {
        dag_item_ptr[i] = mix[i];
    }
}

// ===================================================================================
// == Final Hashing Kernel and Helpers
// ===================================================================================
struct kiss99_rng {
    uint32_t z, w, jsr, jcong;
    __device__ void seed(uint32_t s) { z = w = jsr = jcong = s; }
    __device__ uint32_t get() {
        z = 36969 * (z & 65535) + (z >> 16); 
        w = 18000 * (w & 65535) + (w >> 16); 
        uint32_t mwc = (z << 16) + w;
        jsr ^= (jsr << 17); jsr ^= (jsr >> 13); jsr ^= (jsr << 5); 
        jcong = 69069 * jcong + 1234567;
        return (mwc ^ jcong) + jsr;
    }
};

struct uint256_t { uint32_t val[8]; };

__device__ inline bool is_less_or_equal(const uint256_t& a, const uint256_t& b) {
    for (int i = 7; i >= 0; --i) { 
        if (a.val[i] < b.val[i]) return true; 
        if (a.val[i] > b.val[i]) return false; 
    } 
    return true;
}

// In src/kawpow.cu, replace your entire old kernel with this one.

__global__ void kawpow_kernel(
    uint64_t* d_result_nonce, char* d_result_mix_hash,
    const char* d_header_hash, uint64_t start_nonce,
    const uint32_t* d_dag, const char* d_target_hex)
{
    uint64_t nonce = start_nonce + blockIdx.x * blockDim.x + threadIdx.x;

    if (*d_result_nonce != 0) {
        return;
    }

    // --- Step 1: Initial Keccak Hash ---
    uint64_t keccak_state[25] = {0};
    uint32_t* keccak_state_32 = (uint32_t*)keccak_state;

    // Load header hash (32 bytes)
    for (int i = 0; i < 8; ++i) {
        keccak_state_32[i] = ((uint32_t*)d_header_hash)[i];
    }
    // Load nonce (8 bytes)
    *(uint64_t*)(&keccak_state_32[8]) = nonce;
    // Load "RAVENCOINKAWPOW" constant (60 bytes)
    for (int i = 0; i < 15; ++i) {
        keccak_state_32[10 + i] = d_ravencoin_kawpow[i];
    }

    // Keccak-f800 is used, which means 100 bytes of input.
    // We use Keccak-f1600, so we must set the correct padding for a 100-byte message.
    // 100 bytes = 12.5 uint64_t words.
    // The padding starts at byte 100, which is keccak_state[12].
    keccak_state[12] |= 0x0000000000000001; // Padding: 1
    keccak_state[16] |= 0x8000000000000000; // Padding: 1...0...1

    keccak_f1600(keccak_state);

    // --- Step 2: Main ProgPoW Loop (Your existing code is likely okay here for now) ---
    uint32_t mix[PROGPOW_REGS];
    for (int i = 0; i < 16; ++i) mix[i] = ((uint32_t*)keccak_state)[i];
    for (int i = 16; i < PROGPOW_REGS; ++i) mix[i] = mix[i - 16] * FNV_PRIME;

    kiss99_rng rng;
    rng.seed(mix[0] ^ mix[1]);
    uint32_t dag_lookup_state[PROGPOW_LANES];
    for (int i = 0; i < PROGPOW_LANES; ++i) dag_lookup_state[i] = mix[i];

    for (uint32_t i = 0; i < PROGPOW_LOOP_COUNT; ++i) {
        uint32_t dag_item_index = rng.get() % (PROGPOW_LANES * PROGPOW_REGS);
        for (uint32_t l = 0; l < PROGPOW_LANES; ++l) {
            uint32_t dag_word_index = dag_item_index * 16 + l;
            dag_lookup_state[l] = fnv1a_32(dag_lookup_state[l], d_dag[dag_word_index]);
        }
        uint32_t src_rand = rng.get(), dst_rand = rng.get(), sel_rand = rng.get();
        uint32_t src1 = src_rand % PROGPOW_REGS, src2 = (src1 + (src_rand >> 16) % (PROGPOW_REGS - 1) + 1) % PROGPOW_REGS;
        uint32_t dst = dst_rand % PROGPOW_REGS, sel = sel_rand % 4;
        switch (sel) {
            case 0: mix[dst] *= mix[src1]; break;
            case 1: mix[dst] += mix[src2]; break;
            case 2: mix[dst] = rotate_right(mix[src1], mix[src2] & 31); break;
            case 3: mix[dst] ^= mix[src2]; break;
        }
    }

    // --- Step 3: Final Mix Hash Aggregation (Corrected based on XMRig) ---
    const uint32_t fnv_offset_basis = 0x811c9dc5;
    uint32_t final_mix_hash[8];
    for (int i = 0; i < 8; ++i) {
        final_mix_hash[i] = fnv_offset_basis;
    }
    // Each thread calculates its lane hash and combines it into the final hash
    // This assumes PROGPOW_LANES is 16
    for (int i = 0; i < PROGPOW_REGS; i += 2) {
        uint32_t h1 = fnv1a_32(fnv_offset_basis, mix[i]);
        uint32_t h2 = fnv1a_32(h1, mix[i+1]);
        final_mix_hash[i % 8] = fnv1a_32(final_mix_hash[i % 8], h2);
    }


    // --- Step 4: Final Keccak Hash ---
    // Reset state and prepare for final hash
    for(int i = 0; i < 25; ++i) keccak_state[i] = 0;

    // Load header hash (32 bytes)
    for (int i = 0; i < 8; ++i) {
        keccak_state_32[i] = ((uint32_t*)d_header_hash)[i];
    }
    // Load the final mix hash we just calculated (32 bytes)
    for (int i = 0; i < 8; ++i) {
        keccak_state_32[8 + i] = final_mix_hash[i];
    }
    // Load a different part of the "RAVENCOINKAWPOW" constant (36 bytes)
    for (int i = 0; i < 9; ++i) {
        keccak_state_32[16 + i] = d_ravencoin_kawpow[i];
    }

    // Padding for a 100-byte message (32+32+36)
    keccak_state[12] |= 0x0000000000000001;
    keccak_state[16] |= 0x8000000000000000;

    keccak_f1600(keccak_state);

    // --- Step 5: Comparison (Using your existing endian-correct logic) ---
    uint256_t target_val, hash_val;
    for (int i = 0; i < 8; ++i) {
        uint32_t val = 0;
        for (int j = 0; j < 8; ++j) {
            char c = d_target_hex[i * 8 + j];
            uint32_t nibble = (c >= 'a') ? (c - 'a' + 10) : (c - '0');
            val = (val << 4) | nibble;
        }
        target_val.val[7 - i] = val;
    }
    for (int i = 0; i < 8; ++i) {
        hash_val.val[7 - i] = byteswap_32(keccak_state_32[i]);
    }

    if (is_less_or_equal(hash_val, target_val)) {
        if (atomicCAS((unsigned long long*)d_result_nonce, 0, (unsigned long long)nonce) == 0) {
            for (int i = 0; i < 32; ++i) {
                d_result_mix_hash[i] = ((char*)final_mix_hash)[i];
            }
        }
    }
}

// ===================================================================================
// == DAG Management and Main Search Function
// ===================================================================================
void* get_dag(uint64_t block_number, const char* seed_hash_hex, uint64_t& dag_size, int device_id) {
    uint64_t epoch = block_number / 7500;

    // 🔐 Step 1: Ensure a DagCache exists for this device
    {
        std::lock_guard<std::mutex> lock(g_dag_mutex);
        g_dag_caches.try_emplace(device_id);

    }

    // 🔐 Step 2: Work with this device’s DAG cache
    DagCache& cache = g_dag_caches[device_id];
    std::lock_guard<std::mutex> lock(cache.mutex);

    if (cache.epoch == epoch && cache.d_dag != nullptr) {
        dag_size = cache.size;
        return cache.d_dag;
    }

    LOG_INFO << "Generating new DAG for epoch " << epoch << " (Block: " << block_number << ")";

    size_t cache_size;
    void* h_cache = generate_kawpow_light_cache(epoch, seed_hash_hex, cache_size);
    if (!h_cache) return nullptr;

    dag_size = get_kawpow_dag_size(epoch);
    LOG_INFO << "Calculated DAG size: " << dag_size / (1024 * 1024) << " MB";

    // Clean up old DAG
    if (cache.d_dag != nullptr) {
        cudaFree(cache.d_dag);
    }

    uint32_t* d_cache = nullptr;
    uint32_t* d_dag = nullptr;

    if (cudaMalloc(&d_dag, dag_size) != cudaSuccess) {
        LOG_ERROR << "GPU DAG Malloc failed";
        free(h_cache);
        return nullptr;
    }

    if (cudaMalloc(&d_cache, cache_size) != cudaSuccess) {
        LOG_ERROR << "GPU Cache Malloc failed";
        free(h_cache);
        cudaFree(d_dag);
        return nullptr;
    }

    cudaMemcpy(d_cache, h_cache, cache_size, cudaMemcpyHostToDevice);
    free(h_cache);

    dim3 threads_per_block(256);
    dim3 num_blocks((dag_size / 64 + 255) / 256);
    generate_dag_kernel<<<num_blocks, threads_per_block>>>(d_dag, d_cache, dag_size / 64, cache_size / sizeof(uint32_t));
    cudaDeviceSynchronize();
    cudaFree(d_cache);

    cache.epoch = epoch;
    cache.size = dag_size;
    cache.d_dag = d_dag;

    return d_dag;
}

struct uint256 {
    uint32_t val[8]; // little endian or big endian - be consistent
    
    // Divide by uint64_t small number, returns quotient
    uint256 operator/(uint64_t divisor) const {
        uint256 result = {};
        uint64_t remainder = 0;
        for (int i = 7; i >= 0; --i) {
            uint64_t part = (remainder << 32) | val[i];
            result.val[i] = static_cast<uint32_t>(part / divisor);
            remainder = part % divisor;
        }
        return result;
    }
};

uint256 parse_hex_to_uint256(const std::string& hex) {
    uint256 result = {};
    // parse 64 hex chars to 8 x 32-bit words
    for (int i = 0; i < 8; ++i) {
        sscanf(hex.c_str() + i * 8, "%8x", &result.val[7 - i]); // big endian parsing
    }
    return result;
}

std::string uint256_to_hex(const uint256& value) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 7; i >= 0; --i) {
        ss << std::setw(8) << value.val[i];
    }
    return ss.str();
}

extern "C" void kawpow_cuda_search(
    const char* header_hash, const char* seed_hash, uint64_t block_number, 
    uint64_t start_nonce, int device_id, int intensity, 
    KawPow* kawpow_instance, const char* target)
{
    cudaSetDevice(device_id);
    
    uint64_t dag_size;
    uint32_t* d_dag = static_cast<uint32_t*>(get_dag(block_number, seed_hash, dag_size, device_id));
    if (!d_dag) { 
        LOG_ERROR << "Device " << device_id << ": Failed to get DAG."; 
        return; 
    }

    // Allocate GPU memory for kernel parameters
    char *d_header_hash, *d_target_hex, *d_result_mix_hash;
    uint64_t* d_result_nonce;
    
    cudaMalloc(&d_header_hash, 32);
    cudaMalloc(&d_target_hex, 64);
    cudaMalloc(&d_result_nonce, sizeof(uint64_t));
    cudaMalloc(&d_result_mix_hash, 32);
    
    // Copy data to GPU
    cudaMemcpy(d_header_hash, header_hash, 32, cudaMemcpyHostToDevice);
    cudaMemcpy(d_target_hex, target, 64, cudaMemcpyHostToDevice);
    cudaMemset(d_result_nonce, 0, sizeof(uint64_t));

    LOG_INFO << "Device " << device_id << ": Starting search loop for block " << block_number;
    
    uint64_t current_nonce = start_nonce;
    dim3 threads_per_block(256);
    dim3 num_blocks(intensity);

    // 1. Parse pool max target string (hardcoded or from mining.set_target)
    // std::string pool_max_target = "00000000ffff0000000000000000000000000000000000000000000000000000";
    std::string pool_max_target = "00000001ffffffffffffffffffffffffffffffffffffffffffffffffffffffff";

    // 2. Convert to uint256 (or similar big int struct)
    uint256 max_target = parse_hex_to_uint256(pool_max_target);

    // 3. fixed difficulty
    uint64_t fixed_difficulty = 256;

    // 4. Calculate fixed share target
    uint256 fixed_target = max_target / fixed_difficulty;

    // 5. Format fixed_target back to hex string
    std::string fixed_target_hex = uint256_to_hex(fixed_target);

    // 6. Copy fixed_target_hex to GPU as d_target_hex instead of the param `target`
    cudaMemcpy(d_target_hex, fixed_target_hex.c_str(), 64, cudaMemcpyHostToDevice);
    auto start_time = std::chrono::high_resolution_clock::now();
    uint64_t total_hashes = 0;

    while (kawpow_instance->should_continue()) {
        kawpow_kernel<<<num_blocks, threads_per_block>>>(
            d_result_nonce, d_result_mix_hash, d_header_hash, current_nonce,
            d_dag, d_target_hex
        );

        uint64_t h_result_nonce = 0;
        cudaMemcpy(&h_result_nonce, d_result_nonce, sizeof(uint64_t), cudaMemcpyDeviceToHost);

        if (h_result_nonce != 0) {
            char h_mix_hash[32];
            cudaMemcpy(h_mix_hash, d_result_mix_hash, 32, cudaMemcpyDeviceToHost);
            
            // === FIXED: Proper nonce and mix hash formatting ===
            std::stringstream nonce_ss;
            nonce_ss << std::hex << std::setfill('0') << std::setw(16) << h_result_nonce;
            
            std::stringstream mix_ss;
            for(int i = 0; i < 32; ++i) {
                mix_ss << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)h_mix_hash[i];
            }
            
            LOG_INFO << "Device " << device_id << ": Found valid share! Nonce: " << nonce_ss.str();
            kawpow_instance->submit_share(nonce_ss.str(), mix_ss.str());
            
            // Reset for next search
            cudaMemset(d_result_nonce, 0, sizeof(uint64_t));
        }
        
        current_nonce += (uint64_t)num_blocks.x * threads_per_block.x;
        total_hashes += (uint64_t)num_blocks.x * threads_per_block.x;

        auto now = std::chrono::high_resolution_clock::now();
        auto seconds_passed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

        if (seconds_passed >= 5) {
            double hashrate = (double)total_hashes / seconds_passed;
            LOG_INFO << "Device " << device_id << ": ~" << (uint64_t)(hashrate / 1000000.0) << " MH/s";
            // Reset counters
            total_hashes = 0;
            start_time = now;
        }

    }

    // Cleanup
    cudaFree(d_header_hash); 
    cudaFree(d_target_hex); 
    cudaFree(d_result_nonce); 
    cudaFree(d_result_mix_hash);
    
    LOG_INFO << "Device " << device_id << ": Search loop finished.";
}
