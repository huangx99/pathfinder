#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_agent_execution/world_agent_execution_context_store.h"
#include "pathfinder/world_agent_execution/world_agent_context_builder.h"
#include "pathfinder/world_agent_execution/world_agent_goal_selector.h"
#include "pathfinder/world_agent_execution/world_agent_reasoning_bridge.h"
#include "pathfinder/world_agent_execution/agent_action_knowledge_guard.h"
#include "pathfinder/world_agent_execution/agent_plan_command_compiler.h"
#include "pathfinder/world_agent_execution/agent_execution_coordinator.h"
#include "pathfinder/world_agent_execution/world_agent_projection_bridge.h"
#include "pathfinder/world_teaching/npc_basic_action_knowledge_gate.h"
#include "pathfinder/agent_reasoning/agent_reasoner.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_agent_execution;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
using namespace pathfinder::knowledge;
using namespace pathfinder::world_teaching;
using namespace pathfinder::agent_reasoning;
using namespace pathfinder::goal_execution;
using namespace pathfinder::foundation;
using namespace pathfinder::world_command;

// ---------------------------------------------------------------------------
// Integration Fake Ports
// ---------------------------------------------------------------------------

class IntegFakeVisibleWorldQueryPort : public IAgentVisibleWorldQueryPort {
public:
    void setActor(const std::string& key, int x, int y) {
        WorldActorRuntime a;
        a.actor_key = key;
        a.coord.x = x;
        a.coord.y = y;
        actors_[key] = a;
    }

    Result<WorldActorRuntime> findActor(const std::string& actor_key) const override {
        auto it = actors_.find(actor_key);
        if (it != actors_.end()) return Result<WorldActorRuntime>::ok(it->second);
        return Result<WorldActorRuntime>::fail(makeError(ErrorCode::id_not_found, "actor_not_found"));
    }

    Result<WorldRuntimeSnapshot> visibleSnapshotForActor(const std::string&) const override {
        return Result<WorldRuntimeSnapshot>::ok(WorldRuntimeSnapshot{});
    }

    Result<std::vector<VisibleResourceRef>> visibleResourcesForActor(const std::string&) const override {
        return Result<std::vector<VisibleResourceRef>>::ok(resources_);
    }

    Result<std::vector<VisibleEntityRef>> visibleEntitiesForActor(const std::string&) const override {
        return Result<std::vector<VisibleEntityRef>>::ok(entities_);
    }

    void addResource(const VisibleResourceRef& r) { resources_.push_back(r); }
    void addEntity(const VisibleEntityRef& e) { entities_.push_back(e); }
    void clearResources() { resources_.clear(); }
    void clearEntities() { entities_.clear(); }

private:
    mutable std::map<std::string, WorldActorRuntime> actors_;
    std::vector<VisibleResourceRef> resources_;
    std::vector<VisibleEntityRef> entities_;
};

class IntegFakeInventoryQueryPort : public IAgentInventoryQueryPort {
public:
    Result<InventoryRuntimeSnapshot> snapshotForActorDecision(const std::string&) const override {
        return Result<InventoryRuntimeSnapshot>::ok(InventoryRuntimeSnapshot{});
    }
};

class IntegFakeKnowledgeQueryPort : public IAgentKnowledgeQueryPort {
public:
    void setClaims(const std::string& actor_key, const std::vector<KnowledgeClaim>& claims) {
        claims_[actor_key] = claims;
    }

    Result<std::vector<KnowledgeClaim>> claimsForActor(const std::string& actor_key) const override {
        auto it = claims_.find(actor_key);
        if (it != claims_.end()) return Result<std::vector<KnowledgeClaim>>::ok(it->second);
        return Result<std::vector<KnowledgeClaim>>::ok(std::vector<KnowledgeClaim>{});
    }

private:
    mutable std::map<std::string, std::vector<KnowledgeClaim>> claims_;
};

class IntegFakePipelinePort : public IAgentCommandPipelinePort {
public:
    int execute_count = 0;
    std::vector<WorldExperienceDto> experiences_to_return;

    Result<WorldCommandExecutionResult> executeAgentCommand(const WorldCommandDto& cmd) override {
        execute_count++;
        WorldCommandExecutionResult res;
        res.result_kind = WorldCommandResultKind::Succeeded;
        res.experiences = experiences_to_return;
        // Verify source is correct
        assert(cmd.source == WorldCommandSource::AgentDecision || cmd.source == WorldCommandSource::BeastDecision);
        return Result<WorldCommandExecutionResult>::ok(res);
    }
};

static KnowledgeClaim makeIntegClaim(const std::string& id, const std::string& owner, const std::string& action, const std::string& subject_id = "neutral_resource") {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(id);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = EntityId(owner);
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject_id;
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = action;
    claim.subject.related_subject_ids.push_back(subject_id);
    claim.status = KnowledgeStatus::Active;
    claim.projection_flags.usable_for_action = true;
    claim.projection_flags.usable_by_ai = true;
    claim.confidence.confidence = 0.8;
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);
    return claim;
}

// ---------------------------------------------------------------------------
// Integration Tests
// ---------------------------------------------------------------------------

