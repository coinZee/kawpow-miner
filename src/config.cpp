#include "config.h"
#include <fstream>
#include <iostream>
#include "rapidjson/istreamwrapper.h"

bool Config::load(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "Could not open config file: " << filename << std::endl;
        return false;
    }

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasMember("pools")) {
        const rapidjson::Value& pools_val = doc["pools"];
        for (auto& p : pools_val.GetArray()) {
            PoolConfig pool;
            pool.url = p["url"].GetString();
            pool.user = p["user"].GetString();
            pool.pass = p["pass"].GetString();
            pools.push_back(pool);
        }
    }

    if (doc.HasMember("cuda")) {
        const rapidjson::Value& cuda_val = doc["cuda"];
        if (cuda_val.HasMember("devices")) {
            const rapidjson::Value& devices_val = cuda_val["devices"];
            for (auto& d : devices_val.GetArray()) {
                CudaDeviceConfig device;
                device.device_id = d["device_id"].GetInt();
                device.intensity = d["intensity"].GetInt();
                cuda_devices.push_back(device);
            }
        }
    }
    
    if (doc.HasMember("api")) {
        const rapidjson::Value& api_val = doc["api"];
        if (api_val.HasMember("port")) {
            api_port = api_val["port"].GetInt();
        }
        if (api_val.HasMember("enabled")) {
            api_enabled = api_val["enabled"].GetBool();
        }
    }

    return true;
}

