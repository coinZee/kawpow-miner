#include "stratum.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <netdb.h>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "logging.h"

// Connection timeout in seconds
#define CONNECTION_TIMEOUT 10

Stratum::Stratum(const Config& config, KawPow& kawpow) : config(config), kawpow(kawpow), sock(0) {
    LOG_INFO << "Initializing Stratum client";
}

void Stratum::run() {
    LOG_INFO << "Starting Stratum client";
    connect();
    subscribe();
    authorize();

    char buffer[4096];
    LOG_INFO << "Entering main receive loop";
    while (true) {
        // Use poll to wait for data with a timeout
        struct pollfd fds;
        fds.fd = sock;
        fds.events = POLLIN;
        int poll_result = poll(&fds, 1, 30000); // 30 second timeout
        
        if (poll_result < 0) {
            LOG_ERROR << "Poll error: " << strerror(errno);
            break;
        } else if (poll_result == 0) {
            // Timeout - no data received
            LOG_INFO << "No data received for 30 seconds, connection still active";
            continue;
        }
        
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            LOG_STRATUM << "Received " << bytes_received << " bytes";
            handle_message(std::string(buffer, bytes_received));
        } else if (bytes_received == 0) {
            LOG_ERROR << "Connection closed by pool";
            LOG_INFO << "Attempting to reconnect...";
            close(sock);
            connect();
            subscribe();
            authorize();
        } else {
            LOG_ERROR << "Error receiving data: " << strerror(errno);
            if (errno == ECONNRESET || errno == ETIMEDOUT) {
                LOG_INFO << "Connection reset or timed out, attempting to reconnect...";
                close(sock);
                connect();
                subscribe();
                authorize();
            } else {
                break;
            }
        }
    }
}

void Stratum::connect() {
    const PoolConfig& pool = config.getPools()[0];
    
    // Improved URL parsing
    std::string url = pool.url;
    std::string host;
    int port;
    
    // Check if URL has protocol prefix
    size_t protocol_pos = url.find("://");
    if (protocol_pos != std::string::npos) {
        url = url.substr(protocol_pos + 3); // Remove protocol part
    }
    
    // Extract host and port
    size_t port_pos = url.rfind(":");
    if (port_pos != std::string::npos) {
        host = url.substr(0, port_pos);
        port = std::stoi(url.substr(port_pos + 1));
    } else {
        // Default port if not specified
        host = url;
        port = 3333; // Default stratum port
    }

    LOG_INFO << "Connecting to pool " << host << ":" << port;
    LOG_INFO << "Resolving hostname...";

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR << "Failed to create socket: " << strerror(errno);
        exit(1);
    }

    // Set socket to non-blocking mode for timeout support
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Prepare server address
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Try to resolve hostname
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        // If direct IP conversion fails, try to resolve as hostname
        struct hostent *he;
        he = gethostbyname(host.c_str());
        if (he == nullptr) {
            LOG_ERROR << "Failed to resolve hostname: " << host << " - " << strerror(errno);
            close(sock);
            exit(1);
        }
        
        LOG_INFO << "Hostname resolved successfully";
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    } else {
        LOG_INFO << "IP address provided, no resolution needed";
    }

    LOG_INFO << "Attempting connection to " << inet_ntoa(server_addr.sin_addr) << ":" << port;
    
    // Connect with timeout
    int result = ::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result < 0 && errno != EINPROGRESS) {
        LOG_ERROR << "Connection failed immediately: " << strerror(errno);
        close(sock);
        exit(1);
    }

    if (result < 0 && errno == EINPROGRESS) {
        LOG_INFO << "Connection in progress, waiting up to " << CONNECTION_TIMEOUT << " seconds...";
        
        // Wait for connection with timeout
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);
        
        struct timeval timeout;
        timeout.tv_sec = CONNECTION_TIMEOUT;
        timeout.tv_usec = 0;
        
        result = select(sock + 1, NULL, &write_fds, NULL, &timeout);
        
        if (result < 0) {
            LOG_ERROR << "Connection error: " << strerror(errno);
            close(sock);
            exit(1);
        } else if (result == 0) {
            LOG_ERROR << "Connection timed out after " << CONNECTION_TIMEOUT << " seconds";
            close(sock);
            exit(1);
        }
        
        // Check if connection was successful
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            LOG_ERROR << "Connection failed: " << strerror(error ? error : errno);
            close(sock);
            exit(1);
        }
    }
    
    // Set socket back to blocking mode
    fcntl(sock, F_SETFL, flags);
    
    LOG_INFO << "Successfully connected to pool";
}

