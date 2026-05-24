#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::client_runtime_host {
struct ClientRuntimeHostFactory;
}

namespace pf::client {

struct LocalCellView {
    std::string cell_id;
    int x{0};
    int y{0};
    std::string layer_key{"surface"};
    std::string terrain_key{"unknown"};
    bool blocks_movement{false};
    std::string visibility;
};

struct LocalEntityView {
    std::string entity_id;
    std::string entity_key;
    std::string display_name_key;
    std::string actor_key;
    int x{0};
    int y{0};
    std::string layer_key{"surface"};
    bool is_resource_node{false};
    int health{0};
    int max_health{0};
    bool alive{true};
};

struct LocalInventoryItemView {
    std::string entry_id;
    std::string entity_id;
    std::string entity_key;
    std::string stack_key;
    std::string display_name;
    int quantity{1};
    std::map<std::string, double> numeric_states;
};


struct LocalKnowledgeView {
    std::string knowledge_id;
    std::string subject_id;
    std::string action_key;
    std::string effect_key;
    std::string status;
    std::string confidence;
    std::string subject_name;
    std::string action_name;
    std::string effect_name;
    std::string summary_text;
    std::string recipe_text;
    std::string actor_key;
};

struct LocalCommandOptionView {
    std::string option_id;
    std::string command_key;
    std::string label_text;
    pathfinder::world_command::WorldCommandKind command_kind{pathfinder::world_command::WorldCommandKind::Unknown};
    bool enabled{false};
    std::optional<int> target_x;
    std::optional<int> target_y;
    std::string target_entity_id;
    std::string target_actor_key;
    std::string knowledge_key;
};

struct LocalClientSnapshot {
    uint64_t projection_version{0};
    std::string status_text;
    std::vector<LocalCellView> cells;
    std::vector<LocalEntityView> entities;
    std::vector<LocalCommandOptionView> options;
    std::vector<LocalInventoryItemView> inventory_items;
    std::map<std::string, std::vector<LocalInventoryItemView>> actor_inventory_items;
    std::map<std::string, int> actor_inventory_capacity_slots;
    std::vector<LocalKnowledgeView> knowledge_items;
    int inventory_capacity_slots{9};
    std::map<std::string, std::string> actor_work_status;
    std::vector<std::string> events;
};

class LocalClientRuntime final {
public:
    LocalClientRuntime();
    ~LocalClientRuntime();

    bool bootstrap();
    bool refresh();
    bool reset();
    bool submitOption(const std::string& option_id);

    std::optional<LocalCommandOptionView> findMoveOptionTo(int x, int y) const;
    std::optional<LocalCommandOptionView> findInspectOptionFor(int x, int y) const;
    std::optional<LocalCommandOptionView> findBestOptionForEntityAt(int x, int y) const;

    const LocalClientSnapshot& snapshot() const { return snapshot_; }
    const std::string& lastError() const { return last_error_; }

private:
    void applyBootstrap(const pathfinder::client_protocol::ClientBootstrapResponse& response);
    void applyRefresh(const pathfinder::client_protocol::ClientRefreshResponse& response);
    void applyCommand(const pathfinder::client_protocol::ClientCommandResponse& response);
    void applyProjectionPatch(const pathfinder::world_command::WorldProjectionPatchDto& patch);
    void applyProjection(
        uint64_t projection_version,
        const pathfinder::client_protocol::ClientWorldProjection& projection,
        const std::vector<pathfinder::world_command::WorldCommandOptionDto>& options,
        const std::vector<pathfinder::world_command::WorldEventDto>& events);
    void localizeInventoryItems(std::vector<LocalInventoryItemView>& items) const;
    std::string translateKey(const std::string& key) const;
    std::string translateInlineText(const std::string& text) const;
    std::string displayNameForEntityKey(const std::string& entity_key, const std::string& fallback) const;

    static std::string firstErrorText(const std::vector<pathfinder::foundation::ErrorDetail>& errors);
    static int parseIntField(const std::map<std::string, std::string>& fields, const std::string& key, int fallback);
    static bool parseBoolField(const std::map<std::string, std::string>& fields, const std::string& key, bool fallback);
    static std::string stringField(const std::map<std::string, std::string>& fields, const std::string& key, const std::string& fallback = "");

    std::unique_ptr<pathfinder::client_runtime_host::ClientRuntimeHostFactory> factory_;
    LocalClientSnapshot snapshot_;
    std::string session_id_{"axmol_local_session"};
    std::string client_id_{"axmol_local_client"};
    uint64_t client_sequence_{0};
    std::string last_error_;
};

} // namespace pf::client
