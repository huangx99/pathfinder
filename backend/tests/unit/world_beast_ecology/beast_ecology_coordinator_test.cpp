#include "pathfinder/world_beast_ecology/beast_ecology_coordinator.h"
#include "pathfinder/world_beast_ecology/beast_perception_builder.h"
#include "pathfinder/world_beast_ecology/beast_instinct_gate.h"
#include "pathfinder/world_beast_ecology/beast_decision_policy.h"
#include "pathfinder/world_beast_ecology/beast_command_compiler.h"
#include "pathfinder/world_beast_ecology/beast_interrupt_projector.h"
#include "pathfinder/world_beast_ecology/beast_projection_bridge.h"
#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_beast_ecology;
using namespace pathfinder::foundation;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_command;
using namespace pathfinder::knowledge;
using namespace pathfinder::goal_execution;

// ---------------------------------------------------------------------------
// Fake Ports
// ---------------------------------------------------------------------------

class FakeBeastProfileQueryPort : public IBeastProfileQueryPort {
public:
    void setProfile(const std::string& actor_key, const BeastAgentProfile& profile) {
        profiles_[actor_key] = profile;
    }

    Result<BeastAgentProfile> profileForActor(const std::string& actor_key) const override {
        auto it = profiles_.find(actor_key);
        if (it != profiles_.end()) return Result<BeastAgentProfile>::ok(it->second);
        return Result<BeastAgentProfile>::fail(makeError(ErrorCode::id_not_found, "profile_not_found"));
    }

private:
    mutable std::map<std::string, BeastAgentProfile> profiles_;
};

class FakeBeastWorldQueryPort : public IBeastWorldQueryPort {
public:
    void setActor(const std::string& key, int x, int y) {
        WorldActorRuntime a;
        a.actor_key = key;
        a.coord = WorldCellCoord{x, y, "surface"};
        actors_[key] = a;
    }

    Result<WorldActorRuntime> findBeastActor(const std::string& actor_key) const override {
        auto it = actors_.find(actor_key);
        if (it != actors_.end()) return Result<WorldActorRuntime>::ok(it->second);
        return Result<WorldActorRuntime>::fail(makeError(ErrorCode::id_not_found, "actor_not_found"));
    }

    Result<std::vector<WorldActorRuntime>> nearbyActorsForBeast(const std::string&) const override {
        return Result<std::vector<WorldActorRuntime>>::ok(nearby_actors_);
    }

    Result<std::vector<WorldEntityInstance>> nearbyEntitiesForBeast(const std::string&) const override {
        return Result<std::vector<WorldEntityInstance>>::ok(nearby_entities_);
    }

    Result<std::vector<WorldResourceNodeRuntime>> nearbyResourcesForBeast(const std::string&) const override {
        return Result<std::vector<WorldResourceNodeRuntime>>::ok(nearby_resources_);
    }

    Result<std::vector<BeastPerceptionItem>> nearbyEffectsForBeast(const std::string&) const override {
        return Result<std::vector<BeastPerceptionItem>>::ok(nearby_effects_);
    }

    void setNearbyActors(const std::vector<WorldActorRuntime>& actors) { nearby_actors_ = actors; }
    void setNearbyEntities(const std::vector<WorldEntityInstance>& entities) { nearby_entities_ = entities; }
    void setNearbyResources(const std::vector<WorldResourceNodeRuntime>& resources) { nearby_resources_ = resources; }
    void setNearbyEffects(const std::vector<BeastPerceptionItem>& effects) { nearby_effects_ = effects; }

private:
    mutable std::map<std::string, WorldActorRuntime> actors_;
    std::vector<WorldActorRuntime> nearby_actors_;
    std::vector<WorldEntityInstance> nearby_entities_;
    std::vector<WorldResourceNodeRuntime> nearby_resources_;
    std::vector<BeastPerceptionItem> nearby_effects_;
};

class FakeBeastKnowledgeQueryPort : public IBeastKnowledgeQueryPort {
public:
    void setClaims(const std::string& actor_key, const std::vector<KnowledgeClaim>& claims) {
        claims_[actor_key] = claims;
    }

    Result<std::vector<KnowledgeClaim>> claimsForBeast(const std::string& actor_key) const override {
        auto it = claims_.find(actor_key);
        if (it != claims_.end()) return Result<std::vector<KnowledgeClaim>>::ok(it->second);
        return Result<std::vector<KnowledgeClaim>>::ok(std::vector<KnowledgeClaim>{});
    }

private:
    mutable std::map<std::string, std::vector<KnowledgeClaim>> claims_;
};

