// include/logging.h
#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <mutex>

extern std::mutex log_mutex;

#define LOG(level, color) \
    std::lock_guard<std::mutex> lock(log_mutex); \
    do { \
        auto now = std::chrono::system_clock::now(); \
        auto in_time_t = std::chrono::system_clock::to_time_t(now); \
        std::cout << "\033[" << color << "m[" \
                  << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") \
                  << " " << level << "] "

#define LOG_INFO LOG("INFO ", "32") <<
#define LOG_WARN LOG("WARN ", "33") <<
#define LOG_ERROR LOG("ERROR", "31") <<
#define LOG_CUDA LOG("CUDA ", "36") <<
#define LOG_STRATUM LOG("STRATUM", "35") <<

#define ENDL "\033(const boost::system::error_code& ec, const tcp::endpoint& endpoint) {
            if (!ec) {
                LOG_STRATUM << "Connected to " << config_.pool.url << ":" << config_.pool.port << ENDL;
                connected_ = true;
                subscribe();
                do_read();
            } else {
                LOG_ERROR("Stratum connect error: " << ec.message() << ENDL;
                // Implement reconnect logic here if desired
            }
        });
}

void StratumClient::subscribe() {
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("id", message_id_++, d.GetAllocator());
    d.AddMember("method", "mining.subscribe", d.GetAllocator());
    rapidjson::Value params(rapidjson::kArrayType);
    params.PushBack("rvn-miner/0.1", d.GetAllocator());
    d.AddMember("params", params, d.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    
    std::string msg = buffer.GetString();
    msg += "\n";
    
    write_msgs_.push_back(msg);
    do_write();
}

void StratumClient::authorize() {
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("id", message_id_++, d.GetAllocator());
    d.AddMember("method", "mining.authorize", d.GetAllocator());
    rapidjson::Value params(rapidjson::kArrayType);
    params.PushBack(rapidjson::Value(config_.pool.user.c_str(), d.GetAllocator()), d.GetAllocator());
    params.PushBack(rapidjson::Value(config_.pool.pass.c_str(), d.GetAllocator()), d.GetAllocator());
    d.AddMember("params", params, d.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    std::string msg = buffer.GetString();
    msg += "\n";

    write_msgs_.push_back(msg);
    if (write_msgs_.size() == 1) {
        do_write();
    }
}

void StratumClient::submit(const std::string& job_id, const std::string& nonce_hex, const std::string& mix_hash_hex) {
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("id", message_id_++, d.GetAllocator());
    d.AddMember("method", "mining.submit", d.GetAllocator());
    rapidjson::Value params(rapidjson::kArrayType);
    params.PushBack(rapidjson::Value(config_.pool.user.c_str(), d.GetAllocator()), d.GetAllocator());
    params.PushBack(rapidjson::Value(job_id.c_str(), d.GetAllocator()), d.GetAllocator());
    params.PushBack(rapidjson::Value(nonce_hex.c_str(), d.GetAllocator()), d.GetAllocator());
    params.PushBack(rapidjson::Value(mix_hash_hex.c_str(), d.GetAllocator()), d.GetAllocator());
    d.AddMember("params", params, d.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    std::string msg = buffer.GetString();
    msg += "\n";

    asio::post(io_context_, [this, msg]() {
        bool write_in_progress =!write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress) {
            do_write();
        }
    });
}

void StratumClient::do_read() {
    asio::async_read_until(socket_, read_buffer_, '\n',
        [this](const boost::system::error_code& ec, std::size_t length) {
            if (!ec) {
                std::istream is(&read_buffer_);
                std::string line;
                std::getline(is, line);
                process_line(line);
                do_read();
            } else {
                LOG_ERROR("Stratum read error: " << ec.message() << ENDL;
                disconnect();
            }
        });
}

void StratumClient::do_write() {
    if (write_msgs_.empty()) return;
    asio::async_write(socket_, asio::buffer(write_msgs_.front()),
        [this](const boost::system::error_code& ec, std::size_t /*length*/) {
            if (!ec) {
                write_msgs_.pop_front();
                if (!write_msgs_.empty()) {
                    do_write();
                }
            } else {
                LOG_ERROR("Stratum write error: " << ec.message() << ENDL;
                disconnect();
            }
        });
}

void StratumClient::process_line(const std::string& line) {
    if (line.empty()) return;
    LOG_STRATUM << "RECV: " << line << ENDL;

    try {
        auto j = nlohmann::json::parse(line);
        if (j.contains("method")) {
            handle_notification(j);
        } else if (j.contains("result")) {
            if (j["id"] == 1) { // Subscription response
                auto result = j["result"];
                extranonce1_ = result.[1]get<std::string>();
                extranonce2_size_ = result.[2]get<int>();
                LOG_STRATUM << "Subscribed. Extranonce1: " << extranonce1_ << ", Extranonce2_size: " << extranonce2_size_ << ENDL;
                authorize();
            } else if (j.contains("result") && j["result"].is_boolean() && j["result"].get<bool>()) {
                 LOG_STRATUM << "Share accepted." << ENDL;
            } else if (j.contains("error") &&!j["error"].is_null()) {
                 LOG_ERROR("Share rejected: " << j["error"].dump() << ENDL;
            }
        }
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("JSON parse error: " << e.what() << ENDL;
    }
}

void StratumClient::handle_notification(const nlohmann::json& notification) {
    std::string method = notification["method"];
    if (method == "mining.notify") {
        auto params = notification["params"];
        StratumJob job;
        job.job_id = params.get<std::string>();
        job.header_hash = params.[1]get<std::string>();
        job.seed_hash = params.[2]get<std::string>();
        job.height = std::stoull(params.[3]get<std::string>(), nullptr, 16);
        
        std::string target_hex = params.[4]get<std::string>();
        uint64_t target_val = std::stoull(target_hex, nullptr, 16);
        job.target = (0xFFFFFFFFFFFFFFFFULL / target_val);

        LOG_STRATUM << "New job " << job.job_id << " at height " << job.height << ENDL;
        if (on_new_job) {
            on_new_job(job);
        }
    } else if (method == "mining.set_difficulty") {
        double difficulty = notification["params"].get<double>();
        LOG_STRATUM << "Difficulty set to " << difficulty << ENDL;
        // The target is sent with each job in KawPoW stratum, so we don't need to store this.
    }
}

