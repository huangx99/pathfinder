#include "pathfinder/learning/learning_loop.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::learning;
using namespace pathfinder::foundation;
using namespace pathfinder::cognition;
using namespace pathfinder::memory;
using namespace pathfinder::knowledge;

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

static LearningSafeFeedbackInput makeEatFeedback(const std::string& subject_id,
                                                  const std::string& effect = "restore_hunger") {
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
    fb.effect_key = effect;
    fb.utility_delta = (effect == "poison") ? -0.7 : 0.5;
    fb.risk_delta = (effect == "poison") ? 0.8 : 0.0;
    fb.observed_tick = Tick(10);
    fb.source_event_ids = {EventId("evt_001")};
    fb.reason_keys = {"direct_experience"};
    return fb;
}

static KnowledgeClaim makeClaim(const std::string& id, const KnowledgeOwner& owner,
                                 const std::string& subject_id,
                                 const std::string& effect = "restore_hunger",
                                 KnowledgeStatus status = KnowledgeStatus::Active) {
    KnowledgeClaim claim;
    claim.knowledge_id = KnowledgeId(id);
    claim.owner = owner;
    claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = subject_id;
    claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = "eat";
    claim.predicate.effect_key = effect;
    claim.confidence.confidence = 0.80;
    claim.confidence.band = KnowledgeConfidenceBand::Strong;
    claim.status = status;
    claim.created_tick = Tick(5);
    claim.updated_tick = Tick(5);
    claim.evidence_refs.push_back(KnowledgeEvidence{});
    claim.evidence_refs.back().evidence_id = KnowledgeEvidenceId("kevd_" + id);
    claim.evidence_refs.back().evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    claim.evidence_refs.back().source_summary_id = MemorySummaryId("sum_" + id);
    claim.evidence_refs.back().supports_claim = true;
    claim.evidence_refs.back().reliability = 0.9;
    claim.evidence_refs.back().confidence_delta = 0.5;
    claim.evidence_refs.back().observed_tick = Tick(5);
    return claim;
}

