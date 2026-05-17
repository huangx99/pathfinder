#include "pathfinder/knowledge/knowledge_revision.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_projection.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::knowledge;
using namespace pathfinder::memory;
using namespace pathfinder::foundation;

// ============================================================
// Helpers
// ============================================================

static KnowledgeOwner makeValidOwner() {
    KnowledgeOwner owner;
    owner.kind = KnowledgeOwnerKind::Agent;
    owner.entity_id = EntityId("agent_001");
    return owner;
}

static KnowledgeSubject makeValidSubject(const std::string& id = "berry_red") {
    KnowledgeSubject subject;
    subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = id;
    return subject;
}

static KnowledgePredicate makeValidPredicate(const std::string& action = "eat",
                                              const std::string& effect = "restore_hunger",
                                              KnowledgeRelationType rt = KnowledgeRelationType::HasEffect) {
    KnowledgePredicate pred;
    pred.relation_type = rt;
    pred.action_key = action;
    pred.effect_key = effect;
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

// ============================================================
// Tests
// ============================================================

void test_knowledge_revision_options_validate_basic() {
    KnowledgeRevisionOptions opts;
    assert(opts.validateBasic().is_ok());

    opts.reinforce_weight = 1.5;
    assert(opts.validateBasic().is_error());
    opts.reinforce_weight = 0.12;

    opts.weak_conflict_penalty = -0.1;
    assert(opts.validateBasic().is_error());
    opts.weak_conflict_penalty = 0.15;

    opts.strong_conflict_min_count = 0;
    assert(opts.validateBasic().is_error());
    opts.strong_conflict_min_count = 2;

    opts.max_revision_evidence_refs = 0;
    assert(opts.validateBasic().is_error());
    opts.max_revision_evidence_refs = 50;

    assert(opts.validateBasic().is_ok());
}

void test_knowledge_conflict_signal_validate_basic() {
    KnowledgeConflictSignal signal;
    signal.conflict_type = KnowledgeConflictType::SameClaimSupport;
    signal.existing_knowledge_id = KnowledgeId("know_001");
    signal.subject = makeValidSubject();
    signal.existing_predicate = makeValidPredicate();
    signal.incoming_predicate = makeValidPredicate();
    signal.severity = 0.5;
    assert(signal.validateBasic().is_ok());

    signal.conflict_type = KnowledgeConflictType::Unknown;
    assert(signal.validateBasic().is_error());
    signal.conflict_type = KnowledgeConflictType::TestOnly;
    assert(signal.validateBasic().is_error());
    signal.conflict_type = KnowledgeConflictType::SameClaimSupport;

    signal.existing_knowledge_id = KnowledgeId("");
    assert(signal.validateBasic().is_error());
    signal.existing_knowledge_id = KnowledgeId("know_001");

    signal.severity = 1.5;
    assert(signal.validateBasic().is_error());
    signal.severity = 0.5;

    signal.should_specialize = true;
    signal.incoming_conditions.clear();
    signal.existing_conditions.clear();
    assert(signal.validateBasic().is_error());
    signal.incoming_conditions.push_back(KnowledgeCondition{.condition_key = "fresh"});
    assert(signal.validateBasic().is_ok());
}

void test_knowledge_revision_input_owner_mismatch_rejected() {
    KnowledgeClaim claim = makeValidClaim("know_001");
    claim.owner.entity_id = EntityId("agent_002");

    KnowledgeRevisionInput input;
    input.formation_input = makeValidFormationInput();
    input.existing_claims = {claim};
    input.revision_tick = Tick(200);

    assert(input.validateBasic().is_error());
}

void test_knowledge_revision_input_invalid_existing_claim_rejected() {
    KnowledgeClaim claim = makeValidClaim("know_001");
    claim.status = KnowledgeStatus::TestOnly;

    KnowledgeRevisionInput input;
    input.formation_input = makeValidFormationInput();
    input.existing_claims = {claim};
    input.revision_tick = Tick(200);

    assert(input.validateBasic().is_error());
}

void test_knowledge_revision_assessment_invalid_enum_rejected() {
    KnowledgeRevisionAssessment assessment;
    assessment.recommended_strategy = KnowledgeResolutionStrategy::Unknown;
    assert(assessment.validateBasic().is_error());

    assessment.recommended_strategy = KnowledgeResolutionStrategy::Reinforce;
    assessment.confidence_delta = 0.5;
    assert(assessment.validateBasic().is_ok());

    assessment.confidence_delta = 1.5;
    assert(assessment.validateBasic().is_error());
    assessment.confidence_delta = 0.5;

    assessment.has_conflict = true;
    assert(assessment.validateBasic().is_error()); // empty conflict_signals
    assessment.conflict_signals.clear();
    KnowledgeConflictSignal signal;
    signal.conflict_type = KnowledgeConflictType::WeakContradiction;
    signal.existing_knowledge_id = KnowledgeId("know_001");
    signal.subject = makeValidSubject();
    signal.existing_predicate = makeValidPredicate();
    signal.incoming_predicate = makeValidPredicate();
    signal.severity = 0.5;
    assessment.conflict_signals.push_back(signal);
    assert(assessment.validateBasic().is_ok());

    assessment.should_disprove_existing = true;
    assessment.has_conflict = false;
    assert(assessment.validateBasic().is_error());
}

void test_knowledge_claim_patch_disproven_requires_negative_evidence() {
    KnowledgeClaimPatch patch;
    patch.knowledge_id = KnowledgeId("know_001");
    patch.status_before = KnowledgeStatus::Active;
    patch.status_after = KnowledgeStatus::Disproven;
    patch.confidence_before = makeValidConfidence();
    patch.confidence_after = makeValidConfidence(0.1);
    assert(patch.validateBasic().is_error()); // no negative evidence

    KnowledgeEvidence neg_ev = makeValidEvidence(false);
    patch.added_evidence_refs.push_back(neg_ev);
    assert(patch.validateBasic().is_ok());
}

void test_knowledge_revision_plan_ok_dto_validate_basic() {
    KnowledgeRevisionPlanner planner;
    KnowledgeClaim claim = makeValidClaim("know_001");
    auto input = makeValidRevisionInput({claim}, 0.75, 0);
    auto plan_result = planner.planRevision(input);
    assert(plan_result.is_ok());
    assert(plan_result.value().validateBasic().is_ok());
}

void test_knowledge_revision_result_ok_dto_validate_basic() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001");
    auto input = makeValidRevisionInput({claim}, 0.75, 0);
    auto result = service.revise(input);
    assert(result.is_ok());
    auto vb = result.value().validateBasic();
    if (vb.is_error()) {
        for (const auto& err : vb.errors()) {
            std::cout << "ERROR: " << (int)err.code << " " << (err.debug_message.has_value() ? err.debug_message.value() : "") << std::endl;
        }
    }
    assert(vb.is_ok());
}

