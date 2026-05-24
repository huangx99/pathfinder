#include "pathfinder/world_beast_ecology/beast_runtime_module.h"
#include "pathfinder/world_beast_ecology/beast_command_compiler.h"
#include "pathfinder/world_beast_ecology/beast_decision_policy.h"
#include "pathfinder/world_beast_ecology/beast_ecology_coordinator.h"
#include "pathfinder/world_beast_ecology/beast_instinct_gate.h"
#include "pathfinder/world_beast_ecology/beast_interrupt_projector.h"
#include "pathfinder/world_beast_ecology/beast_perception_builder.h"
#include "pathfinder/world_beast_ecology/beast_projection_bridge.h"
#include "pathfinder/world_modules/core/world_module_utils.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace pathfinder::world_beast_ecology {
namespace {

using pathfinder::world_modules::actorKnowledgeOwner;
using pathfinder::world_modules::manhattan;
using pathfinder::world_modules::numericAsInt;

class RuntimeBeastProfilePort final : public pathfinder::world_beast_ecology::IBeastProfileQueryPort {
public:
    RuntimeBeastProfilePort(
        const pathfinder::world_runtime::IWorldRuntime& runtime,
        const pathfinder::content::ContentRegistry& content_registry)
        : runtime_(runtime), content_registry_(content_registry) {}

    pathfinder::foundation::Result<pathfinder::world_beast_ecology::BeastAgentProfile> profileForActor(
        const std::string& actor_key) const override {
        auto actor_res = runtime_.findActor(actor_key);
        if (actor_res.is_error()) return pathfinder::foundation::Result<pathfinder::world_beast_ecology::BeastAgentProfile>::fail(actor_res.errors());
        auto entity_res = runtime_.findEntity(actor_res.value()->entity_id);
        if (entity_res.is_error()) return pathfinder::foundation::Result<pathfinder::world_beast_ecology::BeastAgentProfile>::fail(entity_res.errors());
        const auto* agent = content_registry_.findAgent(entity_res.value()->entity_key);
        if (!agent || agent->embodiment != "wildlife") {
            return pathfinder::foundation::Result<pathfinder::world_beast_ecology::BeastAgentProfile>::fail(
                pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_not_found, "not_wild_agent"));
        }

        pathfinder::world_beast_ecology::BeastAgentProfile profile;
        profile.profile_key = agent->key.value();
        profile.display_key = agent->display_key;
        profile.temperament = pathfinder::world_beast_ecology::BeastTemperamentKind::Predatory;
        profile.vision_radius = std::max(3, actor_res.value()->vision_radius);
        profile.hearing_radius = profile.vision_radius;
        profile.base_aggression = std::max(40.0, agent->default_hunger);
        profile.base_fear = std::max(10.0, agent->default_fear);
        profile.attack_damage = std::clamp(numericAsInt(entity_res.value()->numeric_states, "attack_damage", 1), 1, 1000);
        profile.prey_tags = {"prey", "humanoid", "player", "npc"};
        profile.danger_tags = {"fire", "fire_source", "deterrent"};
        profile.deterrent_tags = {"fire", "fire_source", "deterrent"};

        auto add_cap = [&](const std::string& key,
                           pathfinder::world_beast_ecology::BeastActionIntentKind action,
                           pathfinder::world_command::WorldCommandKind command,
                           bool requires_target) {
            pathfinder::world_beast_ecology::BeastInstinctCapability cap;
            cap.capability_key = key;
            cap.action_kind = action;
            cap.command_kind = command;
            cap.requires_target = requires_target;
            cap.risk_limit = 100.0;
            profile.instinct_capabilities.push_back(std::move(cap));
        };
        add_cap("wild_move_approach", pathfinder::world_beast_ecology::BeastActionIntentKind::Approach, pathfinder::world_command::WorldCommandKind::Move, true);
        add_cap("wild_move_flee", pathfinder::world_beast_ecology::BeastActionIntentKind::Flee, pathfinder::world_command::WorldCommandKind::Move, true);
        add_cap("wild_move_wander", pathfinder::world_beast_ecology::BeastActionIntentKind::Wander, pathfinder::world_command::WorldCommandKind::Move, true);
        add_cap("wild_attack", pathfinder::world_beast_ecology::BeastActionIntentKind::Attack, pathfinder::world_command::WorldCommandKind::Attack, true);
        add_cap("wild_wait", pathfinder::world_beast_ecology::BeastActionIntentKind::Wait, pathfinder::world_command::WorldCommandKind::Wait, false);

        auto valid = profile.validateBasic();
        if (valid.is_error()) return pathfinder::foundation::Result<pathfinder::world_beast_ecology::BeastAgentProfile>::fail(valid.errors());
        return pathfinder::foundation::Result<pathfinder::world_beast_ecology::BeastAgentProfile>::ok(std::move(profile));
    }

private:
    const pathfinder::world_runtime::IWorldRuntime& runtime_;
    const pathfinder::content::ContentRegistry& content_registry_;
};

