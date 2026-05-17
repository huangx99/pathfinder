#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_repository.h"
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

static KnowledgeEvidence makeValidEvidence() {
    KnowledgeEvidence ev;
    ev.evidence_id = KnowledgeEvidenceId("kevd_test_001");
    ev.evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    ev.source_summary_id = MemorySummaryId("sum_001");
    ev.supports_claim = true;
    ev.confidence_delta = 0.5;
    ev.reliability = 0.9;
    return ev;
}

static KnowledgeConfidence makeValidConfidence() {
    KnowledgeConfidence conf;
    conf.confidence = 0.75;
    conf.band = KnowledgeConfidenceBand::Strong;
    conf.support_count = 3;
    return conf;
}

static KnowledgeTeachingProfile makeValidTeachingProfile() {
    KnowledgeTeachingProfile tp;
    tp.teachable = true;
    tp.teaching_message_key = "msg_eat_berry";
    tp.required_confidence = 0.75;
    return tp;
}

static KnowledgeProjectionFlags makeValidProjectionFlags() {
    KnowledgeProjectionFlags pf;
    pf.visible_to_player = true;
    pf.usable_by_ai = true;
    pf.usable_for_action = true;
    return pf;
}

static KnowledgeClaim makeValidClaim() {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId("know_agent_001_berry_red_has_effect_10_0");
    claim.owner = makeValidOwner();
    claim.subject = makeValidSubject();
    claim.predicate = makeValidPredicate();
    claim.confidence = makeValidConfidence();
    claim.status = KnowledgeStatus::Active;
    claim.evidence_refs = {makeValidEvidence()};
    claim.teaching_profile = makeValidTeachingProfile();
    claim.projection_flags = makeValidProjectionFlags();
    claim.created_tick = Tick(10);
    claim.updated_tick = Tick(10);
    return claim;
}

static MemorySummary makeValidSummary() {
    MemorySummary summary;
    summary.summary_id = MemorySummaryId("sum_001");
    summary.key.owner.kind = MemoryOwnerKind::Agent;
    summary.key.owner.entity_id = EntityId("agent_001");
    summary.key.subject.kind = MemorySubjectKind::ObjectDefinition;
    summary.key.subject.subject_id = "berry_red";
    summary.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
    summary.compression_level = MemoryCompressionLevel::LightSummary;
    summary.highest_importance = MemoryImportance::Normal;
    summary.aggregate_strength = 0.75;
    summary.source_memory_count = 3;
    summary.source_memory_ids = {MemoryId("mem_001"), MemoryId("mem_002"), MemoryId("mem_003")};
    summary.representative_memory_ids = {MemoryId("mem_001")};
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = EventId("evt_001");
    summary.evidence_refs = {ref};
    summary.has_long_term_source = true;
    summary.teaching_candidate_count = 2;
    summary.summarized_tick = Tick(100);
    return summary;
}

