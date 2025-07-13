#pragma once

#include <string>
#include <functional>
#include "config.h"
#include "kawpow.h"

class Stratum {
public:
    Stratum(const Config& config, KawPow& kawpow);
    void run();
    void submit(const std::string& job_id, const std::string& nonce, const std::string& mix_hash);

private:
    void connect();
    void subscribe();
    void authorize();
    void handle_message(const std::string& message);
    void process_single_message(const std::string& message);
    
    Config config;
    KawPow& kawpow;
    int sock;
    std::string session_id;
    std::string current_job_id;
};

