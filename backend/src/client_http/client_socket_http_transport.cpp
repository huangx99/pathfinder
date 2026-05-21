#include "pathfinder/client_http/client_socket_http_transport.h"
#include "pathfinder/client_http/client_http_router.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace pathfinder::client_http {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace {

constexpr size_t MAX_BODY_SIZE = 1024 * 1024; // 1MB

} // namespace

ClientSocketHttpTransport::ClientSocketHttpTransport() = default;

ClientSocketHttpTransport::~ClientSocketHttpTransport() {
    stop();
}

Result<void> ClientSocketHttpTransport::start(
    uint16_t port,
    const std::string& host,
    IClientHttpRouter& router) {
    if (running_.load()) return Result<void>::ok();
    router_ = &router;

    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "client_http.socket_failed"));
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (host == "0.0.0.0") ? INADDR_ANY : inet_addr(host.c_str());

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "client_http.bind_failed"));
    }

    if (listen(server_fd_, 16) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return Result<void>::fail(makeError(ErrorCode::common_internal_invariant_broken, "client_http.listen_failed"));
    }

    running_.store(true);
    server_thread_ = std::thread([this, port, host]() { acceptLoop(port, host); });
    return Result<void>::ok();
}

uint16_t ClientSocketHttpTransport::boundPort() const {
    if (server_fd_ < 0) return 0;
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getsockname(server_fd_, reinterpret_cast<sockaddr*>(&addr), &len) == 0) {
        return ntohs(addr.sin_port);
    }
    return 0;
}

void ClientSocketHttpTransport::stop() {
    running_.store(false);
    if (server_fd_ >= 0) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

bool ClientSocketHttpTransport::isRunning() const {
    return running_.load();
}

void ClientSocketHttpTransport::acceptLoop(uint16_t, const std::string&) {
    while (running_.load()) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client_fd < 0) {
            if (running_.load()) {
                std::cerr << "client_http accept error\n";
            }
            continue;
        }
        handleClient(client_fd);
        close(client_fd);
    }
}

void ClientSocketHttpTransport::handleClient(int client_fd) {
    const auto raw_request = readRequest(client_fd);
    if (raw_request.empty()) return;

    ClientHttpRequest request;
    request.method = parseMethod(raw_request);
    request.path = parsePath(raw_request);
    request.query = parseQuery(raw_request);
    request.headers = parseHeaders(raw_request);
    request.body = parseBody(raw_request);

    // Body size limit
    if (request.body.size() > MAX_BODY_SIZE) {
        ClientHttpResponse response;
        response.status_code = 413;
        response.content_type = "application/json; charset=utf-8";
        response.body = "{\"ok\":false,\"error_key\":\"payload_too_large\",\"message\":\"Request body exceeds 1MB\",\"details\":[]}";
        sendResponse(client_fd, response);
        return;
    }

    if (router_) {
        auto response = router_->route(request);
        sendResponse(client_fd, response);
    }
}

std::string ClientSocketHttpTransport::readRequest(int client_fd) {
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
            if (content_length == 0 || request.size() >= header_end + marker_size + content_length) {
                break;
            }
        }
    }
    return request;
}

void ClientSocketHttpTransport::sendResponse(int client_fd, const ClientHttpResponse& response) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << response.status_code;
    switch (response.status_code) {
        case 200: oss << " OK"; break;
        case 400: oss << " Bad Request"; break;
        case 404: oss << " Not Found"; break;
        case 405: oss << " Method Not Allowed"; break;
        case 413: oss << " Payload Too Large"; break;
        case 500: oss << " Internal Server Error"; break;
        default: oss << " Unknown"; break;
    }
    oss << "\r\n";
    oss << "Content-Type: " << response.content_type << "\r\n";
    oss << "Content-Length: " << response.body.size() << "\r\n";
    for (const auto& [k, v] : response.headers) {
        oss << k << ": " << v << "\r\n";
    }
    oss << "Connection: close\r\n\r\n";
    oss << response.body;

    const auto message = oss.str();
    ::send(client_fd, message.data(), message.size(), 0);
}

ClientHttpMethod ClientSocketHttpTransport::parseMethod(const std::string& request) {
    auto pos = request.find(' ');
    if (pos == std::string::npos) return ClientHttpMethod::Unknown;
    auto method = request.substr(0, pos);
    if (method == "GET") return ClientHttpMethod::Get;
    if (method == "POST") return ClientHttpMethod::Post;
    if (method == "OPTIONS") return ClientHttpMethod::Options;
    return ClientHttpMethod::Unknown;
}

std::string ClientSocketHttpTransport::parsePath(const std::string& request) {
    auto first = request.find(' ');
    if (first == std::string::npos) return "";
    auto second = request.find(' ', first + 1);
    if (second == std::string::npos) return "";
    auto full = request.substr(first + 1, second - first - 1);
    auto query_pos = full.find('?');
    if (query_pos != std::string::npos) {
        return full.substr(0, query_pos);
    }
    return full;
}

std::string ClientSocketHttpTransport::parseQuery(const std::string& request) {
    auto first = request.find(' ');
    if (first == std::string::npos) return "";
    auto second = request.find(' ', first + 1);
    if (second == std::string::npos) return "";
    auto full = request.substr(first + 1, second - first - 1);
    auto query_pos = full.find('?');
    if (query_pos != std::string::npos) {
        return full.substr(query_pos + 1);
    }
    return "";
}

std::map<std::string, std::string> ClientSocketHttpTransport::parseHeaders(const std::string& request) {
    std::map<std::string, std::string> headers;
    auto header_end = request.find("\r\n\r\n");
    size_t marker_size = 4;
    if (header_end == std::string::npos) {
        header_end = request.find("\n\n");
        marker_size = 2;
    }
    if (header_end == std::string::npos) return headers;

    auto header_block = request.substr(0, header_end);
    size_t pos = header_block.find("\r\n");
    if (pos == std::string::npos) pos = header_block.find('\n');
    if (pos == std::string::npos) return headers;

    size_t line_start = pos + (header_block[pos] == '\r' ? 2 : 1);
    while (line_start < header_block.size()) {
        size_t line_end = header_block.find("\r\n", line_start);
        if (line_end == std::string::npos) line_end = header_block.find('\n', line_start);
        if (line_end == std::string::npos) line_end = header_block.size();

        auto line = header_block.substr(line_start, line_end - line_start);
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            auto key = line.substr(0, colon);
            auto value = line.substr(colon + 1);
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
                value = value.substr(1);
            }
            headers[key] = value;
        }

        line_start = line_end + (header_block[line_end] == '\r' ? 2 : 1);
    }
    return headers;
}

std::string ClientSocketHttpTransport::parseBody(const std::string& request) {
    auto header_end = request.find("\r\n\r\n");
    size_t marker_size = 4;
    if (header_end == std::string::npos) {
        header_end = request.find("\n\n");
        marker_size = 2;
    }
    if (header_end == std::string::npos) return "";
    return request.substr(header_end + marker_size);
}

} // namespace pathfinder::client_http
