#include "pathfinder/learning/learning_loop.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::learning;
using namespace pathfinder::foundation;
using namespace pathfinder::cognition;
using namespace pathfinder::memory;
using namespace pathfinder::knowledge;

// ============================================================
// Helpers
// ============================================================

static LearningActorRef makeActor(const std::string& id) {
    LearningActorRef actor;
    actor.knowledge_owner.kind = KnowledgeOwnerKind::Agent;
    actor.knowledge_owner.entity_id = EntityId(id);
    actor.memory_owner.kind = MemoryOwnerKind::Agent;
    actor.memory_owner.entity_id = EntityId(id);
    actor.cognition_subject.kind = CognitionSubjectKind::Actor;
    actor.cognition_subject.subject_id = EntityId(id);
    actor.display_key = id;
    return actor;
}

static LearningSafeFeedbackInput makeFeedback(const std::string& subject_id) {
    LearningSafeFeedbackInput fb;
    fb.feedback_key = "fb_eat_" + subject_id;
    fb.cognition_target.kind = CognitionTargetKind::ObjectDefinition;
    fb.cognition_target.target_id = subject_id;
    fb.cognition_target.public_label_key = subject_id;
    fb.memory_subject.kind = MemorySubjectKind::ObjectDefinition;
    fb.memory_subject.subject_id = subject_id;
    fb.knowledge_subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    fb.knowledge_subject.subject_id = subject_id;
    fb.action_context = CognitionActionContextKind::Eat;
    fb.action_id = ActionId("act_eat");
    fb.action_key = "eat";
    fb.outcome_signals = {CognitionOutcomeSignal::NeedImproved};
    fb.effect_key = "restore_hunger";
    fb.utility_delta = 0.5;
    fb.risk_delta = 0.0;
    fb.observed_tick = Tick(10);
    fb.source_event_ids = {EventId("evt_001")};
    fb.reason_keys = {"direct_experience"};
    return fb;
}

static LearningLoopInput makeInput(const std::string& loop_key,
                                    const std::string& actor_id,
                                    const std::string& subject_id,
                                    LearningLoopStoryKind kind = LearningLoopStoryKind::DirectDiscovery) {
    LearningLoopInput input;
    input.loop_key = loop_key;
    input.story_kind = kind;
    input.actor = makeActor(actor_id);
    input.feedback = makeFeedback(subject_id);
    input.loop_tick = Tick(10);
    input.reason_keys = {"test_story"};
    return input;
}

// ============================================================
// Enum tests
// ============================================================

static void test_enum_roundtrip() {
    auto r1 = learningLoopStoryKindFromString("direct_discovery");
    assert(r1.is_ok());
    assert(r1.value() == LearningLoopStoryKind::DirectDiscovery);
    assert(toString(r1.value()) == "direct_discovery");

    auto r2 = learningLoopStageFromString("cognition_updated");
    assert(r2.is_ok());
    assert(r2.value() == LearningLoopStage::CognitionUpdated);

    auto r3 = learningLoopDecisionFromString("completed");
    assert(r3.is_ok());
    assert(r3.value() == LearningLoopDecision::Completed);

    auto r4 = learningLoopFailureReasonFromString("invalid_input");
    assert(r4.is_ok());
    assert(r4.value() == LearningLoopFailureReason::InvalidInput);

    auto r5 = learningStepDecisionFromString("executed");
    assert(r5.is_ok());
    assert(r5.value() == LearningStepDecision::Executed);

    auto bad = learningLoopStoryKindFromString("not_a_kind");
    assert(bad.is_error());

    std::cout << "enum_roundtrip passed" << std::endl;
}

// ============================================================
// DTO validation tests
// ============================================================

static void test_options_validateBasic() {
    LearningLoopOptions opts;
    auto r = opts.validateBasic();
    assert(r.is_ok());

    opts.max_step_traces = 0;
    r = opts.validateBasic();
    assert(r.is_error());
    opts.max_step_traces = 64;

    opts.max_report_items = 0;
    r = opts.validateBasic();
    assert(r.is_error());

    std::cout << "options_validateBasic passed" << std::endl;
}

