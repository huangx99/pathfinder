#pragma once

#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include <memory>
#include <string>

namespace pathfinder::client_http {

// Creates the minimal runnable backend dependencies for P54 client HTTP server.
// Wires P53 Client Protocol with WorldCommand pipeline.
struct ClientServerRuntimeFactory {
    pathfinder::client_protocol::ClientProjectionAdapter projection_adapter;
    pathfinder::client_protocol::ClientPatchContract patch_contract;
    pathfinder::client_protocol::ClientAvailableCommandAdapter available_command_adapter;
    pathfinder::client_protocol::ClientSessionGateway session_gateway;
    pathfinder::world_command::WorldCommandHandlerRegistry registry;
    pathfinder::world_command::WorldCommandDispatcher dispatcher;
    pathfinder::world_command::WorldCommandPipeline pipeline;
    pathfinder::client_protocol::ClientCommandGateway command_gateway;
    pathfinder::client_protocol::ClientProtocolCodec codec;
    std::unique_ptr<pathfinder::client_http::ClientHttpGateway> http_gateway;

    ClientServerRuntimeFactory();
};

} // namespace pathfinder::client_http
