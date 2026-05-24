#include "pathfinder/client_http/client_socket_http_transport.h"
#include "pathfinder/client_http/client_static_file_service.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/client_http/client_http_router_impl.h"
#include "pathfinder/client_http/client_http_server.h"
#include "pathfinder/client_runtime_host/client_runtime_host_factory.h"
#include <atomic>
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
    std::string static_root = "app/frontend/client";

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

    pathfinder::client_runtime_host::ClientRuntimeHostFactory runtime;
    pathfinder::client_http::ClientStaticFileService static_files(static_root);
    pathfinder::client_http::ClientHttpServer server(
        std::make_unique<pathfinder::client_http::ClientSocketHttpTransport>(),
        std::make_unique<pathfinder::client_http::ClientHttpRouter>(static_files, *runtime.http_gateway));

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    auto result = server.start(port, host);
    if (result.is_error()) {
        std::cerr << "Failed to start Pathfinder Client Server\n";
        for (const auto& error : result.errors()) {
            std::cerr << " - " << error.message_key;
            if (error.debug_message.has_value()) {
                std::cerr << ": " << error.debug_message.value();
            }
            std::cerr << "\n";
        }
        return 1;
    }

    std::cout << "Pathfinder Client Server running on http://" << host << ":" << port << "\n";
    std::cout << "Static root: " << static_root << "\n";
    std::cout << "API: /api/client/bootstrap /command /refresh /reset\n";

    while (server.isRunning() && !g_stop_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (server.isRunning()) {
        server.stop();
    }
    return 0;
}
