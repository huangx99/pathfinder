#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::client_protocol {

// Receives client command requests, validates them, materializes options,
// executes via WorldCommandPipeline, and returns safe responses.
class ClientCommandGateway {
public:
    ClientCommandGateway(
        ClientSessionGateway& session_gateway,
        pathfinder::world_command::WorldCommandPipeline& pipeline,
        const ClientPatchContract& patch_contract,
        const ClientAvailableCommandAdapter& available_command_adapter);

    pathfinder::foundation::Result<ClientCommandResponse> handleCommand(
        const ClientCommandRequest& request);

private:
    ClientSessionGateway& session_gateway_;
    pathfinder::world_command::WorldCommandPipeline& pipeline_;
    const ClientPatchContract& patch_contract_;
    const ClientAvailableCommandAdapter& available_command_adapter_;

    // Resolve command from OptionId or WorldCommandDto.
    // Uses session snapshot for OptionId authority.
    pathfinder::foundation::Result<WorldCommandDto> resolveCommand(
        const ClientCommandRequest& request,
        const std::string& actor_key);

    // Validate a client-submitted WorldCommandDto against session policy.
    // Also verifies command_kind/command_key/target match an option in the latest snapshot.
    pathfinder::foundation::Result<void> validateClientCommandDto(
        const WorldCommandDto& command,
        const std::string& session_actor_key,
        const std::vector<WorldCommandOptionDto>& snapshot) const;

    ClientCommandResponse buildBlockedResponse(
        const std::string& session_id,
        uint64_t client_sequence,
        uint64_t projection_version,
        const std::string& reason_key,
        bool requires_full_refresh = false,
        const std::string& actor_key = "");
};

} // namespace pathfinder::client_protocol
