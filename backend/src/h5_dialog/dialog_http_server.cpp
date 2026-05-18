#include "pathfinder/h5_dialog/dialog_http_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {

std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string extractPath(const std::string& request) {
    size_t first = request.find(' ');
    if (first == std::string::npos) return "";
    size_t second = request.find(' ', first + 1);
    if (second == std::string::npos) return "";
    return request.substr(first + 1, second - first - 1);
}

std::string extractMethod(const std::string& request) {
    size_t sp = request.find(' ');
    if (sp == std::string::npos) return "";
    return request.substr(0, sp);
}

std::string extractBody(const std::string& request) {
    size_t pos = request.find("\r\n\r\n");
    if (pos == std::string::npos) {
        pos = request.find("\n\n");
        if (pos == std::string::npos) return "";
        return request.substr(pos + 2);
    }
    return request.substr(pos + 4);
}

} // namespace

Result<void> H5DialogServer::start(uint16_t port, const std::string& host, const std::string& static_root) {
    static_root_ = static_root;
    running_ = true;

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "socket creation failed"));
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (host == "0.0.0.0") {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    }

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "bind failed"));
    }

    if (listen(server_fd_, 10) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "listen failed"));
    }

    server_thread_ = std::thread([this, port, host]() {
        this->acceptLoop(port, host);
    });

    return Result<void>::ok();
}

void H5DialogServer::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

bool H5DialogServer::isRunning() const {
    return running_;
}

void H5DialogServer::acceptLoop(uint16_t port, const std::string& host) {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client_fd < 0) {
            if (running_) {
                std::cerr << "accept error\n";
            }
            continue;
        }
        handleClient(client_fd);
        close(client_fd);
    }
}

void H5DialogServer::handleClient(int client_fd) {
    std::string request = readRequest(client_fd);
    if (request.empty()) return;

    std::string method = extractMethod(request);
    std::string path = extractPath(request);

    if (method == "GET" && path == "/") {
        auto content = getStaticFile("/index.html");
        if (content.empty()) {
            sendResponse(client_fd, 404, "text/plain", "Not Found");
        } else {
            sendResponse(client_fd, 200, "text/html; charset=utf-8", content);
        }
    } else if (method == "GET" && path == "/style.css") {
        auto content = getStaticFile("/style.css");
        sendResponse(client_fd, 200, "text/css; charset=utf-8", content);
    } else if (method == "GET" && path == "/app.js") {
        auto content = getStaticFile("/app.js");
        sendResponse(client_fd, 200, "application/javascript; charset=utf-8", content);
    } else if (method == "POST" && path == "/api/dialog") {
        auto body = extractBody(request);
        auto req_r = wire_codec_.parseFormUrlEncodedRequest(body);
        if (req_r.is_error()) {
            sendResponse(client_fd, 400, "application/json; charset=utf-8", "{\"error\":\"bad_request\"}");
            return;
        }
        auto resp_r = turn_service_.handle(req_r.value());
        if (resp_r.is_error()) {
            sendResponse(client_fd, 500, "application/json; charset=utf-8", "{\"error\":\"internal_error\"}");
            return;
        }
        auto json_r = wire_codec_.encodeJsonResponse(resp_r.value());
        if (json_r.is_error()) {
            sendResponse(client_fd, 500, "application/json; charset=utf-8", "{\"error\":\"encode_error\"}");
            return;
        }
        sendResponse(client_fd, 200, "application/json; charset=utf-8", json_r.value());
    } else {
        sendResponse(client_fd, 404, "text/plain", "Not Found");
    }
}

std::string H5DialogServer::readRequest(int client_fd) {
    std::string request;
    char buf[4096];
    while (true) {
        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        request.append(buf, n);
        if (request.find("\r\n\r\n") != std::string::npos || request.find("\n\n") != std::string::npos) {
            // For POST with body, we might need more data, but for simplicity assume small bodies
            if (request.find("POST") == 0) {
                // Try to read content-length if present
                size_t cl_pos = request.find("Content-Length:");
                if (cl_pos != std::string::npos) {
                    size_t cl_end = request.find("\r\n", cl_pos);
                    if (cl_end == std::string::npos) cl_end = request.find("\n", cl_pos);
                    std::string cl_str = request.substr(cl_pos + 15, cl_end - cl_pos - 15);
                    // trim
                    while (!cl_str.empty() && (cl_str.front() == ' ' || cl_str.front() == '\t')) cl_str.erase(0, 1);
                    try {
                        size_t content_length = std::stoull(cl_str);
                        size_t header_end = request.find("\r\n\r\n");
                        if (header_end == std::string::npos) header_end = request.find("\n\n");
                        size_t body_start = (header_end == std::string::npos) ? request.size() : header_end + 4;
                        if (request.size() >= body_start + content_length) break;
                        // Need more data
                        continue;
                    } catch (...) {
                        break;
                    }
                }
            }
            break;
        }
    }
    return request;
}

void H5DialogServer::sendResponse(int client_fd, int status_code, const std::string& content_type, const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code;
    if (status_code == 200) oss << " OK";
    else if (status_code == 404) oss << " Not Found";
    else if (status_code == 400) oss << " Bad Request";
    else if (status_code == 500) oss << " Internal Server Error";
    oss << "\r\n";
    oss << "Content-Type: " << content_type << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    auto msg = oss.str();
    send(client_fd, msg.data(), msg.size(), 0);
}

std::string H5DialogServer::getStaticFile(const std::string& path) const {
    if (path.empty() || path[0] != '/') return "";
    std::string safe_path = path;
    // Prevent directory traversal
    if (safe_path.find("..") != std::string::npos) return "";
    std::string full = static_root_ + safe_path;
    return readFile(full);
}

std::string H5DialogServer::mimeType(const std::string& path) const {
    if (path.ends_with(".html")) return "text/html; charset=utf-8";
    if (path.ends_with(".css")) return "text/css; charset=utf-8";
    if (path.ends_with(".js")) return "application/javascript; charset=utf-8";
    return "text/plain; charset=utf-8";
}

} // namespace pathfinder::h5_dialog