void Stratum::handle_message(const std::string& message) {
    // Split the message if it contains multiple JSON objects
    std::string buffer = message;
    size_t pos = 0;
    std::vector<std::string> messages;
    
    // Try to find complete JSON objects
    while (pos < buffer.length()) {
        // Find the start of a JSON object
        size_t start = buffer.find_first_of('{', pos);
        if (start == std::string::npos) break;
        
        // Find the matching closing brace
        int brace_count = 1;
        size_t end = start + 1;
        
        while (end < buffer.length() && brace_count > 0) {
            if (buffer[end] == '{') brace_count++;
            else if (buffer[end] == '}') brace_count--;
            end++;
        }
        
        if (brace_count == 0) {
            // Found a complete JSON object
            std::string json_obj = buffer.substr(start, end - start);
            messages.push_back(json_obj);
            pos = end;
        } else {
            // Incomplete JSON object
            break;
        }
    }
    
    // Process each JSON message
    if (messages.empty()) {
        LOG_ERROR << "Failed to parse any complete JSON objects from message";
        LOG_ERROR << "Raw message: " << message;
        return;
    }
    
    LOG_INFO << "Split message into " << messages.size() << " JSON objects";
    
    for (const auto& msg : messages) {
        process_single_message(msg);
    }
}

void Stratum::process_single_message(const std::string& message) {
    LOG_STRATUM << "Processing JSON object: " << message;
    
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError()) {
        LOG_ERROR << "Failed to parse stratum message: " << doc.GetParseError();
        LOG_ERROR << "Error offset: " << doc.GetErrorOffset();
        LOG_ERROR << "Raw message: " << message;
        return;
    }

    if (doc.HasMember("method")) {
        std::string method = doc["method"].GetString();
        LOG_STRATUM << "Received method: " << method;
        
        if (method == "mining.notify" && doc.HasMember("params")) {
            const rapidjson::Value& params = doc["params"];
            
            // Add detailed debugging
            LOG_INFO << "Params type: " << (params.IsArray() ? "array" : "not array") 
                     << ", size: " << (params.IsArray() ? std::to_string(params.Size()) : "N/A");
            
            // Add bounds checking for minimum required parameters
            if (!params.IsArray() || params.Size() < 7) {
                LOG_ERROR << "Invalid mining.notify params format: expected at least 7 elements in array";
                return;
            }
            
            // Safe array access with bounds checking
            if (params.Size() > 0 && params[0].IsString()) {
                current_job_id = params[0].GetString();
            } else {
                LOG_ERROR << "Invalid job_id in mining.notify params";
                return;
            }
            
            std::string header_hash;
            if (params.Size() > 1 && params[1].IsString()) {
                header_hash = params[1].GetString();
            } else {
                LOG_ERROR << "Invalid header_hash in mining.notify params";
                return;
            }
            
            uint64_t start_nonce = 0;
            if (params.Size() > 7 && params[7].IsString()) {
                try {
                    start_nonce = std::stoull(params[7].GetString(), nullptr, 16);
                } catch (const std::exception& e) {
                    LOG_ERROR << "Failed to parse start_nonce: " << e.what();
                    return;
                }
            } else {
                LOG_WARN << "No start_nonce provided, using 0 as default";
            }
            
            LOG_STRATUM << "New job received - ID: " << current_job_id 
                       << ", Header hash: " << header_hash 
                       << ", Start nonce: " << start_nonce;
            
            kawpow.set_job(current_job_id, header_hash, start_nonce);
        } else if (method == "mining.set_target" && doc.HasMember("params")) {
            const rapidjson::Value& params = doc["params"];
            if (params.IsArray() && params.Size() > 0 && params[0].IsString()) {
                LOG_STRATUM << "Target set to: " << params[0].GetString();
            }
        }
    } else if (doc.HasMember("result")) {
        const rapidjson::Value& result = doc["result"];
        if (!result.IsNull() && result.IsArray() && result.Size() > 1) {
            if (session_id.empty() && result[1].IsString()) {
                session_id = result[1].GetString();
                LOG_STRATUM << "Session ID received: " << session_id;
            }
        }
        if (doc.HasMember("id")) {
            LOG_STRATUM << "Received response for request ID: " << doc["id"].GetInt();
        }
    } else if (doc.HasMember("error") && !doc["error"].IsNull()) {
        LOG_ERROR << "Stratum error received: " << doc["error"].GetString();
    }
}

void Stratum::subscribe() {
    LOG_INFO << "Subscribing to mining service";
    
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
    
    LOG_STRATUM << "Sending subscription request: " << msg;
    if (send(sock, msg.c_str(), msg.length(), 0) < 0) {
        LOG_ERROR << "Failed to send subscription request: " << strerror(errno);
    }
}

void Stratum::authorize() {
    LOG_INFO << "Authorizing with mining pool";
    
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
    
    LOG_STRATUM << "Sending authorization request for user: " << pool.user;
    if (send(sock, msg.c_str(), msg.length(), 0) < 0) {
        LOG_ERROR << "Failed to send authorization request: " << strerror(errno);
    }
}

void Stratum::submit(const std::string& job_id, const std::string& nonce, const std::string& mix_hash) {
    LOG_INFO << "Submitting share - Job ID: " << job_id << ", Nonce: " << nonce;
    
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
    
    LOG_STRATUM << "Sending share submission: " << msg;
    if (send(sock, msg.c_str(), msg.length(), 0) < 0) {
        LOG_ERROR << "Failed to submit share: " << strerror(errno);
    }
}

