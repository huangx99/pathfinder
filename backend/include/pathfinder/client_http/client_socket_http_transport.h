#pragma once

#include "pathfinder/client_http/client_http_transport.h"
#include "pathfinder/client_http/client_http_types.h"
#include <atomic>
#include <map>
#include <string>
#include <thread>

namespace pathfinder::client_http {

class ClientSocketHttpTransport final : public IClientHttpTransport {
public:
    ClientSocketHttpTransport();
    ~ClientSocketHttpTransport() override;

    pathfinder::foundation::Result<void> start(
        uint16_t port,
        const std::string& host,
        IClientHttpRouter& router) override;

    void stop() override;
    bool isRunning() const override;
    uint16_t boundPort() const;

private:
    std::atomic<bool> running_{false};
    int server_fd_ = -1;
    std::thread server_thread_;
    IClientHttpRouter* router_ = nullptr;

    void acceptLoop(uint16_t port, const std::string& host);
    void handleClient(int client_fd);

    static std::string readRequest(int client_fd);
    static void sendResponse(int client_fd, const ClientHttpResponse& response);

    static ClientHttpMethod parseMethod(const std::string& request);
    static std::string parsePath(const std::string& request);
    static std::string parseQuery(const std::string& request);
    static std::map<std::string, std::string> parseHeaders(const std::string& request);
    static std::string parseBody(const std::string& request);
};

} // namespace pathfinder::client_http
