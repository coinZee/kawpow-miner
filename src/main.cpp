#include <iostream>
#include "config.h"
#include "stratum.h"
#include "kawpow.h"
#include "logging.h"

std::mutex log_mutex;

int main(int argc, char** argv) {
    LOG_INFO << "KawPow Miner v3 starting up...";
    
    // Load configuration
    LOG_INFO << "Loading configuration...";
    Config config;
    if (!config.load("config/config.json")) {
        LOG_ERROR << "Failed to load config.json - exiting";
        return 1;
    }

    // Initialize KawPoW
    LOG_INFO << "Initializing KawPoW mining engine...";
    KawPow kawpow(config);

    // Initialize and run Stratum client
    LOG_INFO << "Starting Stratum client...";
    Stratum stratum(config, kawpow);
    
    try {
        stratum.run();
    } catch (const std::exception& e) {
        LOG_ERROR << "Fatal error: " << e.what();
        return 1;
    } catch (...) {
        LOG_ERROR << "Unknown fatal error occurred";
        return 1;
    }

    LOG_INFO << "Miner shutting down...";
    return 0;
}