class RuntimeBeastWorldPort final : public pathfinder::world_beast_ecology::IBeastWorldQueryPort {
public:
    explicit RuntimeBeastWorldPort(const pathfinder::world_runtime::IWorldRuntime& runtime) : runtime_(runtime) {}

    pathfinder::foundation::Result<pathfinder::world_runtime::WorldActorRuntime> findBeastActor(const std::string& actor_key) const override {
        auto res = runtime_.findActor(actor_key);
        if (res.is_error()) return pathfinder::foundation::Result<pathfinder::world_runtime::WorldActorRuntime>::fail(res.errors());
        return pathfinder::foundation::Result<pathfinder::world_runtime::WorldActorRuntime>::ok(*res.value());
    }

    pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldActorRuntime>> nearbyActorsForBeast(const std::string& actor_key) const override {
        std::vector<pathfinder::world_runtime::WorldActorRuntime> actors;
        auto snap_res = runtime_.snapshotForDebug();
        if (snap_res.is_error()) return pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldActorRuntime>>::fail(snap_res.errors());
        for (const auto& [key, actor] : snap_res.value().actors) {
            (void)actor_key;
            if (actor.alive) actors.push_back(actor);
        }
        return pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldActorRuntime>>::ok(std::move(actors));
    }

    pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldEntityInstance>> nearbyEntitiesForBeast(const std::string& actor_key) const override {
        std::vector<pathfinder::world_runtime::WorldEntityInstance> entities;
        auto actor_res = runtime_.findActor(actor_key);
        auto snap_res = runtime_.snapshotForDebug();
        if (actor_res.is_error()) return pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldEntityInstance>>::fail(actor_res.errors());
        if (snap_res.is_error()) return pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldEntityInstance>>::fail(snap_res.errors());
        for (const auto& [entity_id, entity] : snap_res.value().entities) {
            (void)entity_id;
            if (!entity.coord.has_value()) continue;
            if (entity.coord->layer_key != actor_res.value()->coord.layer_key) continue;
            if (manhattan(*entity.coord, actor_res.value()->coord) <= actor_res.value()->vision_radius + 2) {
                entities.push_back(entity);
            }
        }
        return pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldEntityInstance>>::ok(std::move(entities));
    }

    pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldResourceNodeRuntime>> nearbyResourcesForBeast(const std::string& /*actor_key*/) const override {
        return pathfinder::foundation::Result<std::vector<pathfinder::world_runtime::WorldResourceNodeRuntime>>::ok({});
    }

    pathfinder::foundation::Result<std::vector<pathfinder::world_beast_ecology::BeastPerceptionItem>> nearbyEffectsForBeast(const std::string& /*actor_key*/) const override {
        return pathfinder::foundation::Result<std::vector<pathfinder::world_beast_ecology::BeastPerceptionItem>>::ok({});
    }

private:
    const pathfinder::world_runtime::IWorldRuntime& runtime_;
};

class RuntimeBeastKnowledgePort final : public pathfinder::world_beast_ecology::IBeastKnowledgeQueryPort {
public:
    explicit RuntimeBeastKnowledgePort(pathfinder::knowledge::KnowledgeRepository& repository) : repository_(repository) {}

    pathfinder::foundation::Result<std::vector<pathfinder::knowledge::KnowledgeClaim>> claimsForBeast(const std::string& actor_key) const override {
        return repository_.listByOwner(actorKnowledgeOwner(actor_key));
    }

private:
    pathfinder::knowledge::KnowledgeRepository& repository_;
};

class RuntimeBeastPipelinePort final : public pathfinder::world_beast_ecology::IBeastCommandPipelinePort {
public:
    RuntimeBeastPipelinePort(pathfinder::world_command::WorldCommandPipeline& pipeline, uint64_t base_version)
        : pipeline_(pipeline), projection_version_(base_version) {}

    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> executeBeastCommand(
        const pathfinder::world_command::WorldCommandDto& command) override {
        pipeline_.setCurrentProjectionVersion(projection_version_);
        auto response_res = pipeline_.execute("wildlife_internal", command);
        if (response_res.is_error()) return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::fail(response_res.errors());
        const auto response = response_res.value();
        projection_version_ = response.projection_patch.new_projection_version;

        pathfinder::world_command::WorldCommandExecutionResult execution;
        execution.result_kind = response.result.result_kind;
        execution.failure_reason_keys = response.result.failure_reason_keys;
        execution.warning_keys = response.result.warning_keys;
        execution.state_deltas = response.result.state_deltas;
        execution.spawned_commands = response.result.spawned_commands;
        execution.events = response.event_feed;
        execution.experiences = response.experiences;
        execution.frontend_hints = response.frontend_hints;
        execution.projection_patch_override = response.projection_patch;
        return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(execution));
    }

    uint64_t projectionVersion() const { return projection_version_; }

