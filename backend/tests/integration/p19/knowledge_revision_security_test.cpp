#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_revision.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_projection.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::knowledge;
using namespace pathfinder::memory;
using namespace pathfinder::foundation;

static KnowledgeOwner makeValidOwner() {
    KnowledgeOwner owner;
    owner.kind = KnowledgeOwnerKind::Agent;
    owner.entity_id = EntityId("agent_001");
    return owner;
}

static KnowledgeSubject makeValidSubject() {
    KnowledgeSubject subject;
    subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = "berry_red";
    return subject;
}

static KnowledgePredicate makeValidPredicate() {
    KnowledgePredicate pred;
    pred.relation_type = KnowledgeRelationType::HasEffect;
    pred.action_key = "eat";
    pred.effect_key = "restore_hunger";
    return pred;
}

static KnowledgeEvidence makeValidEvidence(bool supports = true) {
    KnowledgeEvidence ev;
    ev.evidence_id = KnowledgeEvidenceId("kevd_test_001");
    ev.evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    ev.source_summary_id = MemorySummaryId("sum_001");
    ev.supports_claim = supports;
    ev.confidence_delta = 0.5;
    ev.reliability = 0.9;
    return ev;
}

static KnowledgeConfidence makeValidConfidence(double conf = 0.75) {
    KnowledgeConfidence c;
    c.confidence = conf;
    c.band = confidenceToBand(conf);
    c.support_count = 3;
    c.conflict_count = 0;
    return c;
}

static MemorySummary makeValidSummary(double strength = 0.75, int contradiction_count = 0) {
    MemorySummary summary;
    summary.summary_id = MemorySummaryId("sum_001");
    summary.key.owner.kind = MemoryOwnerKind::Agent;
    summary.key.owner.entity_id = EntityId("agent_001");
    summary.key.subject.kind = MemorySubjectKind::ObjectDefinition;
    summary.key.subject.subject_id = "berry_red";
    summary.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
    summary.schema_version = "memory_summary.v1";
    summary.compression_level = MemoryCompressionLevel::LightSummary;
    summary.highest_importance = MemoryImportance::Normal;
    summary.aggregate_strength = strength;
    summary.source_memory_count = 5;
    summary.source_memory_ids = {
        MemoryId("mem_001"), MemoryId("mem_002"), MemoryId("mem_003"),
        MemoryId("mem_004"), MemoryId("mem_005")
    };
    summary.representative_memory_ids = {MemoryId("mem_001"), MemoryId("mem_002")};
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    summary.evidence_refs = {ref};
    summary.has_long_term_source = true;
    summary.teaching_candidate_count = 2;
    summary.success_count = 4;
    summary.failure_count = 0;
    summary.warning_count = 0;
    summary.accident_count = 0;
    summary.contradiction_count = contradiction_count;
    summary.summarized_tick = Tick(100);
    summary.last_observed_tick = Tick(100);
    return summary;
}

static KnowledgeClaim makeValidClaim(const std::string& know_id,
                                      KnowledgeStatus status = KnowledgeStatus::Active,
                                      double confidence = 0.75) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(know_id);
    claim.owner = makeValidOwner();
    claim.subject = makeValidSubject();
    claim.predicate = makeValidPredicate();
    claim.confidence = makeValidConfidence(confidence);
    claim.status = status;
    claim.evidence_refs = {makeValidEvidence()};
    claim.created_tick = Tick(10);
    claim.updated_tick = Tick(10);
    if (status == KnowledgeStatus::Teachable) {
        claim.teaching_profile.teachable = true;
        claim.teaching_profile.teaching_message_key = "msg";
        claim.projection_flags.usable_for_teaching = true;
    }
    if (status == KnowledgeStatus::Active) {
        claim.projection_flags.usable_for_action = true;
    }
    return claim;
}

static KnowledgeFormationInput makeValidFormationInput(double summary_strength = 0.75,
                                                        int contradiction_count = 0) {
    KnowledgeFormationInput input;
    input.owner = makeValidOwner();
    input.summary = makeValidSummary(summary_strength, contradiction_count);
    input.target_relation = KnowledgeRelationType::HasEffect;
    input.action_key = "eat";
    input.effect_key = "restore_hunger";
    return input;
}

static KnowledgeRevisionInput makeValidRevisionInput(const std::vector<KnowledgeClaim>& existing = {},
                                                      double summary_strength = 0.75,
                                                      int contradiction_count = 0) {
    KnowledgeRevisionInput input;
    input.formation_input = makeValidFormationInput(summary_strength, contradiction_count);
    input.existing_claims = existing;
    input.revision_tick = Tick(200);
    return input;
}