static void test_actor_ref_validateBasic() {
    auto actor = makeActor("agent_a");
    auto r = actor.validateBasic();
    assert(r.is_ok());

    LearningActorRef bad;
    bad.display_key = "hidden_truth_test";
    r = bad.validateBasic();
    assert(r.is_error());

    std::cout << "actor_ref_validateBasic passed" << std::endl;
}

static void test_feedback_validateBasic() {
    auto fb = makeFeedback("red_berry");
    auto r = fb.validateBasic();
    assert(r.is_ok());

    fb.effect_key = "hidden_truth";
    r = fb.validateBasic();
    assert(r.is_error());

    std::cout << "feedback_validateBasic passed" << std::endl;
}

static void test_input_validateBasic() {
    auto input = makeInput("loop_001", "agent_a", "red_berry");
    auto r = input.validateBasic();
    assert(r.is_ok());

    input.story_kind = LearningLoopStoryKind::Unknown;
    r = input.validateBasic();
    assert(r.is_error());
    input.story_kind = LearningLoopStoryKind::DirectDiscovery;

    input.loop_key = "raw_state_key";
    r = input.validateBasic();
    assert(r.is_error());

    std::cout << "input_validateBasic passed" << std::endl;
}

static void test_trace_validateBasic() {
    LearningLoopStepTrace trace;
    trace.stage = LearningLoopStage::CognitionUpdated;
    trace.decision = LearningStepDecision::Executed;
    trace.failure_reason = LearningLoopFailureReason::None;
    auto r = trace.validateBasic();
    assert(r.is_ok());

    trace.stage = LearningLoopStage::Unknown;
    r = trace.validateBasic();
    assert(r.is_error());

    std::cout << "trace_validateBasic passed" << std::endl;
}

static void test_result_validateBasic() {
    LearningLoopResult res;
    res.decision = LearningLoopDecision::Completed;
    res.failure_reason = LearningLoopFailureReason::None;
    auto r = res.validateBasic();
    assert(r.is_ok());

    res.decision = LearningLoopDecision::Unknown;
    r = res.validateBasic();
    assert(r.is_error());

    std::cout << "result_validateBasic passed" << std::endl;
}

static void test_report_validateBasic() {
    LearningDebugReport report;
    report.report_key = "rep_001";
    report.loop_key = "loop_001";
    report.decision = LearningLoopDecision::Completed;
    report.failure_reason = LearningLoopFailureReason::None;
    auto r = report.validateBasic();
    assert(r.is_ok());

    report.report_key = "hidden_truth";
    r = report.validateBasic();
    assert(r.is_error());

    std::cout << "report_validateBasic passed" << std::endl;
}

static void test_unknown_testonly_rejected() {
    LearningLoopInput input;
    input.loop_key = "test";
    input.story_kind = LearningLoopStoryKind::TestOnly;
    input.actor = makeActor("a");
    input.feedback = makeFeedback("x");
    input.loop_tick = Tick(1);
    auto r = input.validateBasic();
    assert(r.is_error());

    input.story_kind = LearningLoopStoryKind::Unknown;
    r = input.validateBasic();
    assert(r.is_error());

    std::cout << "unknown_testonly_rejected passed" << std::endl;
}

static void test_forbidden_key_rejected() {
    auto input = makeInput("loop_001", "agent_a", "red_berry");
    input.feedback.reason_keys.push_back("hidden_truth_detected");
    auto r = input.validateBasic();
    assert(r.is_error());

    std::cout << "forbidden_key_rejected passed" << std::endl;
}

static void test_ok_dto_must_validate() {
    auto input = makeInput("loop_ok", "agent_a", "red_berry");
    auto r = input.validateBasic();
    assert(r.is_ok());

    std::cout << "ok_dto_must_validate passed" << std::endl;
}

// ============================================================
// Planner tests
// ============================================================

static void test_planner_skips_propagation_when_disabled() {
    LearningLoopPlanner planner;
    auto input = makeInput("loop_plan", "agent_a", "red_berry");
    input.recipient = makeActor("agent_b");

    LearningLoopOptions opts;
    opts.enable_knowledge_propagation = false;

    auto plan_r = planner.plan(input, opts);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();

    bool propagation_skipped = false;
    for (const auto& t : plan) {
        if (t.stage == LearningLoopStage::KnowledgePropagated &&
            t.decision == LearningStepDecision::SkippedByOptions) {
            propagation_skipped = true;
        }
    }
    assert(propagation_skipped);

    std::cout << "planner_skips_propagation_when_disabled passed" << std::endl;
}