void test_supporting_evidence_reinforces() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.60);
    auto input = makeValidRevisionInput({claim}, 0.75, 0);
    auto result = service.revise(input);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.decision == KnowledgeRevisionDecision::ReinforcedClaim);
    assert(!res.updated_claims.empty());
    assert(res.updated_claims[0].confidence.confidence > 0.60);
}

void test_weak_contradiction_lowers_confidence() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.60);
    auto input = makeValidRevisionInput({claim}, 0.75, 1); // 1 contradiction = weak
    auto result = service.revise(input);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.decision == KnowledgeRevisionDecision::WeakenedClaim);
    assert(!res.updated_claims.empty());
    assert(res.updated_claims[0].confidence.confidence < 0.60);
}

void test_strong_contradiction_disproves() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.40);
    auto input = makeValidRevisionInput({claim}, 0.80, 5); // 5 contradictions = strong, 0.40 - 0.28 = 0.12 <= 0.20
    auto result = service.revise(input);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.decision == KnowledgeRevisionDecision::DisprovenClaim);
    assert(!res.updated_claims.empty());
    assert(res.updated_claims[0].status == KnowledgeStatus::Disproven);
}

void test_condition_mismatch_creates_specialized_claim() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.80);
    auto input = makeValidRevisionInput({claim}, 0.75, 3);
    // Add a condition to the incoming evidence
    KnowledgeCondition cond;
    cond.condition_key = "state_decayed";
    input.formation_input.candidate_conditions.push_back(cond);
    input.revision_tick = Tick(200);

    auto result = service.revise(input);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.decision == KnowledgeRevisionDecision::CreatedSpecializedClaim);
    assert(!res.created_claims.empty());
    assert(!res.updated_claims.empty());
    assert(res.updated_claims[0].status == KnowledgeStatus::Deprecated);
}

void test_no_change_skipped_valid() {
    KnowledgeRevisionService service;
    auto input = makeValidRevisionInput({}, 0.75, 0);
    auto result = service.revise(input);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.decision == KnowledgeRevisionDecision::Skipped);
    assert(res.validateBasic().is_ok());
}

