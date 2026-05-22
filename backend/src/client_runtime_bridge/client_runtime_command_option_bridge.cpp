#include "pathfinder/client_runtime_bridge/client_runtime_command_option_bridge.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/foundation/error.h"
#include <atomic>
#include <chrono>
#include <set>

namespace pathfinder::client_runtime_bridge {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_runtime::IWorldRuntime;
using pathfinder::world_runtime::WorldActorRuntime;
using pathfinder::world_runtime::WorldCellCoord;
using pathfinder::world_command::WorldCommandHandlerRegistry;
using pathfinder::world_command::WorldCommandOptionDto;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandTargetDto;
using pathfinder::world_command::WorldCommandTargetKind;

namespace {
    std::atomic<uint64_t> g_option_counter{0};
    std::string makeOptionId(const std::string& prefix) {
        uint64_t n = ++g_option_counter;
        uint64_t ts = static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        return prefix + "_" + std::to_string(ts) + "_" + std::to_string(n);
    }
}

// ---------------------------------------------------------------------------
// MovementCommandOptionProvider
// ---------------------------------------------------------------------------

MovementCommandOptionProvider::MovementCommandOptionProvider(
    const WorldCommandHandlerRegistry& registry)
    : registry_(registry) {}

std::vector<WorldCommandOptionDto> MovementCommandOptionProvider::buildOptions(
    const WorldActorRuntime& actor,
    const IWorldRuntime& runtime) const {

    std::vector<WorldCommandOptionDto> options;

    // No Move handler registered -> do not generate Move options
    if (!registry_.findByKind(WorldCommandKind::Move)) {
        return options;
    }

    const std::vector<ClientMoveDirection> directions = {
        ClientMoveDirection::North,
        ClientMoveDirection::South,
        ClientMoveDirection::West,
        ClientMoveDirection::East
    };

    for (auto dir : directions) {
        auto opt = buildMoveOption(actor, dir, runtime);
        options.push_back(std::move(opt));
    }

    return options;
}

WorldCommandOptionDto MovementCommandOptionProvider::buildMoveOption(
    const WorldActorRuntime& actor,
    ClientMoveDirection direction,
    const IWorldRuntime& runtime) const {

    WorldCommandOptionDto opt;
    opt.command_kind = WorldCommandKind::Move;
    opt.option_id = makeOptionId("opt_move");

    int dx = dxForDirection(direction);
    int dy = dyForDirection(direction);
    WorldCellCoord target{actor.coord.x + dx, actor.coord.y + dy, actor.coord.layer_key};

    switch (direction) {
        case ClientMoveDirection::North: opt.command_key = "move_north"; opt.label_text = "向上"; break;
        case ClientMoveDirection::South: opt.command_key = "move_south"; opt.label_text = "向下"; break;
        case ClientMoveDirection::West:  opt.command_key = "move_west";  opt.label_text = "向左"; break;
        case ClientMoveDirection::East:  opt.command_key = "move_east";  opt.label_text = "向右"; break;
        default:                         opt.command_key = "move_unknown"; opt.label_text = "移动"; break;
    }

    opt.target.target_kind = WorldCommandTargetKind::Coordinate;
    opt.target.target_coord = pathfinder::world_command::WorldCoordinateDto{target.x, target.y, target.layer_key};
    opt.priority = 10;

    // Check if move is currently possible
    auto cell_res = runtime.findCell(target);
    if (cell_res.is_error()) {
        opt.enabled = false;
        opt.disabled_reason_text = "out_of_bounds";
    } else {
        const auto* cell = cell_res.value();
        if (cell->blocks_movement) {
            opt.enabled = false;
            opt.disabled_reason_text = "cell_blocked";
        } else if (target.layer_key != actor.coord.layer_key) {
            opt.enabled = false;
            opt.disabled_reason_text = "unknown_layer";
        } else {
            opt.enabled = true;
        }
    }

    return opt;
}

// ---------------------------------------------------------------------------
// InspectCommandOptionProvider
// ---------------------------------------------------------------------------

InspectCommandOptionProvider::InspectCommandOptionProvider(
    const WorldCommandHandlerRegistry& registry)
    : registry_(registry) {}

std::vector<WorldCommandOptionDto> InspectCommandOptionProvider::buildOptions(
    const WorldActorRuntime& actor,
    const IWorldRuntime& runtime) const {

    std::vector<WorldCommandOptionDto> options;

    if (!registry_.findByKind(WorldCommandKind::Inspect)) {
        return options;
    }

    // Inspect current cell
    {
        WorldCommandOptionDto opt;
        opt.option_id = makeOptionId("opt_inspect_cell");
        opt.command_kind = WorldCommandKind::Inspect;
        opt.command_key = "inspect_current_cell";
        opt.label_text = "观察当前格子";
        opt.target.target_kind = WorldCommandTargetKind::Coordinate;
        opt.target.target_coord = pathfinder::world_command::WorldCoordinateDto{
            actor.coord.x, actor.coord.y, actor.coord.layer_key};
        opt.enabled = true;
        opt.priority = 5;
        options.push_back(std::move(opt));
    }

    // Inspect visible adjacent cells
    const std::vector<std::pair<int, int>> deltas = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    const std::vector<std::string> dir_labels = {"观察北边", "观察南边", "观察西边", "观察东边"};
    const std::vector<std::string> dir_keys = {"inspect_adjacent_north", "inspect_adjacent_south", "inspect_adjacent_west", "inspect_adjacent_east"};

    for (size_t i = 0; i < deltas.size(); ++i) {
        WorldCellCoord adj{actor.coord.x + deltas[i].first, actor.coord.y + deltas[i].second, actor.coord.layer_key};
        auto cell_res = runtime.findCell(adj);
        if (cell_res.is_error()) continue;

        WorldCommandOptionDto opt;
        opt.option_id = makeOptionId("opt_inspect_adj");
        opt.command_kind = WorldCommandKind::Inspect;
        opt.command_key = dir_keys[i];
        opt.label_text = dir_labels[i];
        opt.target.target_kind = WorldCommandTargetKind::Coordinate;
        opt.target.target_coord = pathfinder::world_command::WorldCoordinateDto{adj.x, adj.y, adj.layer_key};
        opt.enabled = true;
        opt.priority = 5;
        options.push_back(std::move(opt));
    }

    // Inspect visible entities
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_ok()) {
        const auto& snapshot = snap_res.value();
        for (const auto& [entity_id, entity] : snapshot.entities) {
            if (!entity.coord) continue;
            if (entity.coord->layer_key != actor.coord.layer_key) continue;
            // Only entities on visible cells
            const auto* grid = dynamic_cast<const pathfinder::world_runtime::WorldGridRuntime*>(&runtime);
            if (grid) {
                auto vis = grid->getCellVisibility(actor.actor_key, entity.coord->cellId());
                if (vis != pathfinder::world_runtime::WorldCellVisibility::Visible) continue;
            }
            // Skip player itself
            if (entity.entity_key == "player") continue;

            WorldCommandOptionDto opt;
            opt.option_id = makeOptionId("opt_inspect_entity");
            opt.command_kind = WorldCommandKind::Inspect;
            opt.command_key = "inspect_entity_" + entity_id;
            opt.label_text = "观察 " + entity.display_name_key;
            opt.target.target_kind = WorldCommandTargetKind::Entity;
            opt.target.target_entity_id = entity_id;
            opt.enabled = true;
            opt.priority = 6;
            options.push_back(std::move(opt));
        }
    }

