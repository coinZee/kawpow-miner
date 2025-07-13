#include "config.h"
#include <fstream>
#include <iostream>
#include "rapidjson/istreamwrapper.h"
#include "logging.h"

#ifndef ETHASH_H
#define ETHASH_H

#include <stdint.h>

struct ethash_h256 {
    uint8_t bytes[32];
};

typedef struct ethash_h256 ethash_h256_t;

#endif

bool Config::load(const std::string& filename) {
    LOG_INFO << "Loading configuration from: " << filename;
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        LOG_ERROR << "Could not open config file: " << filename;
        return false;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        LOG_ERROR << "Failed to parse config file: " << doc.GetParseError();
        return false;
    }

    LOG_INFO << "Parsing pool configuration...";
    if (doc.HasMember("pools")) {
        const rapidjson::Value& pools_val = doc["pools"];
        for (auto& p : pools_val.GetArray()) {
            PoolConfig pool;
            pool.url = p["url"].GetString();
            pool.user = p["user"].GetString();
            pool.pass = p["pass"].GetString();
            pools.push_back(pool);
            LOG_INFO << "Added pool: " << pool.url << " with user: " << pool.user;
        }
    } else {
        LOG_WARN << "No pools configured in config file";
    }

    LOG_INFO << "Parsing CUDA device configuration...";
    if (doc.HasMember("cuda")) {
        const rapidjson::Value& cuda_val = doc["cuda"];
        if (cuda_val.HasMember("devices")) {
            const rapidjson::Value& devices_val = cuda_val["devices"];
            for (auto& d : devices_val.GetArray()) {
                CudaDeviceConfig device;
                device.device_id = d["device_id"].GetInt();
                device.intensity = d["intensity"].GetInt();
                cuda_devices.push_back(device);
                LOG_INFO << "Added CUDA device " << device.device_id << " with intensity " << device.intensity;
            }
        } else {
            LOG_WARN << "No CUDA devices configured in config file";
        }
    } else {
        LOG_WARN << "No CUDA configuration found in config file";
    }
    
    LOG_INFO << "Parsing API configuration...";
    if (doc.HasMember("api")) {
        const rapidjson::Value& api_val = doc["api"];
        if (api_val.HasMember("port")) {
            api_port = api_val["port"].GetInt();
            LOG_INFO << "API port set to: " << api_port;
        }
        if (api_val.HasMember("enabled")) {
            api_enabled = api_val["enabled"].GetBool();
            LOG_INFO << "API " << (api_enabled ? "enabled" : "disabled");
        }
    } else {
        LOG_WARN << "No API configuration found in config file";
    }

    LOG_INFO << "Configuration loaded successfully";
    return true;
}

