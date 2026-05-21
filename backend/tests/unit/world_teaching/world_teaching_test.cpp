#include "pathfinder/world_teaching/world_teaching_types.h"
#include "pathfinder/world_teaching/world_teaching_owner_resolver.h"
#include "pathfinder/world_teaching/world_teaching_eligibility_service.h"
#include "pathfinder/world_teaching/world_teaching_loop_bridge.h"
#include "pathfinder/world_teaching/npc_basic_action_knowledge_gate.h"
#include "pathfinder/world_teaching/iworld_actor_query_port.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace pathfinder::world_teaching;
using namespace pathfinder::knowledge;
using namespace pathfinder::world_runtime;
using namespace pathfinder::foundation;

// ---------------------------------------------------------------------------
// Fake Actor Query Port
// ---------------------------------------------------------------------------

class FakeWorldActorQueryPort : public IWorldActorQueryPort {
public:
    void setActor(const std::string& key, int x, int y, const std::string& layer = "surface") {
        WorldActorRuntime actor;
        actor.actor_key = key;
        actor.coord.x = x;
        actor.coord.y = y;
        actor.coord.layer_key = layer;
        actors_[key] = actor;
    }

    std::optional<WorldActorRuntime> findActor(const std::string& actor_key) const override {
        auto it = actors_.find(actor_key);
        if (it != actors_.end()) return it->second;
        return std::nullopt;
    }

private:
    mutable std::map<std::string, WorldActorRuntime> actors_;
};

// ---------------------------------------------------------------------------
// Test Helpers
// ---------------------------------------------------------------------------

static KnowledgeClaim makeTestClaim(
    const std::string& id,
    const std::string& owner_key,
    const std::string& subject,
    const std::string& action,
    const std::string& effect,
    bool teachable,
    KnowledgeStatus status,
    bool usable_for_action,
    KnowledgeRelationType relation = KnowledgeRelationType::HasEffect) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(id);
    claim.owner.kind = KnowledgeOwnerKind::Actor;
    claim.owner.entity_id = EntityId(owner_key);
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject;
    claim.predicate.relation_type = relation;
    claim.predicate.action_key = action;
    claim.predicate.effect_key = effect;
    claim.status = status;
    claim.teaching_profile.teachable = teachable;
    if (teachable) {
        claim.teaching_profile.teaching_message_key = "test_teaching_msg";
    }
    claim.projection_flags.usable_for_action = usable_for_action;
    claim.projection_flags.usable_by_ai = true;
    claim.confidence.confidence = 0.8;
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);

    // Add minimal evidence ref for Active/Teachable claims
    if (status == KnowledgeStatus::Active || status == KnowledgeStatus::Teachable || status == KnowledgeStatus::Shared || status == KnowledgeStatus::Operational) {
        KnowledgeEvidence ev;
        ev.evidence_id = KnowledgeEvidenceId("evd_" + id);
        ev.evidence_kind = KnowledgeEvidenceKind::DirectExperience;
        ev.source_summary_id = pathfinder::memory::MemorySummaryId("sum_" + id);
        ev.supports_claim = true;
        ev.confidence_delta = 0.1;
        ev.reliability = 1.0;
        ev.observed_tick = Tick(1);
        claim.evidence_refs.push_back(ev);
    }

    return claim;
}

// ---------------------------------------------------------------------------
// Enum Roundtrip Tests
// ---------------------------------------------------------------------------

