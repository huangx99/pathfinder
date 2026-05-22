#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_patch_contract.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/world_map_interaction/region_activity_window_service.h"
#include "pathfinder/world_map_interaction/region_lifecycle_trigger_service.h"
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

    void setRegionEnsureAdapter(client_runtime_bridge::ClientWorldRegionEnsureAdapter* ensure_adapter);
    void setActivityWindowService(world_map_interaction::RegionActivityWindowService* service);
    void setLifecycleTriggerService(world_map_interaction::RegionLifecycleTriggerService* service);
    void setWorldContext(const std::string& world_id, uint64_t world_seed);
    bool hasActivityWindowService() const { return activity_window_service_ != nullptr; }

    pathfinder::foundation::Result<ClientCommandResponse> handleCommand(
        const ClientCommandRequest& request);

private:
    ClientSessionGateway& session_gateway_;
    pathfinder::world_command::WorldCommandPipeline& pipeline_;
    const ClientPatchContract& patch_contract_;
    const ClientAvailableCommandAdapter& available_command_adapter_;
    client_runtime_bridge::ClientWorldRegionEnsureAdapter* ensure_adapter_ = nullptr;
    world_map_interaction::RegionActivityWindowService* activity_window_service_ = nullptr;
    world_map_interaction::RegionLifecycleTriggerService* lifecycle_trigger_service_ = nullptr;
    std::string world_id_ = "world_default";
    uint64_t world_seed_ = 1;

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