void test_repository_stores_deprecated_disproven() {
    KnowledgeRepository repo;

    // Deprecated claim
    KnowledgeClaim deprecated = makeValidClaim("know_dep", KnowledgeStatus::Deprecated, 0.50);
    deprecated.superseded_by_knowledge_id = KnowledgeId("know_new_001");
    deprecated.reason_keys.push_back("deprecated_by_revision");
    assert(repo.put(deprecated).is_ok());

    // Disproven claim
    KnowledgeClaim disproven = makeValidClaim("know_dis", KnowledgeStatus::Disproven, 0.10);
    disproven.confidence.conflict_count = 3;
    disproven.evidence_refs.push_back(makeValidEvidence(false));
    disproven.reason_keys.push_back("disproven_by_revision");
    assert(repo.put(disproven).is_ok());

    assert(repo.size() == 2);
}

void test_repository_default_query_excludes_deprecated_disproven() {
    KnowledgeRepository repo;
    KnowledgeClaim active = makeValidClaim("know_active", KnowledgeStatus::Active, 0.75);
    assert(repo.put(active).is_ok());

    KnowledgeClaim deprecated = makeValidClaim("know_dep", KnowledgeStatus::Deprecated, 0.50);
    deprecated.superseded_by_knowledge_id = KnowledgeId("know_new_001");
    deprecated.reason_keys.push_back("deprecated_by_revision");
    assert(repo.put(deprecated).is_ok());

    KnowledgeClaim disproven = makeValidClaim("know_dis", KnowledgeStatus::Disproven, 0.10);
    disproven.confidence.conflict_count = 3;
    disproven.evidence_refs.push_back(makeValidEvidence(false));
    disproven.reason_keys.push_back("disproven_by_revision");
    assert(repo.put(disproven).is_ok());

    KnowledgeQuery query;
    query.owner = makeValidOwner();
    query.mode = KnowledgeQueryMode::ByOwner;
    query.limit = 10;

    auto result = repo.query(query);
    assert(result.is_ok());
    auto& claims = result.value();
    assert(claims.size() == 1);
    assert(claims[0].knowledge_id == KnowledgeId("know_active"));
}

void test_repository_include_deprecated_disproven() {
    KnowledgeRepository repo;
    KnowledgeClaim active = makeValidClaim("know_active", KnowledgeStatus::Active, 0.75);
    assert(repo.put(active).is_ok());

    KnowledgeClaim deprecated = makeValidClaim("know_dep", KnowledgeStatus::Deprecated, 0.50);
    deprecated.superseded_by_knowledge_id = KnowledgeId("know_new_001");
    deprecated.reason_keys.push_back("deprecated_by_revision");
    assert(repo.put(deprecated).is_ok());

    KnowledgeClaim disproven = makeValidClaim("know_dis", KnowledgeStatus::Disproven, 0.10);
    disproven.confidence.conflict_count = 3;
    disproven.evidence_refs.push_back(makeValidEvidence(false));
    disproven.reason_keys.push_back("disproven_by_revision");
    assert(repo.put(disproven).is_ok());

    KnowledgeQuery query;
    query.owner = makeValidOwner();
    query.mode = KnowledgeQueryMode::ByOwner;
    query.include_deprecated = true;
    query.include_disproven = true;
    query.limit = 10;

    auto result = repo.query(query);
    assert(result.is_ok());
    auto& claims = result.value();
    assert(claims.size() == 3);
}

void test_projection_builder_skips_deprecated_disproven() {
    KnowledgeClaim active = makeValidClaim("know_active", KnowledgeStatus::Active, 0.75);
    KnowledgeClaim deprecated = makeValidClaim("know_dep", KnowledgeStatus::Deprecated, 0.50);
    deprecated.superseded_by_knowledge_id = KnowledgeId("know_new_001");
    deprecated.reason_keys.push_back("deprecated_by_revision");
    KnowledgeClaim disproven = makeValidClaim("know_dis", KnowledgeStatus::Disproven, 0.10);
    disproven.confidence.conflict_count = 3;
    disproven.evidence_refs.push_back(makeValidEvidence(false));
    disproven.reason_keys.push_back("disproven_by_revision");

    KnowledgeQuery query;
    query.owner = makeValidOwner();
    query.mode = KnowledgeQueryMode::ByOwner;
    query.limit = 10;

    KnowledgeProjectionBuilder builder;
    auto result = builder.buildProjection({active, deprecated, disproven}, query);
    assert(result.is_ok());
    auto& projection = result.value();
    assert(projection.items.size() == 1);
    assert(projection.items[0].knowledge_id == KnowledgeId("know_active"));
    assert(projection.validateBasic().is_ok());
}

void test_hidden_truth_rejected() {
    KnowledgeRevisionInput input;
    input.formation_input = makeValidFormationInput();
    input.formation_input.action_key = "edible_profile";
    input.revision_tick = Tick(200);
    assert(input.validateBasic().is_error());

    input.formation_input.action_key = "eat";
    input.formation_input.effect_key = "hunger_delta";
    assert(input.validateBasic().is_error());

    input.formation_input.effect_key = "restore_hunger";
    input.reason_keys = {"gamestate"};
    assert(input.validateBasic().is_error());
}

