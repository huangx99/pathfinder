#pragma once

#include "pathfinder/h5_playable/playable_turn_service.h"
#include "pathfinder/h5_playable/playable_wire_codec.h"
#include <atomic>
#include <string>
#include <thread>

namespace pathfinder::h5_playable {

class H5PlayableServer {
public:
    pathfinder::foundation::Result<void> start(uint16_t port, const std::string& host, const std::string& static_root);
    void stop();
    bool isRunning() const;

private:
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    int server_fd_{-1};
    std::string static_root_;
    H5PlayableTurnService turn_service_;
    H5PlayableWireCodec wire_codec_;

    void acceptLoop(uint16_t port, const std::string& host);
    void handleClient(int client_fd);
    std::string readRequest(int client_fd);
    void sendResponse(int client_fd, int status_code, const std::string& content_type, const std::string& body);
    std::string getStaticFile(const std::string& path) const;
    std::string mimeType(const std::string& path) const;
};

} // namespace pathfinder::h5_playable
