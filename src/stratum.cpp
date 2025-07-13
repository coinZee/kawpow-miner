#include "stratum.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

Stratum::Stratum(const Config& config, KawPow& kawpow) : config(config), kawpow(kawpow), sock(0) {}

void Stratum::run() {
    connect();
    subscribe();
    authorize();

    char buffer[4096];
    while (true) {
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            handle_message(std::string(buffer, bytes_received));
        }
    }
}

void Stratum::connect() {
    const PoolConfig& pool = config.getPools()[0];
    std::string host = pool.url.substr(pool.url.find("://") + 3, pool.url.rfind(":") - (pool.url.find("://") + 3));
    int port = std::stoi(pool.url.substr(pool.url.rfind(":") + 1));

    sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    if (::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        exit(1);
    }
}

void Stratum::handle_message(const std::string& message) {
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasMember("method")) {
        std::string method = doc["method"].GetString();
        if (method == "mining.notify") {
            const rapidjson::Value& params = doc["params"];
            current_job_id = params[0].GetString();
            std::string header_hash = params[1].GetString();
            uint64_t start_nonce = std::stoull(params[7].GetString(), nullptr, 16);
            
            kawpow.set_job(current_job_id, header_hash, start_nonce);
        }
    } else if (doc.HasMember("result")) {
        const rapidjson::Value& result = doc["result"];
        if (!result.IsNull() && result.IsArray()) {
            if (session_id.empty()) {
                session_id = result[1].GetString();
            }
        }
    }
}

void Stratum::subscribe() {
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("id", 1, d.GetAllocator());
    d.AddMember("method", "mining.subscribe", d.GetAllocator());
    rapidjson::Value params(rapidjson::kArrayType);
    params.PushBack("KawPowMiner/0.1", d.GetAllocator());
    d.AddMember("params", params, d.GetAllocator());
    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    std::string msg = std::string(buffer.GetString()) + "\n";
    send(sock, msg.c_str(), msg.length(), 0);
}

void Stratum::authorize() {
    const PoolConfig& pool = config.getPools()[0];
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("id", 2, d.GetAllocator());
    d.AddMember("method", "mining.authorize", d.GetAllocator());
    rapidjson::Value params(rapidjson::kArrayType);
    params.PushBack(rapidjson::Value(pool.user.c_str(), d.GetAllocator()).Move(), d.GetAllocator());
    params.PushBack(rapidjson::Value(pool.pass.c_str(), d.GetAllocator()).Move(), d.GetAllocator());
    d.AddMember("params", params, d.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    std::string msg = std::string(buffer.GetString()) + "\n";
    send(sock, msg.c_str(), msg.length(), 0);
}

void Stratum::submit(const std::string& job_id, const std::string& nonce, const std::string& mix_hash) {
    const PoolConfig& pool = config.getPools()[0];
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("id", 4, d.GetAllocator());
    d.AddMember("method", "mining.submit", d.GetAllocator());
    rapidjson::Value params(rapidjson::kArrayType);
    params.PushBack(rapidjson::Value(pool.user.c_str(), d.GetAllocator()).Move(), d.GetAllocator());
    params.PushBack(rapidjson::Value(job_id.c_str(), d.GetAllocator()).Move(), d.GetAllocator());
    params.PushBack(rapidjson::Value(nonce.c_str(), d.GetAllocator()).Move(), d.GetAllocator());
    params.PushBack(rapidjson::Value(mix_hash.c_str(), d.GetAllocator()).Move(), d.GetAllocator());
    d.AddMember("params", params, d.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);
    std::string msg = std::string(buffer.GetString()) + "\n";
    send(sock, msg.c_str(), msg.length(), 0);
}