    return options;
}

// ---------------------------------------------------------------------------
// WaitCommandOptionProvider
// ---------------------------------------------------------------------------

WaitCommandOptionProvider::WaitCommandOptionProvider(
    const WorldCommandHandlerRegistry& registry)
    : registry_(registry) {}

std::vector<WorldCommandOptionDto> WaitCommandOptionProvider::buildOptions(
    const WorldActorRuntime& /*actor*/) const {

    std::vector<WorldCommandOptionDto> options;

    if (!registry_.findByKind(WorldCommandKind::Wait)) {
        return options;
    }

    WorldCommandOptionDto opt;
    opt.option_id = makeOptionId("opt_wait");
    opt.command_kind = WorldCommandKind::Wait;
    opt.command_key = "wait";
    opt.label_text = "等待";
    opt.target.target_kind = WorldCommandTargetKind::None;
    opt.enabled = true;
    opt.priority = 0;
    options.push_back(std::move(opt));

    return options;
}

// ---------------------------------------------------------------------------
// ClientRuntimeCommandOptionBridge
// ---------------------------------------------------------------------------

ClientRuntimeCommandOptionBridge::ClientRuntimeCommandOptionBridge(
    IWorldRuntime& runtime,
    const WorldCommandHandlerRegistry& registry,
    ClientRuntimeBridgeMode mode)
    : runtime_(runtime)
    , registry_(registry)
    , mode_(mode)
    , movement_provider_(registry)
    , inspect_provider_(registry)
    , wait_provider_(registry)
    , harvest_provider_(nullptr)
    , inventory_provider_(nullptr) {}

