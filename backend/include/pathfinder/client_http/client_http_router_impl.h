#pragma once

#include "pathfinder/client_http/client_http_router.h"

namespace pathfinder::client_http {

class ClientStaticFileService;
class ClientHttpGateway;

class ClientHttpRouter final : public IClientHttpRouter {
public:
    ClientHttpRouter(
        ClientStaticFileService& static_files,
        ClientHttpGateway& gateway);

    ClientHttpResponse route(
        const ClientHttpRequest& request) override;

private:
    ClientStaticFileService& static_files_;
    ClientHttpGateway& gateway_;
};

} // namespace pathfinder::client_http
