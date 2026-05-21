#pragma once

#include "pathfinder/foundation/result.h"
#include <cstdint>
#include <string>

namespace pathfinder::client_http {

class IClientHttpRouter;

class IClientHttpTransport {
public:
    virtual ~IClientHttpTransport() = default;

    virtual pathfinder::foundation::Result<void> start(
        uint16_t port,
        const std::string& host,
        IClientHttpRouter& router) = 0;

    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
};

} // namespace pathfinder::client_http