void test_test_only_enum_rejected() {
    KnowledgeRevisionOptions opts;
    opts.allow_deprecated = true;
    assert(opts.validateBasic().is_ok());

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
}

void test_invalid_claim_input_returns_fail() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001");
    claim.status = KnowledgeStatus::TestOnly;
    auto input = makeValidRevisionInput({claim});
    auto result = service.revise(input);
    assert(result.is_error());
}

void test_different_effect_without_contradiction_must_not_reinforce() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.75);
    // Incoming effect is poison, not restore_hunger
    auto input = makeValidRevisionInput({claim}, 0.75, 0);
    input.formation_input.effect_key = "poison";
    auto result = service.revise(input);
    assert(result.is_ok());
    auto& res = result.value();
    // Effect mismatch must not reinforce, even with contradiction_count=0
    assert(res.decision != KnowledgeRevisionDecision::ReinforcedClaim);
    assert(res.decision == KnowledgeRevisionDecision::WeakenedClaim ||
           res.decision == KnowledgeRevisionDecision::CreatedSpecializedClaim ||
           res.decision == KnowledgeRevisionDecision::DeprecatedClaim);
}

void test_condition_mismatch_created_claim_uses_incoming_predicate() {
    KnowledgeRevisionService service;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.75);
    auto input = makeValidRevisionInput({claim}, 0.70, 3);
    input.formation_input.effect_key = "poison";
    KnowledgeCondition cond;
    cond.condition_key = "state_decayed";
    input.formation_input.candidate_conditions.push_back(cond);
    input.revision_tick = Tick(200);

    auto result = service.revise(input);
    assert(result.is_ok());
    auto& res = result.value();
    assert(res.decision == KnowledgeRevisionDecision::CreatedSpecializedClaim);
    assert(!res.created_claims.empty());
    assert(res.created_claims[0].predicate.effect_key == "poison");
    assert(!res.updated_claims.empty());
    assert(res.updated_claims[0].status == KnowledgeStatus::Deprecated);
    assert(!res.updated_claims[0].superseded_by_knowledge_id.empty());
}

void test_incoming_predicate_recorded_in_conflict_signal() {
    KnowledgeRevisionPlanner planner;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.75);
    auto input = makeValidRevisionInput({claim}, 0.70, 3);
    input.formation_input.effect_key = "poison";
    auto plan_result = planner.planRevision(input);
    assert(plan_result.is_ok());
    auto& plan = plan_result.value();
    assert(!plan.assessment.conflict_signals.empty());
    assert(plan.assessment.conflict_signals[0].incoming_predicate.effect_key == "poison");
    assert(plan.assessment.conflict_signals[0].existing_predicate.effect_key == "restore_hunger");
}

void test_opposite_effect_classification() {
    KnowledgeRevisionPlanner planner;
    KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.75);
    auto input = makeValidRevisionInput({claim}, 0.70, 0); // contradiction_count=0
    input.formation_input.effect_key = "poison";
    auto plan_result = planner.planRevision(input);
    assert(plan_result.is_ok());
    auto& plan = plan_result.value();
    assert(!plan.assessment.conflict_signals.empty());
    assert(plan.assessment.conflict_signals[0].conflict_type == KnowledgeConflictType::OppositeEffect);
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    test_knowledge_revision_options_validate_basic();
    test_knowledge_conflict_signal_validate_basic();
    test_knowledge_revision_input_owner_mismatch_rejected();
    test_knowledge_revision_input_invalid_existing_claim_rejected();
    test_knowledge_revision_assessment_invalid_enum_rejected();
    test_knowledge_claim_patch_disproven_requires_negative_evidence();
    test_knowledge_revision_plan_ok_dto_validate_basic();
    test_knowledge_revision_result_ok_dto_validate_basic();
    test_supporting_evidence_reinforces();
    test_weak_contradiction_lowers_confidence();
    test_strong_contradiction_disproves();
    test_condition_mismatch_creates_specialized_claim();
    test_no_change_skipped_valid();
    test_repository_stores_deprecated_disproven();
    test_repository_default_query_excludes_deprecated_disproven();
    test_repository_include_deprecated_disproven();
    test_projection_builder_skips_deprecated_disproven();
    test_hidden_truth_rejected();
    test_test_only_enum_rejected();
    test_invalid_claim_input_returns_fail();
    test_different_effect_without_contradiction_must_not_reinforce();
    test_condition_mismatch_created_claim_uses_incoming_predicate();
    test_incoming_predicate_recorded_in_conflict_signal();
    test_opposite_effect_classification();

    std::cout << "knowledge_revision tests passed" << std::endl;
    return 0;
}