private:
    pathfinder::world_command::WorldCommandPipeline& pipeline_;
    uint64_t projection_version_ = 0;
};

bool isWildlifeActor(const pathfinder::world_runtime::WorldActorRuntime& actor,
                     const pathfinder::world_runtime::IWorldRuntime& runtime,
                     const pathfinder::content::ContentRegistry& content_registry) {
    if (!actor.alive) return false;
    auto entity_res = runtime.findEntity(actor.entity_id);
    if (entity_res.is_error()) return false;
    const auto* agent = content_registry.findAgent(entity_res.value()->entity_key);
    return agent && agent->embodiment == "wildlife";
}

void mergeWildlifeResult(pathfinder::client_protocol::ClientCommandResponse& response,
                         const pathfinder::world_beast_ecology::BeastTickResult& tick_result) {
    response.event_feed.insert(response.event_feed.end(), tick_result.events.begin(), tick_result.events.end());
    if (tick_result.command_result.has_value()) {
        const auto& command_result = tick_result.command_result.value();
        response.experiences.insert(response.experiences.end(), command_result.experiences.begin(), command_result.experiences.end());
        response.result.state_deltas.insert(response.result.state_deltas.end(), command_result.state_deltas.begin(), command_result.state_deltas.end());
        response.frontend_hints.insert(response.frontend_hints.end(), command_result.frontend_hints.begin(), command_result.frontend_hints.end());
    }
    if (tick_result.projection_patch.has_value()) {
        const auto& patch = tick_result.projection_patch.value();
        response.projection_patch.changed_cells.insert(response.projection_patch.changed_cells.end(), patch.changed_cells.begin(), patch.changed_cells.end());
        response.projection_patch.changed_entities.insert(response.projection_patch.changed_entities.end(), patch.changed_entities.begin(), patch.changed_entities.end());
        response.projection_patch.changed_inventories.insert(response.projection_patch.changed_inventories.end(), patch.changed_inventories.begin(), patch.changed_inventories.end());
        response.projection_patch.changed_knowledge.insert(response.projection_patch.changed_knowledge.end(), patch.changed_knowledge.begin(), patch.changed_knowledge.end());
        response.projection_patch.changed_area_effects.insert(response.projection_patch.changed_area_effects.end(), patch.changed_area_effects.begin(), patch.changed_area_effects.end());
    }
}

void runWildlifeTicksInternal(pathfinder::world_runtime::WorldGridRuntime& runtime,
                      const pathfinder::content::ContentRegistry& content_registry,
                      pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
                      pathfinder::world_command::WorldCommandPipeline& pipeline,
                      pathfinder::client_protocol::ClientCommandResponse& response) {
    if (response.result.result_kind != pathfinder::world_command::WorldCommandResultKind::Succeeded) return;
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) return;

    RuntimeBeastProfilePort profile_port(runtime, content_registry);
    RuntimeBeastWorldPort world_port(runtime);
    RuntimeBeastKnowledgePort knowledge_port(knowledge_repository);
    RuntimeBeastPipelinePort pipeline_port(pipeline, response.new_projection_version);
    pathfinder::world_beast_ecology::BeastPerceptionBuilder perception_builder;
    pathfinder::world_beast_ecology::BeastInstinctGate instinct_gate;
    pathfinder::world_beast_ecology::BeastDecisionPolicy decision_policy;
    pathfinder::world_beast_ecology::BeastCommandCompiler command_compiler;
    pathfinder::world_beast_ecology::BeastInterruptProjector interrupt_projector;
    pathfinder::world_beast_ecology::BeastProjectionBridge projection_bridge;
    pathfinder::world_beast_ecology::BeastEcologyCoordinator coordinator(
        perception_builder,
        instinct_gate,
        decision_policy,
        command_compiler,
        interrupt_projector,
        projection_bridge,
        profile_port,
        world_port,
        knowledge_port,
        pipeline_port);

    for (const auto& [actor_key, actor] : snap_res.value().actors) {
        if (!isWildlifeActor(actor, runtime, content_registry)) continue;
        pathfinder::world_beast_ecology::BeastTickRequest request;
        request.request_id = response.result.command_id + "_wild_" + actor_key;
        request.actor_key = actor_key;
        request.tick = response.new_projection_version;
        auto tick_res = coordinator.tick(request);
        if (tick_res.is_error()) continue;
        mergeWildlifeResult(response, tick_res.value());
    }
    response.new_projection_version = std::max(response.new_projection_version, pipeline_port.projectionVersion());
    response.projection_patch.new_projection_version = response.new_projection_version;
}



} // namespace

void runWildlifeTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    pathfinder::client_protocol::ClientCommandResponse& response) {
    runWildlifeTicksInternal(runtime, content_registry, knowledge_repository, pipeline, response);
}

} // namespace pathfinder::world_beast_ecology
