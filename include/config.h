#pragma once

#include <string>
#include <vector>
#include "rapidjson/document.h"

struct CudaDeviceConfig {
    int device_id;
    int intensity;
};

struct PoolConfig {
    std::string url;
    std::string user;
    std::string pass;
};

class Config {
public:
    bool load(const std::string& filename);

    const std::vector<PoolConfig>& getPools() const { return pools; }
    const std::vector<CudaDeviceConfig>& getCudaDevices() const { return cuda_devices; }
    int getApiPort() const { return api_port; }
    bool isApiEnabled() const { return api_enabled; }

private:
    std::vector<PoolConfig> pools;
    std::vector<CudaDeviceConfig> cuda_devices;
    int api_port;
    bool api_enabled;
};

