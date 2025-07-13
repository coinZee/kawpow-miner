#pragma once

#include <string>
#include <functional>
#include "config.h"
#include "kawpow.h"

class Stratum {
public:
    Stratum(const Config& config, KawPow& kawpow);
    void run();
    // NEW, CORRECTED VERSION
    void submit(const std::string& job_id, const std::string& nonce_hex, const std::string& header_hash_hex, const std::string& mix_hash_hex);
private:
    void connect();
    void subscribe();
    void authorize();
    void handle_message(const std::string& message);
    void process_single_message(const std::string& message);
    
    const Config& config;
    KawPow& kawpow;
    int sock;
    std::string session_id;
    std::string current_job_id;
    std::string current_header_hash;
    std::string current_target;
};