static void test_planner_skips_propagation_without_recipient() {
    LearningLoopPlanner planner;
    auto input = makeInput("loop_plan2", "agent_a", "red_berry");
    // no recipient

    auto plan_r = planner.plan(input);
    assert(plan_r.is_ok());
    auto plan = plan_r.value();

    bool propagation_skipped_no_input = false;
    for (const auto& t : plan) {
        if (t.stage == LearningLoopStage::KnowledgePropagated &&
            t.decision == LearningStepDecision::SkippedNoInput) {
            propagation_skipped_no_input = true;
        }
    }
    assert(propagation_skipped_no_input);

    std::cout << "planner_skips_propagation_without_recipient passed" << std::endl;
}

// ============================================================
// Service basic tests
// ============================================================

static void test_service_runs_direct_discovery() {
    LearningLoopService service;
    auto input = makeInput("loop_direct", "agent_a", "red_berry", LearningLoopStoryKind::DirectDiscovery);

    LearningLoopOptions opts;
    opts.enable_memory_compression = false;
    opts.enable_memory_recall = false;
    opts.enable_knowledge_formation = false;
    opts.enable_knowledge_revision = false;
    opts.enable_knowledge_propagation = false;
    opts.enable_recipient_projection_check = false;

    auto result_r = service.run(input, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.ok());
    assert(result.decision == LearningLoopDecision::Completed || result.decision == LearningLoopDecision::PartialCompleted);

    bool has_cognition = false;
    for (const auto& t : result.traces) {
        if (t.stage == LearningLoopStage::CognitionUpdated && t.decision == LearningStepDecision::Executed) {
            has_cognition = true;
        }
    }
    assert(has_cognition);

    std::cout << "service_runs_direct_discovery passed" << std::endl;
}

static void test_service_runs_with_recipient() {
    LearningLoopService service;
    auto input = makeInput("loop_teach", "agent_a", "red_berry", LearningLoopStoryKind::TeachingToRecipient);
    input.recipient = makeActor("agent_b");

    // Pre-seed actor with an existing claim so propagation has something to transfer
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId("know_a_001");
    claim.owner = input.actor.knowledge_owner;
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = "red_berry";
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = "eat";
    claim.predicate.effect_key = "restore_hunger";
    claim.confidence.confidence = 0.80;
    claim.confidence.band = KnowledgeConfidenceBand::Strong;
    claim.status = KnowledgeStatus::Active;
    claim.created_tick = Tick(5);
    claim.updated_tick = Tick(5);
    claim.evidence_refs.push_back(KnowledgeEvidence{});
    claim.evidence_refs.back().evidence_id = KnowledgeEvidenceId("kevd_001");
    claim.evidence_refs.back().evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    claim.evidence_refs.back().source_summary_id = MemorySummaryId("sum_001");
    claim.evidence_refs.back().supports_claim = true;
    claim.evidence_refs.back().reliability = 0.9;
    claim.evidence_refs.back().confidence_delta = 0.5;
    claim.evidence_refs.back().observed_tick = Tick(5);
    input.actor_existing_claims.push_back(claim);

    LearningLoopOptions opts;
    opts.enable_memory_compression = false;
    opts.enable_memory_recall = false;
    opts.enable_knowledge_formation = false;
    opts.enable_knowledge_revision = false;
    opts.require_full_chain = false;

    auto result_r = service.run(input, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.ok());

    bool has_propagation = false;
    for (const auto& t : result.traces) {
        if (t.stage == LearningLoopStage::KnowledgePropagated && t.decision == LearningStepDecision::Executed) {
            has_propagation = true;
        }
    }
    assert(has_propagation);

    // Verify recipient got a claim
    bool recipient_has_claim = false;
    for (const auto& c : result.recipient_claim_snapshot_after) {
        if (c.owner.entity_id.value() == "agent_b") {
            recipient_has_claim = true;
            // Must have Teaching evidence
            bool has_teaching = false;
            for (const auto& ev : c.evidence_refs) {
                if (ev.evidence_kind == KnowledgeEvidenceKind::Teaching) {
                    has_teaching = true;
                    break;
                }
            }
            assert(has_teaching);
        }
    }
    assert(recipient_has_claim);

    std::cout << "service_runs_with_recipient passed" << std::endl;
}

