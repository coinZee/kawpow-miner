#pragma once

#include "config.h"
#include "kawpow.h"
#include <string>

class Stratum {
public:
    Stratum(const Config& config, KawPow& kawpow);
    void run();

private:
    void connect();
    void handle_message(const std::string& message);
    void subscribe();
    void authorize();
    void submit(const std::string& job_id, const std::string& nonce, const std::string& mix_hash);

    const Config& config;
    KawPow& kawpow;
    int sock;
    std::string session_id;
    std::string current_job_id;
};

