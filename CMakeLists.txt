# CMake minimum version
cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0104 NEW)
# Set the CUDA architecture before the project command to ensure
# the initial compiler check uses the correct architecture.
# This prevents errors with newer CUDA toolkits that have dropped
# support for older architectures like sm_30.
set(CMAKE_CUDA_ARCHITECTURES 80)

# Project name and language
project(KawPowMiner CXX CUDA)

# Standard C++17
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CUDA standard
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Find necessary packages
find_package(OpenSSL REQUIRED)

# Add an executable
add_executable(kawpow-miner
    src/main.cpp
    src/config.cpp
    src/stratum.cpp
    src/kawpow.cpp
    src/hashing.cpp
    src/kawpow.cu
)

# Include directories
target_include_directories(kawpow-miner PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(kawpow-miner
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# Install the executable
install(TARGETS kawpow-miner DESTINATION bin)

