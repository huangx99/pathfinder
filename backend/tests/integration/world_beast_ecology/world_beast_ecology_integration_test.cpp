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

class IntegrationFakeProfilePort : public IBeastProfileQueryPort {
public:
    Result<BeastAgentProfile> profileForActor(const std::string&) const override {
        return Result<BeastAgentProfile>::ok(profile_);
    }
    BeastAgentProfile profile_;
};

class IntegrationFakeWorldPort : public IBeastWorldQueryPort {
public:
    Result<WorldActorRuntime> findBeastActor(const std::string&) const override {
        return Result<WorldActorRuntime>::ok(actor_);
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
    WorldActorRuntime actor_;
    std::vector<WorldActorRuntime> nearby_actors_;
    std::vector<WorldEntityInstance> nearby_entities_;
    std::vector<WorldResourceNodeRuntime> nearby_resources_;
    std::vector<BeastPerceptionItem> nearby_effects_;
};

class IntegrationFakeKnowledgePort : public IBeastKnowledgeQueryPort {
public:
    Result<std::vector<KnowledgeClaim>> claimsForBeast(const std::string&) const override {
        return Result<std::vector<KnowledgeClaim>>::ok(claims_);
    }
    std::vector<KnowledgeClaim> claims_;
};

class IntegrationFakePipelinePort : public IBeastCommandPipelinePort {
public:
    int execute_count = 0;
    WorldCommandExecutionResult last_result;

    Result<WorldCommandExecutionResult> executeBeastCommand(const WorldCommandDto&) override {
        execute_count++;
        return Result<WorldCommandExecutionResult>::ok(last_result);
    }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static BeastAgentProfile makePredatorProfile() {
    BeastAgentProfile profile;
    profile.profile_key = "predator_v1";
    profile.temperament = BeastTemperamentKind::Predatory;
    profile.vision_radius = 5;
    profile.base_aggression = 70.0;
    profile.base_fear = 20.0;
    profile.prey_tags.push_back("prey_tag");
    profile.danger_tags.push_back("danger_tag");

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

    BeastInstinctCapability cap_approach;
    cap_approach.capability_key = "approach";
    cap_approach.action_kind = BeastActionIntentKind::Approach;
    cap_approach.command_kind = WorldCommandKind::Move;
    cap_approach.requires_target = true;
    profile.instinct_capabilities.push_back(cap_approach);

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

    return profile;
}

static KnowledgeClaim makeDangerClaim(const std::string& subject_key) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId("k_" + subject_key);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = EntityId("beast_1");
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject_key;
    claim.predicate.relation_type = KnowledgeRelationType::IsDangerousUnder;
    claim.status = KnowledgeStatus::Active;
    claim.projection_flags.usable_by_ai = true;
    claim.confidence.confidence = 0.85;
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);
    return claim;
}

// ---------------------------------------------------------------------------
// Integration Tests
// ---------------------------------------------------------------------------