ClientRuntimeCommandOptionBridge::ClientRuntimeCommandOptionBridge(
    IWorldRuntime& runtime,
    const WorldCommandHandlerRegistry& registry,
    pathfinder::world_harvest::ResourceHarvestService* harvest_service,
    pathfinder::world_inventory::IInventoryRuntime* inventory_runtime,
    ClientRuntimeBridgeMode mode)
    : runtime_(runtime)
    , registry_(registry)
    , mode_(mode)
    , movement_provider_(registry)
    , inspect_provider_(registry)
    , wait_provider_(registry) {
    if (harvest_service) {
        harvest_provider_ = std::make_unique<HarvestCommandOptionProvider>(registry, *harvest_service);
    }
    if (inventory_runtime) {
        inventory_provider_ = std::make_unique<InventoryCommandOptionProvider>(registry, *inventory_runtime);
    }
}

Result<ClientRuntimeView> ClientRuntimeCommandOptionBridge::buildRuntimeView(
    const ClientRuntimeViewRequest& /*request*/) const {
    // Command option bridge does not build projection views.
    return Result<ClientRuntimeView>::fail(
        makeError(ErrorCode::common_not_implemented, "command_option_bridge_does_not_build_views"));
}

Result<std::vector<WorldCommandOptionDto>> ClientRuntimeCommandOptionBridge::buildRuntimeOptions(
    const ClientRuntimeCommandOptionRequest& request) const {

    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return Result<std::vector<WorldCommandOptionDto>>::fail(valid.errors());
    }

    if (mode_ != ClientRuntimeBridgeMode::RuntimeBacked) {
        return Result<std::vector<WorldCommandOptionDto>>::fail(
            makeError(ErrorCode::common_not_implemented, "bridge_mode_invalid", "Option bridge not in RuntimeBacked mode"));
    }

    auto actor_res = runtime_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<std::vector<WorldCommandOptionDto>>::fail(
            makeError(ErrorCode::id_not_found, "actor_not_found", "Actor not found: " + request.actor_key));
    }

    const auto* actor = actor_res.value();
    std::vector<WorldCommandOptionDto> options;

    // Determine which providers to run
    bool run_wait = true;
    bool move_move = true;
    bool run_inspect = true;
    bool run_harvest = true;
    bool run_inventory = true;

    if (!request.provider_kinds.empty()) {
        run_wait = false;
        move_move = false;
        run_inspect = false;
        run_harvest = false;
        run_inventory = false;
        for (const auto& kind : request.provider_kinds) {
            if (kind == ClientCommandOptionProviderKind::Wait) run_wait = true;
            if (kind == ClientCommandOptionProviderKind::Move) move_move = true;
            if (kind == ClientCommandOptionProviderKind::Inspect) run_inspect = true;
            if (kind == ClientCommandOptionProviderKind::Harvest) run_harvest = true;
            if (kind == ClientCommandOptionProviderKind::Inventory) run_inventory = true;
        }
    }

    if (run_wait) {
        auto wait_opts = wait_provider_.buildOptions(*actor);
        options.insert(options.end(), wait_opts.begin(), wait_opts.end());
    }

    if (move_move) {
        auto move_opts = movement_provider_.buildOptions(*actor, runtime_);
        options.insert(options.end(), move_opts.begin(), move_opts.end());
    }

    if (run_inspect) {
        auto inspect_opts = inspect_provider_.buildOptions(*actor, runtime_);
        options.insert(options.end(), inspect_opts.begin(), inspect_opts.end());
    }

    if (run_harvest && harvest_provider_) {
        auto harvest_opts = harvest_provider_->buildOptions(*actor, runtime_);
        options.insert(options.end(), harvest_opts.begin(), harvest_opts.end());
    }

    if (run_inventory && inventory_provider_) {
        auto inv_opts = inventory_provider_->buildOptions(*actor, runtime_);
        options.insert(options.end(), inv_opts.begin(), inv_opts.end());
    }

    return Result<std::vector<WorldCommandOptionDto>>::ok(std::move(options));
}