static void test_applier_actor_claims() {
    LearningLoopApplier applier;
    std::vector<KnowledgeClaim> current;

    LearningLoopResult result;
    result.decision = LearningLoopDecision::Completed;
    result.failure_reason = LearningLoopFailureReason::None;

    KnowledgeFormationResult form_res;
    form_res.decision = KnowledgeFormationDecision::CreatedClaim;
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId("know_new_001");
    claim.owner.kind = KnowledgeOwnerKind::Agent;
    claim.owner.entity_id = EntityId("agent_a");
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = "red_berry";
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = "eat";
    claim.predicate.effect_key = "restore_hunger";
    claim.confidence.confidence = 0.6;
    claim.confidence.band = KnowledgeConfidenceBand::Reliable;
    claim.status = KnowledgeStatus::Active;
    claim.created_tick = Tick(10);
    claim.updated_tick = Tick(10);
    claim.evidence_refs.push_back(KnowledgeEvidence{});
    claim.evidence_refs.back().evidence_id = KnowledgeEvidenceId("kevd_001");
    claim.evidence_refs.back().evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    claim.evidence_refs.back().source_summary_id = MemorySummaryId("sum_001");
    claim.evidence_refs.back().supports_claim = true;
    claim.evidence_refs.back().reliability = 0.8;
    claim.evidence_refs.back().confidence_delta = 0.4;
    claim.evidence_refs.back().observed_tick = Tick(10);
    form_res.claim = claim;
    result.knowledge_formation_result = form_res;

    auto apply_r = applier.applyActorClaims(current, result);
    assert(apply_r.is_ok());
    auto snapshot = apply_r.value();
    assert(snapshot.size() == 1);
    assert(snapshot[0].knowledge_id.value() == "know_new_001");

    std::cout << "applier_actor_claims passed" << std::endl;
}

static void test_report_builder() {
    LearningLoopInput input = makeInput("loop_rep", "agent_a", "red_berry");
    LearningLoopResult result;
    result.decision = LearningLoopDecision::Completed;
    result.failure_reason = LearningLoopFailureReason::None;
    LearningLoopStepTrace t1;
    t1.stage = LearningLoopStage::FeedbackCaptured;
    t1.decision = LearningStepDecision::Executed;
    t1.failure_reason = LearningLoopFailureReason::None;
    result.traces.push_back(t1);
    LearningLoopStepTrace t2;
    t2.stage = LearningLoopStage::CognitionUpdated;
    t2.decision = LearningStepDecision::Executed;
    t2.failure_reason = LearningLoopFailureReason::None;
    result.traces.push_back(t2);

    LearningDebugReportBuilder builder;
    auto report_r = builder.buildReport(input, result);
    assert(report_r.is_ok());
    auto report = report_r.value();
    assert(report.loop_key == "loop_rep");
    assert(!report.timeline_keys.empty());

    std::cout << "report_builder passed" << std::endl;
}

// ============================================================
// Main
// ============================================================

int main(int argc, char* argv[]) {
    test_enum_roundtrip();
    test_options_validateBasic();
    test_actor_ref_validateBasic();
    test_feedback_validateBasic();
    test_input_validateBasic();
    test_trace_validateBasic();
    test_result_validateBasic();
    test_report_validateBasic();
    test_unknown_testonly_rejected();
    test_forbidden_key_rejected();
    test_ok_dto_must_validate();
    test_planner_skips_propagation_when_disabled();
    test_planner_skips_propagation_without_recipient();
    test_service_runs_direct_discovery();
    test_service_runs_with_recipient();
    test_applier_actor_claims();
    test_report_builder();

    std::cout << "All learning_loop unit tests passed" << std::endl;
    return 0;
}
