#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_agent_execution/agent_action_knowledge_guard.h"
#include "pathfinder/world_teaching/npc_basic_action_knowledge_gate.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_agent_execution;
using namespace pathfinder::world_teaching;
using namespace pathfinder::knowledge;
using namespace pathfinder::foundation;
using namespace pathfinder::agent_reasoning;

static KnowledgeClaim makeGuardTestClaim(
    const std::string& id,
    const std::string& owner_key,
    const std::string& action,
    bool usable) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(id);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = EntityId(owner_key);
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = "neutral_resource";
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = action;
    claim.subject.related_subject_ids.push_back("neutral_resource");
    claim.status = KnowledgeStatus::Active;
    claim.projection_flags.usable_for_action = usable;
    claim.projection_flags.usable_by_ai = true;
    claim.confidence.confidence = 0.8;
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);
    return claim;
}

void run_world_agent_execution_knowledge_guard_tests() {
    std::cout << "Running world_agent_execution knowledge guard tests:" << std::endl;

    NpcBasicActionKnowledgeGate gate;
    AgentActionKnowledgeGuard guard(gate);

    std::vector<KnowledgeClaim> empty_claims;
    std::vector<KnowledgeClaim> npc_claims;
    npc_claims.push_back(makeGuardTestClaim("k1", "npc_001", "gather", true));

    AgentPlanStep step;
    step.step_id = "s1";
    step.action_key = "gather";
    step.object_key = "neutral_resource";

    // NPC without knowledge should be blocked for gather
    auto r1 = guard.checkStep("npc_001", step, empty_claims, true, false);
    assert(r1.is_ok());
    assert(!r1.value().allowed);
    assert(r1.value().decision == NpcActionKnowledgeGateDecision::BlockedNoKnowledge);

    // NPC with knowledge should be allowed
    auto r2 = guard.checkStep("npc_001", step, npc_claims, true, false);
    assert(r2.is_ok());
    assert(r2.value().allowed);

    // Move should be allowed without knowledge
    AgentPlanStep move_step;
    move_step.step_id = "s2";
    move_step.action_key = "move";
    auto r3 = guard.checkStep("npc_001", move_step, empty_claims, true, false);
    assert(r3.is_ok());
    assert(r3.value().allowed);

    // Wait should be allowed without knowledge
    AgentPlanStep wait_step;
    wait_step.step_id = "s3";
    wait_step.action_key = "wait";
    auto r4 = guard.checkStep("npc_001", wait_step, empty_claims, true, false);
    assert(r4.is_ok());
    assert(r4.value().allowed);

    // Matched claim owner should be npc_001
    assert(r2.value().matched_claim.has_value());
    assert(r2.value().matched_claim.value().owner.entity_id.value() == "npc_001");

    std::cout << "All knowledge guard tests passed!" << std::endl;
}
