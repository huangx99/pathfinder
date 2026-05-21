#pragma once

#include "pathfinder/client_http/client_http_types.h"
#include "pathfinder/client_http/client_http_router_impl.h"
#include "pathfinder/client_http/client_http_transport.h"
#include "pathfinder/foundation/result.h"
#include <memory>
#include <string>

namespace pathfinder::client_http {

class ClientHttpServer {
public:
    ClientHttpServer(
        std::unique_ptr<IClientHttpTransport> transport,
        std::unique_ptr<ClientHttpRouter> router);

    pathfinder::foundation::Result<void> start(
        uint16_t port,
        const std::string& host);

    void stop();
    bool isRunning() const;

private:
    std::unique_ptr<IClientHttpTransport> transport_;
    std::unique_ptr<ClientHttpRouter> router_;
};

} // namespace pathfinder::client_http
