#include "pathfinder/knowledge/knowledge_revision.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_formation.h"
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

static KnowledgeSubject makeValidSubject(const std::string& id = "berry_red") {
    KnowledgeSubject subject;
    subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = id;
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

void run_knowledge_revision_flow_tests() {
    // ============================================================
    // Full flow: reinforce existing claim
    // ============================================================
    {
        KnowledgeRepository repo;
        KnowledgeClaim claim = makeValidClaim("know_001", KnowledgeStatus::Active, 0.60);
        assert(repo.put(claim).is_ok());

        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput({claim}, 0.75, 0);
        auto result = service.revise(input);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == KnowledgeRevisionDecision::ReinforcedClaim);
        assert(!res.updated_claims.empty());

        // Apply to snapshot
        KnowledgeRevisionApplier applier;
        auto snapshot_result = applier.applyToSnapshot({claim}, res);
        assert(snapshot_result.is_ok());
        auto& snapshot = snapshot_result.value();
        assert(snapshot.size() == 1);
        assert(snapshot[0].confidence.confidence > 0.60);
    }

    // ============================================================
    // Full flow: weak contradiction -> weaken
    // ============================================================
    {
        KnowledgeClaim claim = makeValidClaim("know_002", KnowledgeStatus::Active, 0.70);
        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput({claim}, 0.60, 1);
        auto result = service.revise(input);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == KnowledgeRevisionDecision::WeakenedClaim);
        assert(!res.updated_claims.empty());
        assert(res.updated_claims[0].confidence.confidence < 0.70);
        assert(res.updated_claims[0].confidence.conflict_count > 0);
    }

    // ============================================================
    // Full flow: strong contradiction -> disprove
    // ============================================================
    {
        KnowledgeClaim claim = makeValidClaim("know_003", KnowledgeStatus::Active, 0.40);
        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput({claim}, 0.80, 5); // 0.40 - 0.28 = 0.12 <= 0.20
        auto result = service.revise(input);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == KnowledgeRevisionDecision::DisprovenClaim);
        assert(!res.updated_claims.empty());
        assert(res.updated_claims[0].status == KnowledgeStatus::Disproven);
        assert(!res.event_drafts.empty());
        assert(res.state_changes.size() == 1);
    }

    // ============================================================
    // Full flow: condition mismatch -> specialize + deprecate old
    // ============================================================
    {
        KnowledgeClaim claim = makeValidClaim("know_004", KnowledgeStatus::Teachable, 0.80);
        claim.teaching_profile.teachable = true;
        claim.teaching_profile.teaching_message_key = "msg";
        claim.projection_flags.usable_for_teaching = true;

        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput({claim}, 0.70, 3);
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
        assert(!res.updated_claims[0].superseded_by_knowledge_id.empty() ==
               !res.created_claims.empty());
    }

    // ============================================================
    // Full flow: no matching claims -> skipped
    // ============================================================
    {
        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput({}, 0.75, 0);
        auto result = service.revise(input);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision == KnowledgeRevisionDecision::Skipped);
        assert(res.validateBasic().is_ok());
    }

    // ============================================================
    // Full flow: repository stores and queries with filters
    // ============================================================
    {
        KnowledgeRepository repo;
        KnowledgeClaim active = makeValidClaim("know_active", KnowledgeStatus::Active, 0.75);
        assert(repo.put(active).is_ok());

        KnowledgeClaim deprecated = makeValidClaim("know_dep", KnowledgeStatus::Deprecated, 0.50);
        deprecated.superseded_by_knowledge_id = KnowledgeId("know_new");
        deprecated.reason_keys.push_back("deprecated_by_revision");
        assert(repo.put(deprecated).is_ok());

        KnowledgeClaim disproven = makeValidClaim("know_dis", KnowledgeStatus::Disproven, 0.10);
        disproven.confidence.conflict_count = 3;
        disproven.evidence_refs.push_back(makeValidEvidence(false));
        disproven.reason_keys.push_back("disproven_by_revision");
        assert(repo.put(disproven).is_ok());

        // Default query excludes both
        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.limit = 10;
        auto qresult = repo.query(query);
        assert(qresult.is_ok());
        assert(qresult.value().size() == 1);
        assert(qresult.value()[0].knowledge_id == KnowledgeId("know_active"));

        // Include deprecated
        query.include_deprecated = true;
        qresult = repo.query(query);
        assert(qresult.is_ok());
        assert(qresult.value().size() == 2);

        // Include both
        query.include_disproven = true;
        qresult = repo.query(query);
        assert(qresult.is_ok());
        assert(qresult.value().size() == 3);
    }

    // ============================================================
    // Full flow: projection builder never outputs deprecated/disproven
    // ============================================================
    {
        KnowledgeClaim active = makeValidClaim("know_a", KnowledgeStatus::Active, 0.75);
        KnowledgeClaim deprecated = makeValidClaim("know_d1", KnowledgeStatus::Deprecated, 0.50);
        deprecated.superseded_by_knowledge_id = KnowledgeId("know_new");
        deprecated.reason_keys.push_back("deprecated_by_revision");
        KnowledgeClaim disproven = makeValidClaim("know_d2", KnowledgeStatus::Disproven, 0.10);
        disproven.confidence.conflict_count = 3;
        disproven.evidence_refs.push_back(makeValidEvidence(false));
        disproven.reason_keys.push_back("disproven_by_revision");

        KnowledgeQuery query;
        query.owner = makeValidOwner();
        query.mode = KnowledgeQueryMode::ByOwner;
        query.limit = 10;

        KnowledgeProjectionBuilder builder;
        auto presult = builder.buildProjection({active, deprecated, disproven}, query);
        assert(presult.is_ok());
        auto& proj = presult.value();
        assert(proj.items.size() == 1);
        assert(proj.items[0].knowledge_id == KnowledgeId("know_a"));
        assert(proj.validateBasic().is_ok());
    }

    // ============================================================
    // Full flow: different effect without contradiction -> must not reinforce
    // ============================================================
    {
        KnowledgeClaim claim = makeValidClaim("know_diff", KnowledgeStatus::Active, 0.75);
        KnowledgeRevisionService service;
        auto input = makeValidRevisionInput({claim}, 0.75, 0);
        input.formation_input.effect_key = "poison";
        auto result = service.revise(input);
        assert(result.is_ok());
        auto& res = result.value();
        assert(res.decision != KnowledgeRevisionDecision::ReinforcedClaim);
        assert(!res.updated_claims.empty());
        assert(res.updated_claims[0].confidence.confidence < 0.75);
    }

    // ============================================================
    // Full flow: condition mismatch with different effect -> specialized claim uses poison
    // ============================================================
    {
        KnowledgeClaim claim = makeValidClaim("know_spec", KnowledgeStatus::Active, 0.75);
        KnowledgeRevisionService service;
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
        assert(res.created_claims[0].supersedes_knowledge_ids[0] == claim.knowledge_id);
    }

    std::cout << "knowledge_revision_flow tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_revision_flow_tests();
    return 0;
}