// Story: A knows red_berry + eat -> restore_hunger.
// A gets new feedback: red_berry + eat -> poison (decayed condition).
// P19 should revise A's knowledge.
// A teaches B. B already has red_berry + eat -> restore_hunger.
// P20 should route to revision. P21 must consume the route.
static void test_revision_and_propagation_route() {
    LearningLoopInput input;
    input.loop_key = "loop_revision_red_berry";
    input.story_kind = LearningLoopStoryKind::CorrectionAfterContradiction;
    input.actor = makeActor("agent_a");
    input.recipient = makeActor("agent_b");
    input.feedback = makeEatFeedback("red_berry", "poison");
    input.feedback.condition_keys.push_back("decayed");
    input.feedback.outcome_signals = {CognitionOutcomeSignal::HealthWorsened, CognitionOutcomeSignal::DamageTaken};
    input.loop_tick = Tick(20);
    input.reason_keys = {"revision_story"};

    // Actor existing claim: red_berry + eat -> restore_hunger
    input.actor_existing_claims.push_back(makeClaim("know_a_old", input.actor.knowledge_owner, "red_berry", "restore_hunger", KnowledgeStatus::Active));

    // Recipient existing claim: red_berry + eat -> restore_hunger
    input.recipient_existing_claims.push_back(makeClaim("know_b_old", input.recipient->knowledge_owner, "red_berry", "restore_hunger", KnowledgeStatus::Shared));

    LearningLoopOptions opts;
    opts.enable_memory_compression = true;
    opts.enable_memory_recall = true;
    opts.enable_knowledge_formation = true;
    opts.enable_knowledge_revision = true;
    opts.enable_knowledge_propagation = true;
    opts.enable_recipient_projection_check = false;
    opts.route_propagation_conflict_to_revision = true;
    opts.require_full_chain = false;

    LearningLoopService service;
    auto result_r = service.run(input, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();

    bool has_formation = false;
    bool has_propagation_route = false;
    bool has_revision_route = false;
    bool has_failed_revision = false;
    for (const auto& t : result.traces) {
        if (t.stage == LearningLoopStage::KnowledgeFormed && t.decision == LearningStepDecision::Executed) {
            has_formation = true;
        }
        if (t.stage == LearningLoopStage::KnowledgeRevised && t.decision == LearningStepDecision::Routed) {
            has_revision_route = true;
        }
        if (t.stage == LearningLoopStage::KnowledgeRevised && t.decision == LearningStepDecision::Failed) {
            has_failed_revision = true;
        }
    }
    assert(has_formation);
    assert(!has_failed_revision);

    assert(result.knowledge_propagation_result.has_value());
    assert(result.knowledge_propagation_result->plan.has_value());
    for (const auto& assessment : result.knowledge_propagation_result->plan->assessments) {
        if (assessment.should_route_to_revision) {
            has_propagation_route = true;
        }
    }
    assert(has_propagation_route);
    assert(has_revision_route);
    assert(result.decision == LearningLoopDecision::RoutedToRevision);

    // Verify condition key preserved in actor claims
    bool has_decayed_condition = false;
    for (const auto& c : result.actor_claim_snapshot_after) {
        for (const auto& cond : c.conditions) {
            if (cond.condition_key == "decayed") {
                has_decayed_condition = true;
            }
        }
    }
    assert(has_decayed_condition);

    // Verify recipient does not have two contradictory active claims without revision trace
    size_t recipient_red_berry_eat_active = 0;
    for (const auto& c : result.recipient_claim_snapshot_after) {
        if (c.subject.subject_id == "red_berry" && c.predicate.action_key == "eat" &&
            (c.status == KnowledgeStatus::Active || c.status == KnowledgeStatus::Shared)) {
            recipient_red_berry_eat_active++;
        }
    }
    assert(recipient_red_berry_eat_active <= 1);

    auto val_r = result.validateBasic();
    assert(val_r.is_ok());

    std::cout << "revision_and_propagation_route passed" << std::endl;
}

// Story: low-confidence knowledge propagation traceability
static void test_low_confidence_teaching_trace() {
    LearningLoopInput input;
    input.loop_key = "loop_low_conf";
    input.story_kind = LearningLoopStoryKind::TeachingToRecipient;
    input.actor = makeActor("agent_a");
    input.recipient = makeActor("agent_b");
    input.feedback = makeEatFeedback("red_berry");
    input.loop_tick = Tick(10);

    // Actor has low-confidence claim
    KnowledgeClaim low_conf = makeClaim("know_low", input.actor.knowledge_owner, "red_berry", "restore_hunger", KnowledgeStatus::Hypothesis);
    low_conf.confidence.confidence = 0.40;
    low_conf.confidence.band = KnowledgeConfidenceBand::Weak;
    input.actor_existing_claims.push_back(low_conf);

    LearningLoopOptions opts;
    opts.enable_memory_compression = false;
    opts.enable_memory_recall = false;
    opts.enable_knowledge_formation = false;
    opts.enable_knowledge_revision = false;
    opts.enable_knowledge_propagation = true;
    opts.enable_recipient_projection_check = true;
    opts.require_full_chain = false;

    LearningLoopService service;
    auto result_r = service.run(input, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();

    bool recipient_has_teaching = false;
    for (const auto& c : result.recipient_claim_snapshot_after) {
        if (c.owner.entity_id.value() == "agent_b") {
            for (const auto& ev : c.evidence_refs) {
                if (ev.evidence_kind == KnowledgeEvidenceKind::Teaching) {
                    recipient_has_teaching = true;
                }
            }
            // Confidence should be capped by propagation
            assert(c.confidence.confidence <= 0.81); // max_created_confidence default 0.80 + small epsilon
        }
    }
    if (!result.recipient_claim_snapshot_after.empty()) {
        assert(recipient_has_teaching);
    }

    std::cout << "low_confidence_teaching_trace passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_revision_and_propagation_route();
    test_low_confidence_teaching_trace();
    std::cout << "All p21 revision story tests passed" << std::endl;
    return 0;
}
