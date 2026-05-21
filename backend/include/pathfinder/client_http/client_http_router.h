#pragma once

#include "pathfinder/client_http/client_http_types.h"

namespace pathfinder::client_http {

class IClientHttpRouter {
public:
    virtual ~IClientHttpRouter() = default;

    virtual ClientHttpResponse route(
        const ClientHttpRequest& request) = 0;
};

} // namespace pathfinder::client_http