class FakeBeastPipelinePort : public IBeastCommandPipelinePort {
public:
    int execute_count = 0;
    WorldCommandExecutionResult last_result;

    Result<WorldCommandExecutionResult> executeBeastCommand(const WorldCommandDto&) override {
        execute_count++;
        return Result<WorldCommandExecutionResult>::ok(last_result);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static BeastAgentProfile makeTestProfile() {
    BeastAgentProfile profile;
    profile.profile_key = "test_predator";
    profile.temperament = BeastTemperamentKind::Predatory;
    profile.vision_radius = 5;
    profile.base_aggression = 60.0;
    profile.base_fear = 20.0;
    profile.prey_tags.push_back("prey_tag");
    profile.danger_tags.push_back("danger_effect_tag");

    BeastInstinctCapability cap_wait;
    cap_wait.capability_key = "wait";
    cap_wait.action_kind = BeastActionIntentKind::Wait;
    cap_wait.command_kind = WorldCommandKind::Wait;
    cap_wait.requires_target = false;
    profile.instinct_capabilities.push_back(cap_wait);

    BeastInstinctCapability cap_move;
    cap_move.capability_key = "move";
    cap_move.action_kind = BeastActionIntentKind::Wander;
    cap_move.command_kind = WorldCommandKind::Move;
    cap_move.requires_target = false;
    profile.instinct_capabilities.push_back(cap_move);

    BeastInstinctCapability cap_attack;
    cap_attack.capability_key = "attack";
    cap_attack.action_kind = BeastActionIntentKind::Attack;
    cap_attack.command_kind = WorldCommandKind::Attack;
    cap_attack.requires_target = true;
    profile.instinct_capabilities.push_back(cap_attack);

    BeastInstinctCapability cap_flee;
    cap_flee.capability_key = "flee";
    cap_flee.action_kind = BeastActionIntentKind::Flee;
    cap_flee.command_kind = WorldCommandKind::Flee;
    cap_flee.requires_target = true;
    profile.instinct_capabilities.push_back(cap_flee);

    BeastInstinctCapability cap_approach;
    cap_approach.capability_key = "approach";
    cap_approach.action_kind = BeastActionIntentKind::Approach;
    cap_approach.command_kind = WorldCommandKind::Move;
    cap_approach.requires_target = true;
    profile.instinct_capabilities.push_back(cap_approach);

    return profile;
}

void run_world_beast_ecology_coordinator_tests() {
    std::cout << "Running world_beast_ecology coordinator tests:" << std::endl;

    BeastPerceptionBuilder perception_builder;
    BeastInstinctGate instinct_gate;
    BeastDecisionPolicy decision_policy;
    BeastCommandCompiler command_compiler;
    BeastInterruptProjector interrupt_projector;
    BeastProjectionBridge projection_bridge;

    FakeBeastProfileQueryPort profile_port;
    FakeBeastWorldQueryPort world_port;
    FakeBeastKnowledgeQueryPort knowledge_port;
    FakeBeastPipelinePort pipeline_port;

    BeastEcologyCoordinator coordinator(
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

    BeastAgentProfile profile = makeTestProfile();
    profile_port.setProfile("beast_1", profile);
    world_port.setActor("beast_1", 0, 0);

    // 1. No targets -> Wait (P52 safe fallback), pipeline executes
    BeastTickRequest req;
    req.request_id = "r1";
    req.actor_key = "beast_1";
    req.tick = 1;
    req.allow_issue_command = true;

    pipeline_port.last_result.result_kind = WorldCommandResultKind::Succeeded;
    pipeline_port.execute_count = 0;

    auto r1 = coordinator.tick(req);
    assert(r1.is_ok());
    assert(r1.value().ok == true);
    assert(r1.value().actor_key == "beast_1");
    assert(r1.value().selected_intent.has_value());
    assert(r1.value().selected_intent.value().kind == BeastActionIntentKind::Wait);
    assert(r1.value().issued_command.has_value());
    assert(r1.value().issued_command.value().source == WorldCommandSource::BeastDecision);
    assert(r1.value().issued_command.value().command_kind == WorldCommandKind::Wait);
    assert(pipeline_port.execute_count == 1);

    // 2. Profile missing -> fail
    req.actor_key = "missing_beast";
    auto r2 = coordinator.tick(req);
    assert(r2.is_ok());
    assert(r2.value().ok == false);
    req.actor_key = "beast_1";

    // 3. Dry run -> no pipeline call
    pipeline_port.execute_count = 0;
    req.allow_issue_command = false;
    auto r3 = coordinator.tick(req);
    assert(r3.is_ok());
    assert(r3.value().ok == true);
    assert(pipeline_port.execute_count == 0);
    req.allow_issue_command = true;

    // 4. With prey -> Approach (raw actor + entity with tags, builder constructs perception)
    WorldActorRuntime prey_actor;
    prey_actor.actor_key = "prey_actor_1";
    prey_actor.entity_id = "prey_entity";
    prey_actor.coord = WorldCellCoord{2, 0, "surface"};

    WorldEntityInstance prey_entity;
    prey_entity.entity_id = "prey_entity";
    prey_entity.entity_key = "prey_entity_type";
    prey_entity.coord = WorldCellCoord{2, 0, "surface"};
    prey_entity.tag_keys.push_back("prey_tag");

    world_port.setNearbyActors({prey_actor});
    world_port.setNearbyEntities({prey_entity});
    world_port.setNearbyResources({});
    world_port.setNearbyEffects({});

    pipeline_port.execute_count = 0;
    auto r4 = coordinator.tick(req);
    assert(r4.is_ok());
    assert(r4.value().selected_intent.value().kind == BeastActionIntentKind::Approach);
    assert(r4.value().issued_command.value().command_kind == WorldCommandKind::Move);
    assert(pipeline_port.execute_count == 1);

    // 5. With danger -> Flee
    BeastPerceptionItem danger;
    danger.perception_id = "p2";
    danger.kind = BeastPerceptionKind::Effect;
    danger.target_ref = "effect_fire_1";
    danger.target_key = "danger_effect_tag";
    danger.coord = WorldCellCoord{1, 0, "surface"};
    danger.distance = 1;
    danger.visible = true;
    danger.tag_keys.push_back("danger_effect_tag");
    world_port.setNearbyActors({prey_actor});
    world_port.setNearbyEntities({prey_entity});
    world_port.setNearbyResources({});
    world_port.setNearbyEffects({danger});

    pipeline_port.execute_count = 0;
    auto r5 = coordinator.tick(req);
    assert(r5.is_ok());
    assert(r5.value().selected_intent.value().kind == BeastActionIntentKind::Flee);
    assert(r5.value().issued_command.value().source == WorldCommandSource::BeastDecision);
    assert(pipeline_port.execute_count == 1);

    // 6. Interrupt projection when approaching prey near actor
    world_port.setNearbyActors({prey_actor});
    world_port.setNearbyEntities({prey_entity});
    world_port.setNearbyResources({});
    world_port.setNearbyEffects({});
    auto r6 = coordinator.tick(req);
    assert(r6.is_ok());
    assert(r6.value().selected_intent.value().kind == BeastActionIntentKind::Approach);
    assert(!r6.value().interrupts.empty());
    assert(r6.value().interrupts[0].kind == ExternalInterruptKind::ThreatAppeared);
    assert(r6.value().interrupts[0].target_actor_keys[0] == "prey_actor_1");

    // 7. Command result experiences preserved
    pipeline_port.last_result.result_kind = WorldCommandResultKind::Succeeded;
    pipeline_port.last_result.experiences.push_back(WorldExperienceDto{});
    pipeline_port.last_result.experiences.back().experience_id = "exp_1";
    pipeline_port.last_result.experiences.back().actor_key = "beast_1";
    world_port.setNearbyActors({});
    world_port.setNearbyEntities({});
    world_port.setNearbyResources({});
    world_port.setNearbyEffects({});

    auto r7 = coordinator.tick(req);
    assert(r7.is_ok());
    assert(r7.value().command_result.has_value());
    assert(!r7.value().command_result.value().experiences.empty());
    assert(r7.value().command_result.value().experiences[0].experience_id == "exp_1");

    // 8. Knowledge claims not read when disabled
    req.allow_learning_claims = false;
    auto r8 = coordinator.tick(req);
    assert(r8.is_ok());
    req.allow_learning_claims = true;

    std::cout << "All coordinator tests passed!" << std::endl;
}
