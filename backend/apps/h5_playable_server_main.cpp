// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#include "pathfinder/h5_playable/playable_http_server.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace {
std::atomic<bool> g_stop_requested{false};
void signalHandler(int) { g_stop_requested = true; }
} // namespace

int main(int argc, char* argv[]) {
    uint16_t port = 1999;
    std::string host = "127.0.0.1";
    std::string static_root = "frontend/h5_playable";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if ((arg == "--host" || arg == "-h") && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--static-root" && i + 1 < argc) {
            static_root = argv[++i];
        }
    }

    pathfinder::h5_playable::H5PlayableServer server;
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    auto result = server.start(port, host, static_root);
    if (result.is_error()) {
        std::cerr << "Failed to start H5 playable server\n";
        for (const auto& error : result.errors()) {
            std::cerr << " - " << error.message_key;
            if (error.debug_message.has_value()) {
                std::cerr << ": " << error.debug_message.value();
            }
            std::cerr << "\n";
        }
        return 1;
    }

    std::cout << "H5 Playable Server running on http://" << host << ":" << port << "\n";
    std::cout << "Static root: " << static_root << "\n";
    while (server.isRunning() && !g_stop_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    if (server.isRunning()) server.stop();
    return 0;
}
