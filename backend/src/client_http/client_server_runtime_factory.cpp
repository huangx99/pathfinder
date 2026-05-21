#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/world_command/world_command_handlers.h"

namespace pathfinder::client_http {

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;

ClientServerRuntimeFactory::ClientServerRuntimeFactory()
    : session_gateway(projection_adapter, available_command_adapter)
    , dispatcher(registry)
    , pipeline(dispatcher)
    , command_gateway(session_gateway, pipeline, patch_contract, available_command_adapter) {
    // Register minimal command handlers for client protocol.
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createWaitCommandHandler());
    registry.registerHandler(createInspectCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());
    registry.registerHandler(createGenerateWorldCommandHandler());

    http_gateway = std::make_unique<ClientHttpGateway>(codec, session_gateway, command_gateway);
}

} // namespace pathfinder::client_http