Result<ClientRuntimeBridgeDiagnostics> ClientRuntimeCommandOptionBridge::diagnostics(
    const std::string& actor_key,
    const std::string& layer_key) const {

    ClientRuntimeBridgeDiagnostics diag;
    diag.bridge_mode = mode_;
    diag.runtime_state_version = runtime_.stateVersion();

    ClientRuntimeCommandOptionRequest req;
    req.actor_key = actor_key;
    req.layer_key = layer_key;
    req.projection_version = runtime_.stateVersion();

    auto opts_res = buildRuntimeOptions(req);
    if (opts_res.is_ok()) {
        diag.option_count = static_cast<int>(opts_res.value().size());
    }

    return Result<ClientRuntimeBridgeDiagnostics>::ok(std::move(diag));
}

// ---------------------------------------------------------------------------
// HarvestCommandOptionProvider
// ---------------------------------------------------------------------------

HarvestCommandOptionProvider::HarvestCommandOptionProvider(
    const WorldCommandHandlerRegistry& registry,
    pathfinder::world_harvest::ResourceHarvestService& harvest_service)
    : registry_(registry), harvest_service_(harvest_service) {}

std::vector<WorldCommandOptionDto> HarvestCommandOptionProvider::buildOptions(
    const WorldActorRuntime& actor,
    const IWorldRuntime& runtime) const {

    std::vector<WorldCommandOptionDto> options;

    // Check if any harvest handler is registered
    bool has_gather = registry_.findByKind(WorldCommandKind::Gather) != nullptr;
    bool has_chop   = registry_.findByKind(WorldCommandKind::Chop) != nullptr;
    bool has_mine   = registry_.findByKind(WorldCommandKind::Mine) != nullptr;
    bool has_dig    = registry_.findByKind(WorldCommandKind::Dig) != nullptr;
    if (!has_gather && !has_chop && !has_mine && !has_dig) {
        return options;
    }

    // Find resource nodes adjacent to or on the actor's cell
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) {
        return options;
    }
    const auto& snapshot = snap_res.value();

    for (const auto& [node_id, node] : snapshot.resource_nodes) {
        if (node.coord.layer_key != actor.coord.layer_key) continue;

        int dx = std::abs(node.coord.x - actor.coord.x);
        int dy = std::abs(node.coord.y - actor.coord.y);
        if (dx > 1 || dy > 1) continue; // must be adjacent or same cell
        if (node.node_state_str == "Depleted") continue;
        if (node.remaining_charges <= 0) continue;

        // Map required_action_key to harvest command
        WorldCommandKind command_kind = WorldCommandKind::Noop;
        std::string command_key;
        std::string label_text;
        pathfinder::world_harvest::ResourceHarvestKind harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Unknown;

        if (node.required_action_key == "gather" && has_gather) {
            command_kind = WorldCommandKind::Gather;
            command_key = "gather";
            label_text = "采集";
            harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Gather;
        } else if (node.required_action_key == "chop" && has_chop) {
            command_kind = WorldCommandKind::Chop;
            command_key = "chop";
            label_text = "砍伐";
            harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Chop;
        } else if (node.required_action_key == "mine" && has_mine) {
            command_kind = WorldCommandKind::Mine;
            command_key = "mine";
            label_text = "开采";
            harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Mine;
        } else if (node.required_action_key == "dig" && has_dig) {
            command_kind = WorldCommandKind::Dig;
            command_key = "dig";
            label_text = "挖掘";
            harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Dig;
        } else {
            continue; // unsupported required_action_key or handler not registered
        }

        // Validate via harvest service (read-only check)
        pathfinder::world_harvest::ResourceHarvestRequest req;
        req.harvest_id = "option_check_" + node_id;
        req.actor_key = actor.actor_key;
        req.node_id = node_id;
        req.harvest_kind = harvest_kind;

        auto validate_res = harvest_service_.validate(req);
        if (!validate_res.is_ok()) {
            continue; // not harvestable right now
        }

        WorldCommandOptionDto opt;
        opt.option_id = makeOptionId("opt_harvest");
        opt.command_kind = command_kind;
        opt.command_key = command_key;
        opt.label_text = label_text;
        opt.target.target_kind = WorldCommandTargetKind::Entity;
        opt.target.target_entity_id = node_id;
        opt.enabled = true;
        opt.priority = 20;

        options.push_back(std::move(opt));
    }

    return options;
}

