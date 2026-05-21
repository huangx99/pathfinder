#include "pathfinder/client_http/client_http_server.h"

namespace pathfinder::client_http {

using pathfinder::foundation::Result;

ClientHttpServer::ClientHttpServer(
    std::unique_ptr<IClientHttpTransport> transport,
    std::unique_ptr<ClientHttpRouter> router)
    : transport_(std::move(transport))
    , router_(std::move(router)) {
}

Result<void> ClientHttpServer::start(uint16_t port, const std::string& host) {
    if (!transport_ || !router_) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::common_internal_invariant_broken,
                "client_http.server_not_initialized"));
    }
    return transport_->start(port, host, *router_);
}

void ClientHttpServer::stop() {
    if (transport_) {
        transport_->stop();
    }
}

bool ClientHttpServer::isRunning() const {
    if (!transport_) return false;
    return transport_->isRunning();
}

} // namespace pathfinder::client_http