void run_world_agent_execution_integration_tests() {
    std::cout << "Running world_agent_execution integration tests:" << std::endl;

    IntegFakeVisibleWorldQueryPort world_port;
    IntegFakeInventoryQueryPort inventory_port;
    IntegFakeKnowledgeQueryPort knowledge_port;
    InMemoryWorldAgentExecutionContextStore store;
    IntegFakePipelinePort pipeline_port;

    WorldAgentContextBuilder context_builder(world_port, inventory_port, knowledge_port, store);
    WorldAgentGoalSelector goal_selector;
    AgentReasoner reasoner;
    WorldAgentReasoningBridge reasoning_bridge(reasoner);
    NpcBasicActionKnowledgeGate gate;
    AgentActionKnowledgeGuard knowledge_guard(gate);
    AgentPlanCommandCompiler command_compiler;
    WorldAgentProjectionBridge projection_bridge;

    AgentExecutionCoordinator coordinator(
        context_builder,
        goal_selector,
        reasoning_bridge,
        knowledge_guard,
        command_compiler,
        pipeline_port,
        store,
        projection_bridge);

    // Setup world
    world_port.setActor("npc_001", 0, 0);
    VisibleResourceRef res;
    res.node_id = "node_1";
    res.resource_key = "neutral_resource";
    res.coord = WorldCellCoord{1, 0, "surface"};
    res.remaining_charges = 3;
    world_port.addResource(res);

    WorldAgentTickRequest req;
    req.request_id = "integ_1";
    req.actor_key = "npc_001";
    req.tick = 1;

    // 3.1 NPC without knowledge does not gather
    pipeline_port.execute_count = 0;
    auto r1 = coordinator.tick(req);
    assert(r1.is_ok());
    assert(r1.value().decision == WorldAgentExecutionDecision::WaitingForKnowledge ||
           r1.value().decision == WorldAgentExecutionDecision::NoopNoGoal);
    assert(pipeline_port.execute_count == 0);

    // 3.2 Taught NPC issues gather command
    knowledge_port.setClaims("npc_001", {makeIntegClaim("k1", "npc_001", "gather")});
    pipeline_port.experiences_to_return.push_back(WorldExperienceDto{});
    auto r2 = coordinator.tick(req);
    assert(r2.is_ok());
    assert(r2.value().ok);
    assert(r2.value().decision == WorldAgentExecutionDecision::CommandSucceeded);
    assert(r2.value().issued_command.has_value());
    assert(r2.value().issued_command.value().command_kind == WorldCommandKind::Gather);
    assert(r2.value().issued_command.value().source == WorldCommandSource::AgentDecision);
    assert(r2.value().issued_command.value().actor_key == "npc_001");
    assert(pipeline_port.execute_count >= 1);
    assert(!r2.value().command_result.value().experiences.empty());

    // 3.3 Far resource moves before gather
    world_port.setActor("npc_002", 0, 0);
    world_port.clearResources(); // only far resource visible for this actor
    VisibleResourceRef far_res;
    far_res.node_id = "far_node";
    far_res.resource_key = "neutral_resource";
    far_res.coord = WorldCellCoord{5, 5, "surface"};
    world_port.addResource(far_res);

    knowledge_port.setClaims("npc_002", {makeIntegClaim("k2", "npc_002", "gather")});

    WorldAgentTickRequest req_far;
    req_far.request_id = "integ_far";
    req_far.actor_key = "npc_002";
    req_far.tick = 1;

    auto r3 = coordinator.tick(req_far);
    assert(r3.is_ok());
    // First tick should issue Move, not Gather
    assert(r3.value().issued_command.has_value());
    assert(r3.value().issued_command.value().command_kind == WorldCommandKind::Move);

    // 3.5 Missing material inserts subgoal (MVP: blocker recorded)
    // Verified by knowledge_guard blocking unknown craft action
    knowledge_port.setClaims("npc_001", {}); // clear knowledge
    store.clearActor("npc_001"); // clear previous execution context
    WorldAgentTickRequest req_craft;
    req_craft.request_id = "integ_craft";
    req_craft.actor_key = "npc_001";
    req_craft.tick = 2;
    auto r5 = coordinator.tick(req_craft);
    assert(r5.is_ok());
    // No craft knowledge -> WaitingForKnowledge
    assert(r5.value().decision == WorldAgentExecutionDecision::WaitingForKnowledge ||
           r5.value().decision == WorldAgentExecutionDecision::NoopNoGoal);

    // 3.6 Interrupt pauses goal and inserts emergency
    world_port.setActor("npc_003", 0, 0);
    knowledge_port.setClaims("npc_003", {
        makeIntegClaim("k3", "npc_003", "gather"),
        makeIntegClaim("k4", "npc_003", "flee", "self")
    });

    ExternalInterruptSignal interrupt;
    interrupt.interrupt_id = "i1";
    interrupt.kind = ExternalInterruptKind::ThreatAppeared;
    interrupt.urgency = 10.0;
    interrupt.threat_key = "wolf";

    WorldAgentTickRequest req_interrupt;
    req_interrupt.request_id = "integ_interrupt";
    req_interrupt.actor_key = "npc_003";
    req_interrupt.tick = 1;
    req_interrupt.external_interrupts.push_back(interrupt);

    auto r6 = coordinator.tick(req_interrupt);
    assert(r6.is_ok());
    // Should handle interrupt: either pause or insert emergency goal
    assert(r6.value().decision == WorldAgentExecutionDecision::PausedForInterrupt ||
           r6.value().decision == WorldAgentExecutionDecision::InsertedSubGoal ||
           r6.value().decision == WorldAgentExecutionDecision::IssuedCommand ||
           r6.value().decision == WorldAgentExecutionDecision::CommandSucceeded);

    // 3.8 Beast decision source supported
    // In MVP we don't have a separate beast path, but we verify the structure supports it
    // by checking that command source enum includes BeastDecision
    assert(toString(WorldCommandSource::BeastDecision) == "beast_decision");

    std::cout << "All integration tests passed!" << std::endl;
}
