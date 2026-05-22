#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/client_protocol/client_available_command_adapter.h"
#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/foundation/result.h"
#include <map>
#include <string>
#include <vector>

namespace pathfinder::client_protocol {

// Manages client sessions: bootstrap, refresh, reset.
// Does not directly modify runtime arrays.
class ClientSessionGateway {
public:
    ClientSessionGateway(
        const ClientProjectionAdapter& projection_adapter,
        const ClientAvailableCommandAdapter& available_command_adapter);

    void setRegionEnsureAdapter(client_runtime_bridge::ClientWorldRegionEnsureAdapter* ensure_adapter);

    pathfinder::foundation::Result<ClientBootstrapResponse> bootstrap(
        const ClientBootstrapRequest& request);

    pathfinder::foundation::Result<ClientRefreshResponse> refresh(
        const ClientRefreshRequest& request);

    pathfinder::foundation::Result<ClientResetResponse> reset(
        const ClientResetRequest& request);

    // Get current projection version for a session.
    pathfinder::foundation::Result<uint64_t> getProjectionVersion(
        const std::string& session_id) const;

    // Get session actor key (authority, not from client selection_context).
    pathfinder::foundation::Result<std::string> getSessionActorKey(
        const std::string& session_id) const;

    // Get session client id.
    pathfinder::foundation::Result<std::string> getSessionClientId(
        const std::string& session_id) const;

    // Get session layer key.
    pathfinder::foundation::Result<std::string> getSessionLayerKey(
        const std::string& session_id) const;

    // Check if a session exists.
    bool hasSession(const std::string& session_id) const;

    // Update projection version for a session.
    pathfinder::foundation::Result<void> updateProjectionVersion(
        const std::string& session_id, uint64_t new_version);

    // Get the last available commands snapshot for a session.
    pathfinder::foundation::Result<std::vector<WorldCommandOptionDto>> getAvailableCommands(
        const std::string& session_id) const;

    // Check if an option_id is valid in the current session snapshot.
    bool isOptionValid(
        const std::string& session_id, const std::string& option_id) const;

    // Update the authoritative option snapshot for a session.
    pathfinder::foundation::Result<void> updateAvailableCommands(
        const std::string& session_id,
        const std::vector<WorldCommandOptionDto>& options);

private:
    struct SessionState {
        std::string session_id;
        std::string client_id;
        std::string actor_key;
        std::string layer_key;
        uint64_t projection_version = 1;
        std::vector<WorldCommandOptionDto> last_available_commands;
    };

    const ClientProjectionAdapter& projection_adapter_;
    const ClientAvailableCommandAdapter& available_command_adapter_;
    client_runtime_bridge::ClientWorldRegionEnsureAdapter* ensure_adapter_ = nullptr;
    std::map<std::string, SessionState> sessions_;

    SessionState& getOrCreateSession(const ClientBootstrapRequest& request);
};

} // namespace pathfinder::client_protocol
