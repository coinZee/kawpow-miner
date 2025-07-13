#include <iostream>
#include "config.h"
#include "stratum.h"
#include "kawpow.h"

int main(int argc, char** argv) {
    // Load configuration
    Config config;
    if (!config.load("config/config.json")) {
        std::cerr << "Failed to load config.json" << std::endl;
        return 1;
    }

    // Initialize KawPoW
    KawPow kawpow(config);

    // Initialize and run Stratum client
    Stratum stratum(config, kawpow);
    stratum.run();

    return 0;
}