void run_enum_roundtrip_tests() {
    // WorldTeachingFailureKind
    assert(toString(WorldTeachingFailureKind::Unknown) == "Unknown");
    auto fk_unknown = worldTeachingFailureKindFromString("Unknown");
    assert(fk_unknown.is_ok() && fk_unknown.value() == WorldTeachingFailureKind::Unknown);
    assert(toString(WorldTeachingFailureKind::OutOfRange) == "OutOfRange");
    auto fk_r = worldTeachingFailureKindFromString("OutOfRange");
    assert(fk_r.is_ok() && fk_r.value() == WorldTeachingFailureKind::OutOfRange);
    auto fk_bad = worldTeachingFailureKindFromString("BadValue");
    assert(fk_bad.is_error());
    auto fk_test = worldTeachingFailureKindFromString("TestOnly");
    assert(fk_test.is_ok() && fk_test.value() == WorldTeachingFailureKind::TestOnly);

    // WorldTeachingDecision
    assert(toString(WorldTeachingDecision::TaughtNewClaim) == "TaughtNewClaim");
    auto wd_r = worldTeachingDecisionFromString("TaughtNewClaim");
    assert(wd_r.is_ok() && wd_r.value() == WorldTeachingDecision::TaughtNewClaim);
    auto wd_bad = worldTeachingDecisionFromString("BadValue");
    assert(wd_bad.is_error());

    // NpcActionKnowledgeGateDecision
    assert(toString(NpcActionKnowledgeGateDecision::BlockedNoKnowledge) == "BlockedNoKnowledge");
    auto gd_r = npcActionKnowledgeGateDecisionFromString("BlockedNoKnowledge");
    assert(gd_r.is_ok() && gd_r.value() == NpcActionKnowledgeGateDecision::BlockedNoKnowledge);
    auto gd_bad = npcActionKnowledgeGateDecisionFromString("BadValue");
    assert(gd_bad.is_error());

    std::cout << "world_teaching_enum_roundtrip: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Owner Resolver Tests
// ---------------------------------------------------------------------------

void run_owner_resolver_maps_actor_tests() {
    WorldActorKnowledgeOwnerResolver resolver;
    auto owner = resolver.resolve("npc_001");
    assert(owner.kind == KnowledgeOwnerKind::Actor);
    assert(owner.entity_id.value() == "npc_001");
    std::cout << "world_teaching_owner_resolver_maps_actor: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Eligibility Tests
// ---------------------------------------------------------------------------

void run_eligibility_rejects_missing_source_claim_tests() {
    KnowledgeRepository repo;
    FakeWorldActorQueryPort port;
    port.setActor("teacher", 0, 0);
    port.setActor("recipient", 0, 0);

    WorldTeachingRequest req;
    req.teacher_actor_key = "teacher";
    req.recipient_actor_key = "recipient";
    req.source_knowledge_id = KnowledgeId("nonexistent");
    req.max_distance = 2.0;

    WorldTeachingEligibilityService svc;
    auto res = svc.check(req, repo, port);
    assert(!res.allowed);
    assert(res.failure_kind == WorldTeachingFailureKind::SourceClaimMissing);
    std::cout << "world_teaching_eligibility_rejects_missing_source_claim: all passed" << std::endl;
}

void run_eligibility_rejects_owner_mismatch_tests() {
    KnowledgeRepository repo;
    FakeWorldActorQueryPort port;
    port.setActor("teacher", 0, 0);
    port.setActor("recipient", 0, 0);

    // Claim owned by someone else
    auto claim = makeTestClaim("k1", "other_actor", "obj_a", "act_a", "eff_a", true, KnowledgeStatus::Teachable, true);
    repo.put(claim);

    WorldTeachingRequest req;
    req.teacher_actor_key = "teacher";
    req.recipient_actor_key = "recipient";
    req.source_knowledge_id = KnowledgeId("k1");
    req.max_distance = 2.0;

    repo.put(claim);

    WorldTeachingEligibilityService svc;
    auto res = svc.check(req, repo, port);
    assert(!res.allowed);
    assert(res.failure_kind == WorldTeachingFailureKind::SourceClaimOwnerMismatch);
    std::cout << "world_teaching_eligibility_rejects_owner_mismatch: all passed" << std::endl;
}

void run_eligibility_rejects_not_teachable_tests() {
    KnowledgeRepository repo;
    FakeWorldActorQueryPort port;
    port.setActor("teacher", 0, 0);
    port.setActor("recipient", 0, 0);

    auto claim = makeTestClaim("k1", "teacher", "obj_a", "act_a", "eff_a", false, KnowledgeStatus::Active, true);
    repo.put(claim);

    WorldTeachingRequest req;
    req.teacher_actor_key = "teacher";
    req.recipient_actor_key = "recipient";
    req.source_knowledge_id = KnowledgeId("k1");
    req.max_distance = 2.0;

    WorldTeachingEligibilityService svc;
    auto res = svc.check(req, repo, port);
    assert(!res.allowed);
    assert(res.failure_kind == WorldTeachingFailureKind::ClaimNotTeachable);
    std::cout << "world_teaching_eligibility_rejects_not_teachable: all passed" << std::endl;
}

void run_eligibility_rejects_out_of_range_tests() {
    KnowledgeRepository repo;
    FakeWorldActorQueryPort port;
    port.setActor("teacher", 0, 0);
    port.setActor("recipient", 5, 0);

    auto claim = makeTestClaim("k1", "teacher", "obj_a", "act_a", "eff_a", true, KnowledgeStatus::Teachable, true);
    repo.put(claim);

    WorldTeachingRequest req;
    req.teacher_actor_key = "teacher";
    req.recipient_actor_key = "recipient";
    req.source_knowledge_id = KnowledgeId("k1");
    req.max_distance = 2.0;

    WorldTeachingEligibilityService svc;
    auto res = svc.check(req, repo, port);
    assert(!res.allowed);
    assert(res.failure_kind == WorldTeachingFailureKind::OutOfRange);
    std::cout << "world_teaching_eligibility_rejects_out_of_range: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Loop Bridge Tests
// ---------------------------------------------------------------------------

void run_loop_creates_recipient_claim_tests() {
    auto source_claim = makeTestClaim("k1", "teacher", "obj_a", "act_a", "eff_a", true, KnowledgeStatus::Teachable, true);

    WorldActorKnowledgeOwnerResolver resolver;
    auto recipient_owner = resolver.resolve("recipient");

    WorldTeachingLoopBridge bridge;
    auto res = bridge.propagate(
        source_claim,
        recipient_owner,
        {}, // recipient has no claims
        KnowledgePropagationChannel::DirectTeaching,
        KnowledgePropagationTrustBand::High,
        0.8,
        Tick(10),
        "test_loop");

    assert(res.ok);
    assert(!res.recipient_created_claims.empty());

    // Verify recipient claim owner is recipient
    const auto& created = res.recipient_created_claims.front();
    assert(created.owner.kind == KnowledgeOwnerKind::Actor);
    assert(created.owner.entity_id.value() == "recipient");

    // Verify related_knowledge_ids contains source knowledge id
    bool has_source = false;
    for (const auto& rid : created.related_knowledge_ids) {
        if (rid.value() == "k1") {
            has_source = true;
            break;
        }
    }
    assert(has_source);

    std::cout << "world_teaching_loop_creates_recipient_claim: all passed" << std::endl;
}

void run_existing_claim_reinforces_or_updates_tests() {
    // Recipient already has a matching claim
    auto source_claim = makeTestClaim("k1", "teacher", "obj_a", "act_a", "eff_a", true, KnowledgeStatus::Teachable, true);

    // Make a recipient claim with same subject/action/effect but different id
    auto recipient_claim = makeTestClaim("k_recipient_1", "recipient", "obj_a", "act_a", "eff_a", false, KnowledgeStatus::Shared, true);

    WorldActorKnowledgeOwnerResolver resolver;
    auto recipient_owner = resolver.resolve("recipient");

    WorldTeachingLoopBridge bridge;
    auto res = bridge.propagate(
        source_claim,
        recipient_owner,
        {recipient_claim},
        KnowledgePropagationChannel::DirectTeaching,
        KnowledgePropagationTrustBand::High,
        0.8,
        Tick(10),
        "test_loop_reinforce");

    assert(res.ok);
    // Should produce either created or updated claims
    bool has_output = !res.recipient_created_claims.empty() || !res.recipient_updated_claims.empty();
    assert(has_output);

    // Decision should not be Failed
    assert(res.decision != WorldTeachingDecision::Failed);

    std::cout << "world_teaching_existing_claim_reinforces_or_updates: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Action Gate Tests
// ---------------------------------------------------------------------------

void run_action_gate_blocks_unknown_npc_tests() {
    NpcBasicActionKnowledgeGate gate;
    NpcActionKnowledgeGateRequest req;
    req.actor_key = "npc_001";
    req.subject_object_key = "wood";
    req.action_key = "craft";
    req.effect_key = "make_torch";
    req.allow_hypothesis = true;
    req.allow_risk_action = false;
    req.actor_claims = {};

    auto res = gate.check(req);
    assert(!res.allowed);
    assert(res.decision == NpcActionKnowledgeGateDecision::BlockedNoKnowledge);
    std::cout << "world_teaching_action_gate_blocks_unknown_npc: all passed" << std::endl;
}

void run_action_gate_allows_known_action_tests() {
    NpcBasicActionKnowledgeGate gate;
    NpcActionKnowledgeGateRequest req;
    req.actor_key = "npc_001";
    req.subject_object_key = "wood";
    req.action_key = "craft";
    req.effect_key = "make_item";
    req.allow_hypothesis = true;
    req.allow_risk_action = false;

    auto claim = makeTestClaim("k1", "npc_001", "wood", "craft", "make_item", false, KnowledgeStatus::Active, true);
    req.actor_claims.push_back(claim);

    auto res = gate.check(req);
    assert(res.allowed);
    assert(res.decision == NpcActionKnowledgeGateDecision::AllowedByKnowledge);
    assert(res.matched_claim.has_value());
    std::cout << "world_teaching_action_gate_allows_known_action: all passed" << std::endl;
}

void run_action_gate_blocks_ai_disabled_claim_tests() {
    NpcBasicActionKnowledgeGate gate;
    NpcActionKnowledgeGateRequest req;
    req.actor_key = "npc_001";
    req.subject_object_key = "wood";
    req.action_key = "craft";
    req.effect_key = "make_item";
    req.allow_hypothesis = true;
    req.allow_risk_action = false;

    auto claim = makeTestClaim("k1", "npc_001", "wood", "craft", "make_item", false, KnowledgeStatus::Active, true);
    claim.projection_flags.usable_by_ai = false;
    req.actor_claims.push_back(claim);

    auto res = gate.check(req);
    assert(!res.allowed);
    assert(res.decision == NpcActionKnowledgeGateDecision::BlockedNoKnowledge);
    std::cout << "world_teaching_action_gate_blocks_ai_disabled_claim: all passed" << std::endl;
}

void run_action_gate_blocks_risk_without_goal_tests() {
    NpcBasicActionKnowledgeGate gate;
    NpcActionKnowledgeGateRequest req;
    req.actor_key = "npc_001";
    req.subject_object_key = "toxic_obj";
    req.action_key = "eat";
    req.effect_key = "poison";
    req.allow_hypothesis = true;
    req.allow_risk_action = false;

    auto claim = makeTestClaim("k1", "npc_001", "toxic_obj", "eat", "poison", false, KnowledgeStatus::Active, true, KnowledgeRelationType::HasRisk);
    req.actor_claims.push_back(claim);

    auto res = gate.check(req);
    assert(!res.allowed);
    assert(res.decision == NpcActionKnowledgeGateDecision::BlockedDangerWithoutGoal);
    std::cout << "world_teaching_action_gate_blocks_risk_without_goal: all passed" << std::endl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "enum_roundtrip") {
        run_enum_roundtrip_tests();
    } else if (test_name == "owner_resolver_maps_actor") {
        run_owner_resolver_maps_actor_tests();
    } else if (test_name == "eligibility_rejects_missing_source_claim") {
        run_eligibility_rejects_missing_source_claim_tests();
    } else if (test_name == "eligibility_rejects_owner_mismatch") {
        run_eligibility_rejects_owner_mismatch_tests();
    } else if (test_name == "eligibility_rejects_not_teachable") {
        run_eligibility_rejects_not_teachable_tests();
    } else if (test_name == "eligibility_rejects_out_of_range") {
        run_eligibility_rejects_out_of_range_tests();
    } else if (test_name == "loop_creates_recipient_claim") {
        run_loop_creates_recipient_claim_tests();
    } else if (test_name == "existing_claim_reinforces_or_updates") {
        run_existing_claim_reinforces_or_updates_tests();
    } else if (test_name == "action_gate_blocks_unknown_npc") {
        run_action_gate_blocks_unknown_npc_tests();
    } else if (test_name == "action_gate_allows_known_action") {
        run_action_gate_allows_known_action_tests();
    } else if (test_name == "action_gate_blocks_ai_disabled_claim") {
        run_action_gate_blocks_ai_disabled_claim_tests();
    } else if (test_name == "action_gate_blocks_risk_without_goal") {
        run_action_gate_blocks_risk_without_goal_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
