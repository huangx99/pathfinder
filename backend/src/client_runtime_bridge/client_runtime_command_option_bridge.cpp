#include "pathfinder/client_runtime_bridge/client_runtime_command_option_bridge.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <set>
#include <sstream>
#include <cmath>

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

    std::string objectName(const pathfinder::content::ContentRegistry& registry, const std::string& object_key) {
        const auto key = "object." + object_key + ".name";
        auto translated = registry.translate("zh_cn", key);
        return translated == key ? object_key : translated;
    }

    int inputQuantity(const pathfinder::content::ReactionInputDto& input) {
        return input.min > 0 ? input.min : (input.quantity > 0 ? input.quantity : 1);
    }

    std::string reactionOutputName(const pathfinder::content::ContentRegistry& registry,
                                   const pathfinder::content::ReactionDefinitionContent& reaction) {
        if (reaction.outputs.empty()) return {};
        return objectName(registry, reaction.outputs.front().object_key);
    }

    std::string reactionIngredientText(const pathfinder::content::ContentRegistry& registry,
                                       const pathfinder::content::ReactionDefinitionContent& reaction) {
        std::ostringstream oss;
        for (size_t i = 0; i < reaction.inputs.size(); ++i) {
            if (i > 0) oss << " + ";
            const auto& input = reaction.inputs[i];
            oss << objectName(registry, input.object_key) << "×" << inputQuantity(input);
            if (!input.state.empty()) oss << "[" << input.state << "]";
        }
        return oss.str();
    }

    std::string craftLabel(const pathfinder::content::ContentRegistry& registry,
                           const pathfinder::content::ReactionDefinitionContent& reaction,
                           const std::string& verb = "制作") {
        const auto output = reactionOutputName(registry, reaction);
        const auto ingredients = reactionIngredientText(registry, reaction);
        std::string label = output.empty() ? verb : verb + " " + output;
        if (!ingredients.empty()) label += "（原料：" + ingredients + "）";
        return label;
    }

    bool sameOrAdjacent(const pathfinder::world_runtime::WorldCellCoord& a,
                        const pathfinder::world_runtime::WorldCellCoord& b) {
        return a.layer_key == b.layer_key && std::abs(a.x - b.x) <= 1 && std::abs(a.y - b.y) <= 1;
    }

    bool hasTag(const std::vector<std::string>& tags, const std::string& tag) {
        return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }

    bool isSocialNpcActor(const pathfinder::content::ContentRegistry& registry,
                          const pathfinder::world_runtime::IWorldRuntime& runtime,
                          const pathfinder::world_runtime::WorldActorRuntime& actor) {
        if (!actor.alive) return false;
        auto entity_res = runtime.findEntity(actor.entity_id);
        if (entity_res.is_error()) return false;
        const auto* agent = registry.findAgent(entity_res.value()->entity_key);
        if (!agent) return false;
        if (agent->embodiment == "wildlife" || agent->cognition_band == "instinctive") return false;
        return agent->can_teach || agent->embodiment == "humanoid" || agent->cognition_band == "social";
    }

    pathfinder::knowledge::KnowledgeOwner actorKnowledgeOwner(const std::string& actor_key) {
        pathfinder::knowledge::KnowledgeOwner owner;
        owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
        owner.entity_id = pathfinder::foundation::EntityId(actor_key);
        return owner;
    }

    std::string claimLabel(const pathfinder::content::ContentRegistry& registry,
                           const pathfinder::knowledge::KnowledgeClaim& claim) {
        auto render_template = [&](const pathfinder::content::KnowledgeTemplateContent& tmpl) -> std::string {
            if (tmpl.summary_key.empty()) return {};
            const auto translated = registry.translate("zh_cn", tmpl.summary_key);
            return translated == tmpl.summary_key ? tmpl.summary_key : translated;
        };
        for (const auto& reason_key : claim.reason_keys) {
            if (const auto* tmpl = registry.findKnowledgeTemplate(reason_key)) {
                auto label = render_template(*tmpl);
                if (!label.empty()) return label;
            }
        }
        for (const auto* tmpl : registry.allKnowledgeTemplates()) {
            if (!tmpl) continue;
            if (tmpl->subject_object_key != claim.subject.subject_id) continue;
            if (tmpl->action_key != claim.predicate.action_key) continue;
            if (tmpl->effect_key != claim.predicate.effect_key) continue;
            auto label = render_template(*tmpl);
            if (!label.empty()) return label;
        }
        return objectName(registry, claim.subject.subject_id) + " " + claim.predicate.action_key + " -> " + claim.predicate.effect_key;
    }

    bool claimCanDriveNpcWork(const pathfinder::knowledge::KnowledgeClaim& claim,
                              const pathfinder::content::ReactionDefinitionContent& reaction) {
        if (!claim.projection_flags.usable_by_ai) return false;
        if (claim.predicate.action_key != reaction.action_key && claim.predicate.action_key != "craft") return false;
        if (claim.predicate.effect_key != reaction.effect_key) return false;
        const auto status = pathfinder::knowledge::toString(claim.status);
        return status == "hypothesis" || status == "candidate" || status == "active" ||
               status == "teachable" || status == "shared" || status == "operational";
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
    , craft_provider_(nullptr)
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

void ClientRuntimeCommandOptionBridge::setCraftServices(
    pathfinder::world_reaction::WorldReactionService* reaction_service,
    std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry) {
    craft_provider_.reset();
    craft_content_registry_ = std::move(content_registry);
    if (reaction_service && craft_content_registry_) {
        craft_provider_ = std::make_unique<CraftCommandOptionProvider>(
            registry_, *craft_content_registry_, *reaction_service);
    }
}


void ClientRuntimeCommandOptionBridge::setMapEditServices(
    std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry) {
    map_edit_provider_.reset();
    if (content_registry) {
        map_edit_provider_ = std::make_unique<pathfinder::world_map_edit::MapEditCommandOptionProvider>(
            registry_, *content_registry);
    }
}

void ClientRuntimeCommandOptionBridge::setKnowledgeServices(
    pathfinder::knowledge::KnowledgeRepository* knowledge_repository,
    std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry) {
    teaching_provider_.reset();
    knowledge_repository_ = knowledge_repository;
    knowledge_content_registry_ = std::move(content_registry);
    if (knowledge_repository_ && knowledge_content_registry_) {
        teaching_provider_ = std::make_unique<TeachingAndNpcWorkCommandOptionProvider>(
            registry_, *knowledge_content_registry_, *knowledge_repository_);
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
    bool run_craft = true;
    bool run_inventory = true;
    bool run_teaching = true;
    bool run_map_edit = true;

    if (!request.provider_kinds.empty()) {
        run_wait = false;
        move_move = false;
        run_inspect = false;
        run_harvest = false;
        run_craft = false;
        run_inventory = false;
        run_teaching = false;
        run_map_edit = false;
        for (const auto& kind : request.provider_kinds) {
            if (kind == ClientCommandOptionProviderKind::Wait) run_wait = true;
            if (kind == ClientCommandOptionProviderKind::Move) move_move = true;
            if (kind == ClientCommandOptionProviderKind::Inspect) run_inspect = true;
            if (kind == ClientCommandOptionProviderKind::Harvest) run_harvest = true;
            if (kind == ClientCommandOptionProviderKind::Craft) run_craft = true;
            if (kind == ClientCommandOptionProviderKind::Inventory) run_inventory = true;
            if (kind == ClientCommandOptionProviderKind::Teach) run_teaching = true;
            if (kind == ClientCommandOptionProviderKind::MapEdit) run_map_edit = true;
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

    if (run_craft && craft_provider_) {
        auto craft_opts = craft_provider_->buildOptions(*actor);
        options.insert(options.end(), craft_opts.begin(), craft_opts.end());
    }

    if (run_inventory && inventory_provider_) {
        auto inv_opts = inventory_provider_->buildOptions(*actor, runtime_);
        options.insert(options.end(), inv_opts.begin(), inv_opts.end());
    }

    if (run_teaching && teaching_provider_) {
        auto teach_opts = teaching_provider_->buildOptions(*actor, runtime_);
        options.insert(options.end(), teach_opts.begin(), teach_opts.end());
    }

    if (run_map_edit && map_edit_provider_) {
        auto map_edit_opts = map_edit_provider_->buildOptions(*actor, runtime_);
        options.insert(options.end(), map_edit_opts.begin(), map_edit_opts.end());
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
        opt.label_text = label_text + " object." + node.resource_key + ".name";
        opt.target.target_kind = WorldCommandTargetKind::Entity;
        opt.target.target_entity_id = node_id;
        opt.enabled = true;
        opt.priority = 20;

        options.push_back(std::move(opt));
    }

    return options;
}


// ---------------------------------------------------------------------------
// CraftCommandOptionProvider
// ---------------------------------------------------------------------------

CraftCommandOptionProvider::CraftCommandOptionProvider(
    const WorldCommandHandlerRegistry& registry,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::world_reaction::WorldReactionService& reaction_service)
    : registry_(registry), content_registry_(content_registry), reaction_service_(reaction_service) {}

std::vector<WorldCommandOptionDto> CraftCommandOptionProvider::buildOptions(
    const WorldActorRuntime& actor) const {

    std::vector<WorldCommandOptionDto> options;
    if (!registry_.findByKind(WorldCommandKind::Craft)) {
        return options;
    }

    for (const auto* reaction : content_registry_.allReactions()) {
        if (!reaction || (reaction->action_key != "craft" && reaction->action_key != "use" && reaction->action_key != "combine")) continue;

        pathfinder::world_reaction::WorldReactionRequest req;
        req.reaction_command_id = "option_check_" + reaction->key.value();
        req.actor_key = actor.actor_key;
        req.reaction_key = reaction->key.value();
        req.action_kind = pathfinder::world_reaction::WorldReactionActionKind::Craft;
        req.output_location_policy = pathfinder::world_reaction::WorldReactionOutputLocationPolicy::ActorInventory;

        auto validate_res = reaction_service_.validate(req);
        if (!validate_res.is_ok()) continue;

        WorldCommandOptionDto opt;
        opt.option_id = makeOptionId("opt_craft");
        opt.command_kind = WorldCommandKind::Craft;
        opt.command_key = "craft";
        opt.label_text = craftLabel(content_registry_, *reaction);
        opt.target.target_kind = WorldCommandTargetKind::Recipe;
        opt.target.recipe_key = reaction->key.value();
        opt.enabled = true;
        opt.priority = 18;
        options.push_back(std::move(opt));
    }

    return options;
}


// ---------------------------------------------------------------------------
// TeachingAndNpcWorkCommandOptionProvider
// ---------------------------------------------------------------------------

TeachingAndNpcWorkCommandOptionProvider::TeachingAndNpcWorkCommandOptionProvider(
    const WorldCommandHandlerRegistry& registry,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository)
    : registry_(registry), content_registry_(content_registry), knowledge_repository_(knowledge_repository) {}

std::vector<WorldCommandOptionDto> TeachingAndNpcWorkCommandOptionProvider::buildOptions(
    const WorldActorRuntime& actor,
    const IWorldRuntime& runtime) const {

    std::vector<WorldCommandOptionDto> options;
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) return options;
    const auto& snapshot = snap_res.value();

    auto teacher_claims_res = knowledge_repository_.listByOwner(actorKnowledgeOwner(actor.actor_key));
    std::vector<pathfinder::knowledge::KnowledgeClaim> teacher_claims;
    if (teacher_claims_res.is_ok()) teacher_claims = teacher_claims_res.value();

    for (const auto& [candidate_actor_key, candidate_actor] : snapshot.actors) {
        if (candidate_actor_key == actor.actor_key) continue;
        if (!isSocialNpcActor(content_registry_, runtime, candidate_actor)) continue;
        if (!sameOrAdjacent(actor.coord, candidate_actor.coord)) continue;

        std::string target_entity_id = candidate_actor.entity_id;
        if (target_entity_id.empty()) continue;

        if (registry_.findByKind(WorldCommandKind::Inspect)) {
            WorldCommandOptionDto inspect;
            inspect.option_id = makeOptionId("opt_inspect_actor_knowledge");
            inspect.command_kind = WorldCommandKind::Inspect;
            inspect.command_key = "inspect_actor_knowledge";
            inspect.label_text = "查看NPC知识";
            inspect.target.target_kind = WorldCommandTargetKind::Entity;
            inspect.target.target_entity_id = target_entity_id;
            inspect.target.target_actor_key = candidate_actor_key;
            inspect.enabled = true;
            inspect.priority = 7;
            options.push_back(std::move(inspect));
        }

        if (registry_.findByKind(WorldCommandKind::Use)) {
            WorldCommandOptionDto follow;
            follow.option_id = makeOptionId("opt_actor_follow");
            follow.command_kind = WorldCommandKind::Use;
            follow.command_key = "set_actor_follow_player";
            follow.label_text = "让NPC跟随我";
            follow.target.target_kind = WorldCommandTargetKind::Actor;
            follow.target.target_actor_key = candidate_actor_key;
            follow.target.target_entity_id = target_entity_id;
            follow.enabled = true;
            follow.priority = 8;
            options.push_back(std::move(follow));
        }

        auto worker_claims_res = knowledge_repository_.listByOwner(actorKnowledgeOwner(candidate_actor_key));
        std::vector<pathfinder::knowledge::KnowledgeClaim> worker_claims;
        if (worker_claims_res.is_ok()) worker_claims = worker_claims_res.value();

        auto recipientAlreadyKnows = [&](const pathfinder::knowledge::KnowledgeClaim& source_claim) {
            const auto source_id = source_claim.knowledge_id.value();
            return std::any_of(worker_claims.begin(), worker_claims.end(), [&](const auto& known) {
                return known.knowledge_id.value() == source_id ||
                    (known.subject.subject_id == source_claim.subject.subject_id &&
                     known.predicate.action_key == source_claim.predicate.action_key &&
                     known.predicate.effect_key == source_claim.predicate.effect_key);
            });
        };

        if (registry_.findByKind(WorldCommandKind::Teach)) {
            for (const auto& claim : teacher_claims) {
                if (!claim.teaching_profile.teachable || recipientAlreadyKnows(claim)) continue;
                WorldCommandOptionDto teach;
                teach.option_id = makeOptionId("opt_teach_knowledge");
                teach.command_kind = WorldCommandKind::Teach;
                teach.command_key = "teach";
                teach.label_text = "教NPC：" + claimLabel(content_registry_, claim);
                teach.target.target_kind = WorldCommandTargetKind::Actor;
                teach.target.target_actor_key = candidate_actor_key;
                teach.target.target_entity_id = target_entity_id;
                teach.target.knowledge_key = claim.knowledge_id.value();
                teach.enabled = true;
                teach.priority = 20;
                options.push_back(std::move(teach));
            }
        }

        if (!registry_.findByKind(WorldCommandKind::Use)) continue;
        for (const auto* reaction : content_registry_.allReactions()) {
            if (!reaction || (reaction->action_key != "craft" && reaction->action_key != "use" && reaction->action_key != "combine")) continue;
            bool matched = false;
            for (const auto& claim : worker_claims) {
                if (claimCanDriveNpcWork(claim, *reaction)) { matched = true; break; }
            }
            if (!matched) continue;

            WorldCommandOptionDto order;
            order.option_id = makeOptionId("opt_order_npc_craft");
            order.command_kind = WorldCommandKind::Use;
            order.command_key = "order_actor_craft";
            order.label_text = "命令NPC：" + craftLabel(content_registry_, *reaction, "制作");
            order.target.target_kind = WorldCommandTargetKind::Actor;
            order.target.target_actor_key = candidate_actor_key;
            order.target.target_entity_id = target_entity_id;
            order.target.recipe_key = reaction->key.value();
            order.enabled = true;
            order.priority = 25;
            options.push_back(std::move(order));
        }
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
        if (hasTag(entity.tag_keys, "creature") ||
            hasTag(entity.tag_keys, "danger") ||
            hasTag(entity.tag_keys, "predator")) continue;

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
