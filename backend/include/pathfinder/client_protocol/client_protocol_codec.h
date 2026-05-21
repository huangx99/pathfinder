#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include <string>

namespace pathfinder::client_protocol {

class ClientProtocolCodec {
public:
    // Bootstrap
    pathfinder::foundation::Result<std::string> encodeBootstrapRequest(
        const ClientBootstrapRequest& request) const;
    pathfinder::foundation::Result<ClientBootstrapRequest> decodeBootstrapRequest(
        const std::string& json) const;
    pathfinder::foundation::Result<std::string> encodeBootstrapResponse(
        const ClientBootstrapResponse& response) const;
    pathfinder::foundation::Result<ClientBootstrapResponse> decodeBootstrapResponse(
        const std::string& json) const;

    // Command
    pathfinder::foundation::Result<std::string> encodeCommandRequest(
        const ClientCommandRequest& request) const;
    pathfinder::foundation::Result<ClientCommandRequest> decodeCommandRequest(
        const std::string& json) const;
    pathfinder::foundation::Result<std::string> encodeCommandResponse(
        const ClientCommandResponse& response) const;
    pathfinder::foundation::Result<ClientCommandResponse> decodeCommandResponse(
        const std::string& json) const;

    // Refresh
    pathfinder::foundation::Result<std::string> encodeRefreshRequest(
        const ClientRefreshRequest& request) const;
    pathfinder::foundation::Result<ClientRefreshRequest> decodeRefreshRequest(
        const std::string& json) const;
    pathfinder::foundation::Result<std::string> encodeRefreshResponse(
        const ClientRefreshResponse& response) const;
    pathfinder::foundation::Result<ClientRefreshResponse> decodeRefreshResponse(
        const std::string& json) const;

    // Reset
    pathfinder::foundation::Result<std::string> encodeResetRequest(
        const ClientResetRequest& request) const;
    pathfinder::foundation::Result<ClientResetRequest> decodeResetRequest(
        const std::string& json) const;
    pathfinder::foundation::Result<std::string> encodeResetResponse(
        const ClientResetResponse& response) const;
    pathfinder::foundation::Result<ClientResetResponse> decodeResetResponse(
        const std::string& json) const;
};

} // namespace pathfinder::client_protocol
