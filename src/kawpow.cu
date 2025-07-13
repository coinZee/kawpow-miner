#include "kawpow.h"
#include <cuda_runtime.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "libethash/ethash.h"
#include "libethash/ethash_internal.h"

// CUDA kernel - device code only, no logging here
__global__ void kawpow_kernel(const char* header_hash, uint64_t start_nonce, uint32_t* result, uint32_t* d_dag, uint64_t dag_size) {
    uint64_t nonce = start_nonce + blockIdx.x * blockDim.x + threadIdx.x;
    
    // In a real miner, the full KawPoW hashing logic would be implemented here.
    // This includes DAG access and multiple rounds of hashing.
    
    // Simplified simulation of DAG access to make sure we're using the DAG memory
    uint32_t idx = nonce % (dag_size / sizeof(uint32_t));
    uint32_t dag_item = d_dag[idx];
    
    // Simulate finding a valid nonce based on DAG value
    if ((dag_item + nonce) % 1000000 == 0) { // Simulate finding a valid nonce
        *result = nonce;
    }
}

// Simple logging function that doesn't use complex formatting
void cuda_log(const char* level, const char* color, const std::string& message) {
    std::cout << "\033[" << color << "m[" << level << "] " << message << "\033[0m" << std::endl;
}

// Function to log VRAM usage
void log_vram_usage(int device_id) {
    size_t free_mem, total_mem;
    cudaMemGetInfo(&free_mem, &total_mem);
    size_t used_mem = total_mem - free_mem;
    cuda_log("CUDA", "36", "Device " + std::to_string(device_id) + " VRAM usage: " + std::to_string(used_mem / (1024 * 1024)) + " MB / " + std::to_string(total_mem / (1024 * 1024)) + " MB");
}

// Host code with logging
void kawpow_cuda_search(const char* header_hash, uint64_t start_nonce, int device_id, int intensity) {
    cuda_log("CUDA", "36", "Starting CUDA search on device " + std::to_string(device_id) + 
                           " with intensity " + std::to_string(intensity));
    
    cudaError_t err = cudaSetDevice(device_id);
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to set CUDA device " + std::to_string(device_id) + 
                               ": " + std::string(cudaGetErrorString(err)));
        return;
    }

    // Get device properties
    cudaDeviceProp deviceProp;
    err = cudaGetDeviceProperties(&deviceProp, device_id);
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to get device properties: " + 
                               std::string(cudaGetErrorString(err)));
        return;
    }
    
    std::string device_info = "Using device: ";
    device_info += deviceProp.name;
    device_info += " with compute capability ";
    device_info += std::to_string(deviceProp.major);
    device_info += ".";
    device_info += std::to_string(deviceProp.minor);
    cuda_log("CUDA", "36", device_info);

    // Generate and load DAG (simplified, using ethash for KawPoW DAG)
    cuda_log("CUDA", "36", "Generating and loading DAG...");
    
    // Use current block number to determine epoch
    uint64_t current_block = 3929879; // This should come from the stratum job
    uint64_t epoch = current_block / ETHASH_EPOCH_LENGTH;
    
    ethash_light_t light = ethash_light_new(epoch);
    if (!light) {
        cuda_log("ERROR", "31", "Failed to generate DAG");
        return;
    }
    
    // Calculate full DAG size
    uint64_t full_dag_size = ethash_get_datasize(current_block);
    
    // Allocate DAG on GPU
    uint32_t* d_dag;
    size_t dag_size = full_dag_size; // Use full DAG size, not just cache
    err = cudaMalloc(&d_dag, dag_size);
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to allocate DAG memory: " + std::string(cudaGetErrorString(err)));
        ethash_light_delete(light);
        return;
    }
    
    // Generate full DAG in host memory
    void* full_dag = malloc(full_dag_size);
    if (!full_dag) {
        cuda_log("ERROR", "31", "Failed to allocate host memory for full DAG");
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }
    
    // Compute full DAG
    if (!ethash_compute_full_data(full_dag, full_dag_size, light, nullptr)) {
        cuda_log("ERROR", "31", "Failed to compute full DAG");
        free(full_dag);
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }
    
    // Copy DAG to GPU
    err = cudaMemcpy(d_dag, full_dag, dag_size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to copy DAG to device: " + std::string(cudaGetErrorString(err)));
        free(full_dag);
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }
    
    // Free host DAG memory
    free(full_dag);
    
    cuda_log("CUDA", "36", "DAG loaded successfully, size: " + std::to_string(dag_size / (1024 * 1024)) + " MB");

    // Start periodic VRAM logging thread
    std::thread vram_logger([device_id]() {
        while (true) {
            log_vram_usage(device_id);
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
    });
    vram_logger.detach();

    uint32_t* d_result;
    err = cudaMalloc(&d_result, sizeof(uint32_t));
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to allocate device memory: " + 
                               std::string(cudaGetErrorString(err)));
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }
    cuda_log("CUDA", "36", "Allocated device memory for results");

    err = cudaMemset(d_result, 0, sizeof(uint32_t));
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to initialize device memory: " + 
                               std::string(cudaGetErrorString(err)));
        cudaFree(d_result);
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }

    dim3 threads_per_block(256);
    dim3 num_blocks(intensity * 1024);
    cuda_log("CUDA", "36", "Launching kernel with " + std::to_string(num_blocks.x) + 
                          " blocks, " + std::to_string(threads_per_block.x) + " threads per block");

    kawpow_kernel<<<num_blocks, threads_per_block>>>(header_hash, start_nonce, d_result, d_dag, dag_size);
    
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Kernel launch failed: " + 
                               std::string(cudaGetErrorString(err)));
        cudaFree(d_result);
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }
    cuda_log("CUDA", "36", "Kernel launched successfully");

    uint32_t h_result = 0;
    err = cudaMemcpy(&h_result, d_result, sizeof(uint32_t), cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to copy result from device: " + 
                               std::string(cudaGetErrorString(err)));
        cudaFree(d_result);
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }

    if (h_result != 0) {
        cuda_log("CUDA", "36", "Found valid nonce: " + std::to_string(h_result));
        // In a real miner, you would submit this nonce to the stratum server.
    }

    err = cudaFree(d_result);
    if (err != cudaSuccess) {
        cuda_log("ERROR", "31", "Failed to free device memory: " + 
                               std::string(cudaGetErrorString(err)));
        cudaFree(d_dag);
        ethash_light_delete(light);
        return;
    }
    cudaFree(d_dag);
    ethash_light_delete(light);
    cuda_log("CUDA", "36", "Search completed on device " + std::to_string(device_id));
}

