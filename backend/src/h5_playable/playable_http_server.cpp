// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#include "pathfinder/h5_playable/playable_http_server.h"
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace pathfinder::h5_playable {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string extractMethod(const std::string& request) {
    auto pos = request.find(' ');
    return pos == std::string::npos ? std::string{} : request.substr(0, pos);
}

std::string extractPath(const std::string& request) {
    auto first = request.find(' ');
    if (first == std::string::npos) return "";
    auto second = request.find(' ', first + 1);
    if (second == std::string::npos) return "";
    auto path = request.substr(first + 1, second - first - 1);
    auto query_pos = path.find('?');
    if (query_pos != std::string::npos) path = path.substr(0, query_pos);
    return path;
}

std::string extractBody(const std::string& request) {
    auto pos = request.find("\r\n\r\n");
    if (pos != std::string::npos) return request.substr(pos + 4);
    pos = request.find("\n\n");
    if (pos != std::string::npos) return request.substr(pos + 2);
    return "";
}

} // namespace

Result<void> H5PlayableServer::start(uint16_t port, const std::string& host, const std::string& static_root) {
    if (running_) return Result<void>::ok();
    static_root_ = static_root;

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "h5_playable.socket_failed"));

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = host == "0.0.0.0" ? INADDR_ANY : inet_addr(host.c_str());

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "h5_playable.bind_failed"));
    }

    if (listen(server_fd_, 16) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "h5_playable.listen_failed"));
    }

    running_ = true;
    server_thread_ = std::thread([this, port, host]() { acceptLoop(port, host); });
    return Result<void>::ok();
}

void H5PlayableServer::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) server_thread_.join();
}

bool H5PlayableServer::isRunning() const {
    return running_;
}

void H5PlayableServer::acceptLoop(uint16_t, const std::string&) {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client_fd < 0) {
            if (running_) std::cerr << "h5_playable accept error\n";
            continue;
        }
        handleClient(client_fd);
        close(client_fd);
    }
}

void H5PlayableServer::handleClient(int client_fd) {
    const auto request = readRequest(client_fd);
    if (request.empty()) return;
    const auto method = extractMethod(request);
    const auto path = extractPath(request);

    if (method == "GET" && path == "/") {
        auto content = getStaticFile("/index.html");
        sendResponse(client_fd, content.empty() ? 404 : 200, content.empty() ? "text/plain; charset=utf-8" : "text/html; charset=utf-8", content.empty() ? "Not Found" : content);
        return;
    }
    if (method == "GET" && (path == "/style.css" || path == "/app.js")) {
        auto content = getStaticFile(path);
        sendResponse(client_fd, content.empty() ? 404 : 200, mimeType(path), content.empty() ? "Not Found" : content);
        return;
    }
    if (method == "POST" && path == "/api/playable/bootstrap") {
        auto parsed = wire_codec_.parseBootstrapForm(extractBody(request));
        if (parsed.is_error()) {
            sendResponse(client_fd, 400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"bad_bootstrap_request\"}");
            return;
        }
        auto response = turn_service_.bootstrap(parsed.value());
        if (response.is_error()) {
            sendResponse(client_fd, 500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"bootstrap_failed\"}");
            return;
        }
        auto encoded = wire_codec_.encodeResponse(response.value());
        if (encoded.is_error()) {
            sendResponse(client_fd, 500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"encode_failed\"}");
            return;
        }
        sendResponse(client_fd, 200, "application/json; charset=utf-8", encoded.value());
        return;
    }
    if (method == "POST" && path == "/api/playable/turn") {
        auto parsed = wire_codec_.parseTurnForm(extractBody(request));
        if (parsed.is_error()) {
            sendResponse(client_fd, 400, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"bad_turn_request\"}");
            return;
        }
        auto response = turn_service_.handleTurn(parsed.value());
        if (response.is_error()) {
            sendResponse(client_fd, 500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"turn_failed\"}");
            return;
        }
        auto encoded = wire_codec_.encodeResponse(response.value());
        if (encoded.is_error()) {
            sendResponse(client_fd, 500, "application/json; charset=utf-8", "{\"ok\":false,\"error\":\"encode_failed\"}");
            return;
        }
        sendResponse(client_fd, 200, "application/json; charset=utf-8", encoded.value());
        return;
    }

    sendResponse(client_fd, 404, "text/plain; charset=utf-8", "Not Found");
}

std::string H5PlayableServer::readRequest(int client_fd) {
    std::string request;
    char buffer[4096];
    while (true) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        request.append(buffer, n);
        auto header_end = request.find("\r\n\r\n");
        size_t marker_size = 4;
        if (header_end == std::string::npos) {
            header_end = request.find("\n\n");
            marker_size = 2;
        }
        if (header_end != std::string::npos) {
            size_t content_length = 0;
            auto cl_pos = request.find("Content-Length:");
            if (cl_pos != std::string::npos) {
                auto cl_end = request.find("\r\n", cl_pos);
                if (cl_end == std::string::npos) cl_end = request.find('\n', cl_pos);
                try {
                    auto cl_text = request.substr(cl_pos + 15, cl_end - cl_pos - 15);
                    content_length = std::stoull(cl_text);
                } catch (...) {
                    content_length = 0;
                }
            }
            if (content_length == 0 || request.size() >= header_end + marker_size + content_length) break;
        }
    }
    return request;
}

void H5PlayableServer::sendResponse(int client_fd, int status_code, const std::string& content_type, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code;
    if (status_code == 200) response << " OK";
    else if (status_code == 400) response << " Bad Request";
    else if (status_code == 404) response << " Not Found";
    else if (status_code == 500) response << " Internal Server Error";
    response << "\r\nContent-Type: " << content_type;
    response << "\r\nContent-Length: " << body.size();
    response << "\r\nConnection: close\r\n\r\n";
    response << body;
    const auto message = response.str();
    send(client_fd, message.data(), message.size(), 0);
}

std::string H5PlayableServer::getStaticFile(const std::string& path) const {
    if (path.empty() || path[0] != '/' || path.find("..") != std::string::npos) return "";
    return readFile(static_root_ + path);
}

std::string H5PlayableServer::mimeType(const std::string& path) const {
    if (path.ends_with(".html")) return "text/html; charset=utf-8";
    if (path.ends_with(".css")) return "text/css; charset=utf-8";
    if (path.ends_with(".js")) return "application/javascript; charset=utf-8";
    return "text/plain; charset=utf-8";
}

} // namespace pathfinder::h5_playable
