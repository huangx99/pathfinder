#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include <string>

namespace pathfinder::client_protocol {

// Fake client harness for automated protocol testing.
// Simulates a client without any real UI, browser, or CSS.
class ClientProtocolHarness {
public:
    ClientProtocolHarness(
        ClientSessionGateway& session_gateway,
        ClientCommandGateway& command_gateway,
        const ClientProtocolCodec& codec);

    // Simulate bootstrap.
    pathfinder::foundation::Result<ClientBootstrapResponse> fakeBootstrap(
        const std::string& client_id,
        const std::string& session_id,
        const std::string& actor_key = "player");

    // Simulate submitting an option_id.
    pathfinder::foundation::Result<ClientCommandResponse> fakeSubmitOption(
        const std::string& session_id,
        const std::string& client_id,
        uint64_t client_sequence,
        uint64_t known_projection_version,
        const std::string& option_id);

    // Simulate submitting a WorldCommandDto.
    pathfinder::foundation::Result<ClientCommandResponse> fakeSubmitCommand(
        const std::string& session_id,
        const std::string& client_id,
        uint64_t client_sequence,
        uint64_t known_projection_version,
        const WorldCommandDto& command);

    // Simulate refresh.
    pathfinder::foundation::Result<ClientRefreshResponse> fakeRefresh(
        const std::string& session_id,
        const std::string& client_id,
        uint64_t known_projection_version);

    // Simulate reset.
    pathfinder::foundation::Result<ClientResetResponse> fakeReset(
        const std::string& session_id,
        const std::string& client_id);

    // Codec round-trip helpers for testing wire format.
    pathfinder::foundation::Result<ClientBootstrapResponse> roundtripBootstrap(
        const ClientBootstrapRequest& request);

    pathfinder::foundation::Result<ClientCommandResponse> roundtripCommand(
        const ClientCommandRequest& request);

private:
    ClientSessionGateway& session_gateway_;
    ClientCommandGateway& command_gateway_;
    const ClientProtocolCodec& codec_;
};

} // namespace pathfinder::client_protocol