void run_knowledge_revision_security_tests() {
    // ============================================================
    // hidden truth in action_key/effect_key/reason_keys rejected
    // ============================================================
    {
        KnowledgeRevisionInput input;
        input.formation_input = makeValidFormationInput();
        input.revision_tick = Tick(200);
        assert(input.validateBasic().is_ok());

        input.formation_input.action_key = "edible_profile";
        assert(input.validateBasic().is_error());
        input.formation_input.action_key = "eat";

        input.formation_input.effect_key = "hunger_delta";
        assert(input.validateBasic().is_error());
        input.formation_input.effect_key = "restore_hunger";

        input.reason_keys = {"gamestate"};
        assert(input.validateBasic().is_error());
        input.reason_keys.clear();

        input.reason_keys = {"raw_state"};
        assert(input.validateBasic().is_error());
        input.reason_keys.clear();

        input.reason_keys = {"hidden_truth"};
        assert(input.validateBasic().is_error());
        input.reason_keys.clear();
    }

    // ============================================================
    // TestOnly enum rejected
    // ============================================================
    {
        KnowledgeConflictSignal signal;
        signal.conflict_type = KnowledgeConflictType::TestOnly;
        signal.existing_knowledge_id = KnowledgeId("know_001");
        signal.subject = makeValidSubject();
        signal.existing_predicate = makeValidPredicate();
        signal.incoming_predicate = makeValidPredicate();
        signal.severity = 0.5;
        assert(signal.validateBasic().is_error());

        KnowledgeRevisionAssessment assessment;
        assessment.recommended_strategy = KnowledgeResolutionStrategy::TestOnly;
        assessment.confidence_delta = 0.5;
        assert(assessment.validateBasic().is_error());

        KnowledgeRevisionDecision decision = KnowledgeRevisionDecision::TestOnly;
        assert(decision == KnowledgeRevisionDecision::TestOnly);
    }

    // ============================================================
    // ObjectDefinition hidden truth not read
    // ============================================================
    {
        auto keys = knowledgeForbiddenKeys();
        bool has_hidden = false;
        for (const auto& k : keys) {
            if (k == "hidden_truth" || k == "object_truth" || k == "edible_profile") {
                has_hidden = true;
            }
        }
        assert(has_hidden);

        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput();
        input.formation_input.action_key = "objectdefinition hidden";
        auto result = service.revise(input);
        assert(result.is_error());
    }

    // ============================================================
    // GameState raw state not referenced
    // ============================================================
    {
        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput();
        input.reason_keys = {"raw_state_access"};
        auto result = service.revise(input);
        assert(result.is_error());

        input.reason_keys = {"gamestate_read"};
        result = service.revise(input);
        assert(result.is_error());
    }

    // ============================================================
    // invalid claim input returns Result::fail, not Skipped/NoChange
    // ============================================================
    {
        KnowledgeRevisionService service;
        KnowledgeClaim claim = makeValidClaim("know_001");
        claim.status = KnowledgeStatus::TestOnly;
        auto input = makeValidRevisionInput({claim});
        auto result = service.revise(input);
        assert(result.is_error());
    }

    // ============================================================
    // Deprecated claim requires valid reason or superseded_by
    // ============================================================
    {
        KnowledgeClaim claim = makeValidClaim("know_dep", KnowledgeStatus::Deprecated, 0.50);
        // Missing both superseded_by and valid reason
        assert(claim.validateBasic().is_error());

        claim.superseded_by_knowledge_id = KnowledgeId("know_new");
        assert(claim.validateBasic().is_ok());

        claim.superseded_by_knowledge_id = KnowledgeId("");
        claim.reason_keys.push_back("deprecated_by_revision");
        assert(claim.validateBasic().is_ok());
    }

    // ============================================================
    // Disproven claim requires negative evidence and conflict_count
    // ============================================================
    {
        KnowledgeClaim claim = makeValidClaim("know_dis", KnowledgeStatus::Disproven, 0.10);
        claim.confidence.conflict_count = 0;
        assert(claim.validateBasic().is_error());

        claim.confidence.conflict_count = 3;
        // No negative evidence yet
        assert(claim.validateBasic().is_error());

        claim.evidence_refs.push_back(makeValidEvidence(false));
        claim.reason_keys.push_back("disproven_by_revision");
        assert(claim.validateBasic().is_ok());
    }

    // ============================================================
    // KnowledgeRevisionResult Rejected is not ok()
    // ============================================================
    {
        KnowledgeRevisionResult result;
        result.decision = KnowledgeRevisionDecision::Rejected;
        assert(!result.ok());
        assert(result.validateBasic().is_error());
    }

    std::cout << "knowledge_revision_security tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_revision_security_tests();
    return 0;
}