void run_world_beast_ecology_integration_tests() {
    std::cout << "Running world_beast_ecology integration tests:" << std::endl;

    BeastPerceptionBuilder perception_builder;
    BeastInstinctGate instinct_gate;
    BeastDecisionPolicy decision_policy;
    BeastCommandCompiler command_compiler;
    BeastInterruptProjector interrupt_projector;
    BeastProjectionBridge projection_bridge;

    IntegrationFakeProfilePort profile_port;
    IntegrationFakeWorldPort world_port;
    IntegrationFakeKnowledgePort knowledge_port;
    IntegrationFakePipelinePort pipeline_port;

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

    profile_port.profile_ = makePredatorProfile();
    world_port.actor_.actor_key = "beast_1";
    world_port.actor_.coord = WorldCellCoord{0, 0, "surface"};

    BeastTickRequest req;
    req.request_id = "int_r1";
    req.actor_key = "beast_1";
    req.tick = 1;
    req.allow_issue_command = true;
    req.allow_learning_claims = true;

    // 1. Tick executes pipeline (no targets -> Wait)
    pipeline_port.execute_count = 0;
    pipeline_port.last_result.result_kind = WorldCommandResultKind::Succeeded;
    auto r1 = coordinator.tick(req);
    assert(r1.is_ok());
    assert(r1.value().ok);
    assert(r1.value().selected_intent.value().kind == BeastActionIntentKind::Wait);
    assert(pipeline_port.execute_count == 1);
    assert(r1.value().issued_command.has_value());
    assert(r1.value().issued_command.value().source == WorldCommandSource::BeastDecision);
    assert(r1.value().issued_command.value().command_kind == WorldCommandKind::Wait);

    // 2. External interrupt on approach (raw actor + entity through builder)
    WorldActorRuntime npc_actor;
    npc_actor.actor_key = "npc_1";
    npc_actor.entity_id = "npc_entity";
    npc_actor.coord = WorldCellCoord{2, 0, "surface"};

    WorldEntityInstance npc_entity;
    npc_entity.entity_id = "npc_entity";
    npc_entity.entity_key = "npc_type";
    npc_entity.coord = WorldCellCoord{2, 0, "surface"};
    npc_entity.tag_keys.push_back("prey_tag");

    world_port.nearby_actors_ = {npc_actor};
    world_port.nearby_entities_ = {npc_entity};
    world_port.nearby_resources_ = {};
    world_port.nearby_effects_ = {};

    pipeline_port.execute_count = 0;
    auto r2 = coordinator.tick(req);
    assert(r2.is_ok());
    assert(r2.value().selected_intent.value().kind == BeastActionIntentKind::Approach);
    assert(!r2.value().interrupts.empty());
    assert(r2.value().interrupts[0].kind == ExternalInterruptKind::ThreatAppeared);
    assert(r2.value().interrupts[0].target_actor_keys[0] == "npc_1");

    // 3. Learned risk changes intent from Wait to Flee
    WorldEntityInstance strange_obj;
    strange_obj.entity_id = "obj_strange_1";
    strange_obj.entity_key = "strange_object";
    strange_obj.coord = WorldCellCoord{1, 0, "surface"};

    world_port.nearby_actors_ = {};
    world_port.nearby_entities_ = {strange_obj};
    world_port.nearby_resources_ = {};
    world_port.nearby_effects_ = {};

    // Without learned risk -> Wait (not prey, not danger)
    knowledge_port.claims_.clear();
    auto r3a = coordinator.tick(req);
    assert(r3a.is_ok());
    assert(r3a.value().selected_intent.value().kind == BeastActionIntentKind::Wait);

    // With HasRisk learned claim -> Flee
    knowledge_port.claims_.push_back(makeDangerClaim("strange_object"));
    auto r3b = coordinator.tick(req);
    assert(r3b.is_ok());
    assert(r3b.value().selected_intent.value().kind == BeastActionIntentKind::Flee);
    assert(r3b.value().selected_intent.value().reason_kind == BeastDecisionReasonKind::LearnedRisk);

    // 4. Experiences preserved
    world_port.nearby_actors_ = {};
    world_port.nearby_entities_ = {};
    world_port.nearby_resources_ = {};
    world_port.nearby_effects_ = {};
    knowledge_port.claims_.clear();
    pipeline_port.last_result.result_kind = WorldCommandResultKind::Succeeded;
    pipeline_port.last_result.experiences.clear();
    WorldExperienceDto exp;
    exp.experience_id = "exp_danger";
    exp.actor_key = "beast_1";
    exp.command_key = "flee";
    pipeline_port.last_result.experiences.push_back(exp);

    auto r4 = coordinator.tick(req);
    assert(r4.is_ok());
    assert(r4.value().command_result.has_value());
    assert(r4.value().command_result.value().experiences.size() == 1);
    assert(r4.value().command_result.value().experiences[0].experience_id == "exp_danger");

    // 5. Projection bridge works
    auto projection = projection_bridge.project(r4.value());
    assert(projection.actor_key == "beast_1");
    assert(!projection.intent_kind_str.empty());

    std::cout << "All integration tests passed!" << std::endl;
}