void run_knowledge_claim_tests() {
    // ============================================================
    // KnowledgeOwner validateBasic
    // ============================================================
    {
        auto owner = makeValidOwner();
        assert(owner.validateBasic().is_ok());

        owner.kind = KnowledgeOwnerKind::Unknown;
        assert(owner.validateBasic().is_error());

        owner = makeValidOwner();
        owner.entity_id = EntityId();
        assert(owner.validateBasic().is_error());

        owner.kind = KnowledgeOwnerKind::Tribe;
        owner.entity_id = EntityId("agent_001");
        assert(owner.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeSubject hidden truth guard
    // ============================================================
    {
        auto subject = makeValidSubject();
        assert(subject.validateBasic().is_ok());

        subject.subject_id = "edible_profile";
        assert(subject.validateBasic().is_error());

        subject = makeValidSubject();
        subject.subject_type_key = "hunger_delta";
        assert(subject.validateBasic().is_error());

        subject = makeValidSubject();
        subject.safe_tags = {"hidden_truth"};
        assert(subject.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgePredicate hidden truth guard
    // ============================================================
    {
        auto pred = makeValidPredicate();
        assert(pred.validateBasic().is_ok());

        pred.effect_key = "edible_profile";
        assert(pred.validateBasic().is_error());

        pred = makeValidPredicate();
        pred.value_summary_key = "health_delta";
        assert(pred.validateBasic().is_error());

        pred = makeValidPredicate();
        pred.action_id = ActionId();
        pred.action_key = "";
        assert(pred.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeEvidence requires source trace
    // ============================================================
    {
        auto ev = makeValidEvidence();
        assert(ev.validateBasic().is_ok());

        ev.source_summary_id = MemorySummaryId();
        ev.source_memory_ids.clear();
        ev.memory_evidence_refs.clear();
        assert(ev.validateBasic().is_error());

        ev = makeValidEvidence();
        ev.evidence_kind = KnowledgeEvidenceKind::Unknown;
        assert(ev.validateBasic().is_error());

        ev = makeValidEvidence();
        ev.confidence_delta = 1.5;
        assert(ev.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeConfidence range and band
    // ============================================================
    {
        auto conf = makeValidConfidence();
        assert(conf.validateBasic().is_ok());

        conf.confidence = 1.5;
        assert(conf.validateBasic().is_error());

        conf.confidence = 0.75;
        conf.band = KnowledgeConfidenceBand::Strong;
        assert(conf.validateBasic().is_ok());
    }

    // ============================================================
    // KnowledgeTeachingProfile
    // ============================================================
    {
        auto tp = makeValidTeachingProfile();
        assert(tp.validateBasic().is_ok());

        tp.teachable = true;
        tp.teaching_message_key = "";
        assert(tp.validateBasic().is_error());

        tp = makeValidTeachingProfile();
        tp.required_confidence = 1.5;
        assert(tp.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeProjectionFlags
    // ============================================================
    {
        auto pf = makeValidProjectionFlags();
        assert(pf.validateBasic().is_ok());

        pf.debug_only = true;
        pf.usable_for_action = true;
        assert(pf.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeClaim validateBasic success/failure
    // ============================================================
    {
        auto claim = makeValidClaim();
        assert(claim.validateBasic().is_ok());

        claim.knowledge_id = KnowledgeId();
        assert(claim.validateBasic().is_error());

        claim = makeValidClaim();
        claim.status = KnowledgeStatus::Unknown;
        assert(claim.validateBasic().is_error());

        claim = makeValidClaim();
        claim.status = KnowledgeStatus::Deprecated;
        assert(claim.validateBasic().is_error());

        claim = makeValidClaim();
        claim.status = KnowledgeStatus::Active;
        claim.evidence_refs.clear();
        assert(claim.validateBasic().is_error());

        claim = makeValidClaim();
        claim.status = KnowledgeStatus::Teachable;
        claim.teaching_profile.teachable = false;
        assert(claim.validateBasic().is_error());

        claim = makeValidClaim();
        claim.reason_keys = {"edible_profile"};
        assert(claim.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeFormationOptions
    // ============================================================
    {
        KnowledgeFormationOptions opts;
        assert(opts.validateBasic().is_ok());

        opts.min_source_memory_count = 0;
        assert(opts.validateBasic().is_error());

        opts = KnowledgeFormationOptions();
        opts.max_evidence_refs = 0;
        assert(opts.validateBasic().is_error());

        opts = KnowledgeFormationOptions();
        opts.teachable_confidence_threshold = 0.5;
        opts.active_confidence_threshold = 0.6;
        assert(opts.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeFormationInput
    // ============================================================
    {
        KnowledgeFormationInput input;
        input.owner = makeValidOwner();
        input.summary = makeValidSummary();
        input.target_relation = KnowledgeRelationType::HasEffect;
        input.action_key = "eat";
        assert(input.validateBasic().is_ok());

        input.action_key = "edible_profile";
        assert(input.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeFormationPlan
    // ============================================================
    {
        KnowledgeFormationPlan plan;
        plan.plan_key = "plan_001";
        plan.input = []() {
            KnowledgeFormationInput i;
            i.owner = makeValidOwner();
            i.summary = makeValidSummary();
            i.target_relation = KnowledgeRelationType::HasEffect;
            i.action_key = "eat";
            return i;
        }();
        plan.subject = makeValidSubject();
        plan.predicate = makeValidPredicate();
        plan.evidence_refs = {makeValidEvidence()};
        plan.projected_confidence = makeValidConfidence();
        plan.projected_status = KnowledgeStatus::Active;
        assert(plan.validateBasic().is_ok());

        plan.evidence_refs.clear();
        assert(plan.validateBasic().is_error());
    }

    // ============================================================
    // KnowledgeFormationResult
    // ============================================================
    {
        KnowledgeFormationResult result;
        result.decision = KnowledgeFormationDecision::Skipped;
        assert(result.ok());
        assert(result.validateBasic().is_ok());

        result.decision = KnowledgeFormationDecision::CreatedClaim;
        assert(!result.validateBasic().is_ok());

        result.claim = makeValidClaim();
        assert(result.validateBasic().is_ok());
    }

    // ============================================================
    // KnowledgeIdFactory deterministic
    // ============================================================
    {
        KnowledgeIdFactory factory;
        auto id1 = factory.makeKnowledgeId(makeValidOwner(), makeValidSubject(), makeValidPredicate(), Tick(10), 0);
        auto id2 = factory.makeKnowledgeId(makeValidOwner(), makeValidSubject(), makeValidPredicate(), Tick(10), 0);
        assert(id1.is_ok());
        assert(id2.is_ok());
        assert(id1.value().value() == id2.value().value());

        auto id3 = factory.makeKnowledgeId(makeValidOwner(), makeValidSubject(), makeValidPredicate(), Tick(10), 1);
        assert(id3.is_ok());
        assert(id1.value().value() != id3.value().value());

        auto ev_id = factory.makeEvidenceId(id1.value(), 0);
        assert(ev_id.is_ok());
        assert(!ev_id.value().empty());

        auto trace_id = factory.makeTraceId(id1.value(), 0);
        assert(trace_id.is_ok());
        assert(!trace_id.value().empty());
    }

    // ============================================================
    // KnowledgeQuery
    // ============================================================
    {
        KnowledgeQuery q;
        q.owner = makeValidOwner();
        q.mode = KnowledgeQueryMode::ByOwner;
        assert(q.validateBasic().is_ok());

        q.limit = 0;
        assert(q.validateBasic().is_error());
    }

    std::cout << "knowledge_claim tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_claim_tests();
    return 0;
}
