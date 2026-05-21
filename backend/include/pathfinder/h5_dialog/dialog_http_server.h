// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#pragma once

#include "pathfinder/h5_dialog/dialog_turn_service.h"
#include "pathfinder/h5_dialog/dialog_wire_codec.h"
#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace pathfinder::h5_dialog {

class H5DialogServer {
public:
    pathfinder::foundation::Result<void> start(
        uint16_t port,
        const std::string& host,
        const std::string& static_root);

    void stop();

    bool isRunning() const;

private:
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    int server_fd_ = -1;
    std::string static_root_;
    DialogTurnService turn_service_;
    DialogWireCodec wire_codec_;

    void acceptLoop(uint16_t port, const std::string& host);
    void handleClient(int client_fd);
    std::string readRequest(int client_fd);
    void sendResponse(int client_fd, int status_code, const std::string& content_type, const std::string& body);
    std::string getStaticFile(const std::string& path) const;
    std::string mimeType(const std::string& path) const;
};

} // namespace pathfinder::h5_dialog
