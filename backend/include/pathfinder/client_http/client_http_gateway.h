#pragma once

#include "pathfinder/client_http/client_http_types.h"
#include "pathfinder/foundation/result.h"
#include <functional>
#include <string>

namespace pathfinder::client_protocol {
class ClientProtocolCodec;
class ClientSessionGateway;
class ClientCommandGateway;
} // namespace pathfinder::client_protocol

namespace pathfinder::world_map_interaction {
class ClientMapSelectionService;
}

namespace pathfinder::client_http {

class ClientHttpGateway {
public:
    using ResetWorldCallback = std::function<pathfinder::foundation::Result<void>()>;

    ClientHttpGateway(
        pathfinder::client_protocol::ClientProtocolCodec& codec,
        pathfinder::client_protocol::ClientSessionGateway& session_gateway,
        pathfinder::client_protocol::ClientCommandGateway& command_gateway,
        pathfinder::world_map_interaction::ClientMapSelectionService* selection_service = nullptr,
        ResetWorldCallback reset_world_callback = {});

    ClientHttpResponse handleApi(const ClientHttpRequest& request);

    void setMapSelectionService(pathfinder::world_map_interaction::ClientMapSelectionService* service);

private:
    pathfinder::client_protocol::ClientProtocolCodec& codec_;
    pathfinder::client_protocol::ClientSessionGateway& session_gateway_;
    pathfinder::client_protocol::ClientCommandGateway& command_gateway_;
    pathfinder::world_map_interaction::ClientMapSelectionService* selection_service_ = nullptr;
    ResetWorldCallback reset_world_callback_;

    ClientHttpResponse handleBootstrap(const std::string& body);
    ClientHttpResponse handleCommand(const std::string& body);
    ClientHttpResponse handleRefresh(const std::string& body);
    ClientHttpResponse handleReset(const std::string& body);
    ClientHttpResponse handleSelect(const std::string& body);

    static ClientHttpResponse makeJsonError(
        int status_code,
        const std::string& error_key,
        const std::string& message);
    static ClientHttpResponse makeJsonError(
        int status_code,
        const std::string& error_key,
        const std::string& message,
        const std::vector<std::string>& details);
    static ClientHttpResponse makeJsonResponse(int status_code, const std::string& json_body);
};

} // namespace pathfinder::client_http
