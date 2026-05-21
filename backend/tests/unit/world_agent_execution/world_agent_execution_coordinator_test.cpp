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
// Fake Ports
// ---------------------------------------------------------------------------

class CoordFakeVisibleWorldQueryPort : public IAgentVisibleWorldQueryPort {
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
        std::vector<VisibleResourceRef> refs;
        VisibleResourceRef r;
        r.node_id = "node_1";
        r.resource_key = "neutral_resource";
        r.coord = WorldCellCoord{1, 0, "surface"};
        r.remaining_charges = 3;
        refs.push_back(r);
        return Result<std::vector<VisibleResourceRef>>::ok(refs);
    }

    Result<std::vector<VisibleEntityRef>> visibleEntitiesForActor(const std::string&) const override {
        return Result<std::vector<VisibleEntityRef>>::ok(std::vector<VisibleEntityRef>{});
    }

private:
    mutable std::map<std::string, WorldActorRuntime> actors_;
};

class CoordFakeInventoryQueryPort : public IAgentInventoryQueryPort {
public:
    Result<InventoryRuntimeSnapshot> snapshotForActorDecision(const std::string&) const override {
        return Result<InventoryRuntimeSnapshot>::ok(InventoryRuntimeSnapshot{});
    }
};

class CoordFakeKnowledgeQueryPort : public IAgentKnowledgeQueryPort {
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

class CoordFakePipelinePort : public IAgentCommandPipelinePort {
public:
    int execute_count = 0;
    WorldCommandExecutionResult last_result;

    Result<WorldCommandExecutionResult> executeAgentCommand(const WorldCommandDto&) override {
        execute_count++;
        return Result<WorldCommandExecutionResult>::ok(last_result);
    }
};

static KnowledgeClaim makeCoordClaim(const std::string& id, const std::string& owner, const std::string& action) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(id);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = EntityId(owner);
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = "neutral_resource";
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = action;
    claim.subject.related_subject_ids.push_back("neutral_resource");
    claim.status = KnowledgeStatus::Active;
    claim.projection_flags.usable_for_action = true;
    claim.projection_flags.usable_by_ai = true;
    claim.confidence.confidence = 0.8;
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);
    return claim;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void run_world_agent_execution_coordinator_tests() {
    std::cout << "Running world_agent_execution coordinator tests:" << std::endl;

    CoordFakeVisibleWorldQueryPort world_port;
    CoordFakeInventoryQueryPort inventory_port;
    CoordFakeKnowledgeQueryPort knowledge_port;
    InMemoryWorldAgentExecutionContextStore store;
    CoordFakePipelinePort pipeline_port;

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

    // Actor missing
    WorldAgentTickRequest req;
    req.request_id = "r1";
    req.actor_key = "missing";
    req.tick = 1;
    auto r1 = coordinator.tick(req);
    assert(r1.is_ok());
    assert(!r1.value().ok);
    assert(r1.value().failure_kind == WorldAgentExecutionFailureKind::ContextBuildFailed);

    // NPC without knowledge: should not gather
    world_port.setActor("npc_001", 0, 0);
    req.actor_key = "npc_001";
    pipeline_port.execute_count = 0;
    auto r2 = coordinator.tick(req);
    assert(r2.is_ok());
    // No knowledge -> WaitingForKnowledge or NoopNoGoal depending on path
    // With visible resources, goal selector creates AcquireObject goal, but knowledge guard blocks gather
    assert(r2.value().decision == WorldAgentExecutionDecision::WaitingForKnowledge ||
           r2.value().decision == WorldAgentExecutionDecision::NoopNoGoal);
    assert(pipeline_port.execute_count == 0);

    // NPC with gather knowledge and adjacent resource
    knowledge_port.setClaims("npc_001", {makeCoordClaim("k1", "npc_001", "gather")});
    pipeline_port.last_result.result_kind = WorldCommandResultKind::Succeeded;
    pipeline_port.last_result.events.push_back(WorldEventDto{});

    auto r3 = coordinator.tick(req);
    assert(r3.is_ok());
    assert(r3.value().ok);
    assert(r3.value().decision == WorldAgentExecutionDecision::CommandSucceeded);
    assert(r3.value().issued_command.has_value());
    assert(r3.value().issued_command.value().command_kind == WorldCommandKind::Gather);
    assert(r3.value().issued_command.value().source == WorldCommandSource::AgentDecision);
    assert(r3.value().issued_command.value().actor_key == "npc_001");
    assert(pipeline_port.execute_count >= 1);
    assert(!r3.value().events.empty());

    // Dry run: should not call pipeline
    pipeline_port.execute_count = 0;
    req.allow_issue_command = false;
    auto r4 = coordinator.tick(req);
    assert(r4.is_ok());
    assert(r4.value().decision == WorldAgentExecutionDecision::IssuedCommand);
    assert(pipeline_port.execute_count == 0);

    // Command blocked updates execution context
    req.allow_issue_command = true;
    pipeline_port.last_result.result_kind = WorldCommandResultKind::Blocked;
    auto r5 = coordinator.tick(req);
    assert(r5.is_ok());
    assert(r5.value().decision == WorldAgentExecutionDecision::CommandBlocked);

    std::cout << "All coordinator tests passed!" << std::endl;
}