// ---------------------------------------------------------------------------
// InventoryCommandOptionProvider
// ---------------------------------------------------------------------------

InventoryCommandOptionProvider::InventoryCommandOptionProvider(
    const WorldCommandHandlerRegistry& registry,
    pathfinder::world_inventory::IInventoryRuntime& inventory_runtime)
    : registry_(registry), inventory_runtime_(inventory_runtime) {}

std::vector<WorldCommandOptionDto> InventoryCommandOptionProvider::buildOptions(
    const WorldActorRuntime& actor,
    const IWorldRuntime& runtime) const {

    std::vector<WorldCommandOptionDto> options;

    auto pickup_opts = buildPickupOptions(actor, runtime);
    options.insert(options.end(), pickup_opts.begin(), pickup_opts.end());

    auto drop_opts = buildDropOptions(actor, runtime);
    options.insert(options.end(), drop_opts.begin(), drop_opts.end());

    return options;
}

std::vector<WorldCommandOptionDto> InventoryCommandOptionProvider::buildPickupOptions(
    const WorldActorRuntime& actor,
    const IWorldRuntime& runtime) const {

    std::vector<WorldCommandOptionDto> options;
    if (!registry_.findByKind(WorldCommandKind::Pickup)) {
        return options;
    }

    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) {
        return options;
    }
    const auto& snapshot = snap_res.value();

    std::set<std::string> actor_entity_ids;
    for (const auto& [actor_key, snapshot_actor] : snapshot.actors) {
        (void)actor_key;
        if (!snapshot_actor.entity_id.empty()) {
            actor_entity_ids.insert(snapshot_actor.entity_id);
        }
    }

    for (const auto& [entity_id, entity] : snapshot.entities) {
        if (actor_entity_ids.count(entity_id) > 0) continue;
        if (!entity.coord) continue;
        if (entity.coord->layer_key != actor.coord.layer_key) continue;
        if (entity.location_kind != pathfinder::world_runtime::WorldEntityLocationKind::OnMap) continue;

        int dx = std::abs(entity.coord->x - actor.coord.x);
        int dy = std::abs(entity.coord->y - actor.coord.y);
        if (dx > 1 || dy > 1) continue; // must be adjacent or same cell

        WorldCommandOptionDto opt;
        opt.option_id = makeOptionId("opt_pickup");
        opt.command_kind = WorldCommandKind::Pickup;
        opt.command_key = "pickup";
        opt.label_text = "拾取 " + entity.display_name_key;
        opt.target.target_kind = WorldCommandTargetKind::Entity;
        opt.target.target_entity_id = entity_id;
        opt.enabled = true;
        opt.priority = 15;
        options.push_back(std::move(opt));
    }

    return options;
}

std::vector<WorldCommandOptionDto> InventoryCommandOptionProvider::buildDropOptions(
    const WorldActorRuntime& actor,
    const IWorldRuntime& /*runtime*/) const {

    std::vector<WorldCommandOptionDto> options;
    if (!registry_.findByKind(WorldCommandKind::Drop)) {
        return options;
    }

    // Read-only lookup of actor inventory (do NOT create inventory during option query)
    auto inv_state_res = inventory_runtime_.findActorInventory(actor.actor_key);
    if (inv_state_res.is_error()) {
        return options;
    }
    const auto* inv_state = inv_state_res.value();
    if (!inv_state || inv_state->entries.empty()) {
        return options;
    }

    for (const auto& entry : inv_state->entries) {
        WorldCommandOptionDto opt;
        opt.option_id = makeOptionId("opt_drop");
        opt.command_kind = WorldCommandKind::Drop;
        opt.command_key = "drop";
        opt.label_text = "丢弃 " + entry.entity_key;
        opt.target.target_kind = WorldCommandTargetKind::Entity;
        opt.target.target_entity_id = entry.entity_id;
        opt.enabled = true;
        opt.priority = 12;

        options.push_back(std::move(opt));
    }

    return options;
}

} // namespace pathfinder::client_runtime_bridge
