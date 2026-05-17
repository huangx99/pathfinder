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

static LearningSafeFeedbackInput makeEatFeedback(const std::string& subject_id) {
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

static void test_feedback_key_hidden_truth_rejected() {
    LearningLoopInput input;
    input.loop_key = "loop_sec_001";
    input.story_kind = LearningLoopStoryKind::DirectDiscovery;
    input.actor = makeActor("agent_a");
    input.feedback = makeEatFeedback("red_berry");
    input.feedback.feedback_key = "hidden_truth_key";
    input.loop_tick = Tick(10);

    LearningLoopService service;
    auto result_r = service.run(input);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == LearningLoopDecision::Rejected || result.decision == LearningLoopDecision::Failed);

    std::cout << "feedback_key_hidden_truth_rejected passed" << std::endl;
}

static void test_effect_key_real_effect_rejected() {
    LearningLoopInput input;
    input.loop_key = "loop_sec_002";
    input.story_kind = LearningLoopStoryKind::DirectDiscovery;
    input.actor = makeActor("agent_a");
    input.feedback = makeEatFeedback("red_berry");
    input.feedback.effect_key = "real_effect_poison";
    input.loop_tick = Tick(10);

    LearningLoopService service;
    auto result_r = service.run(input);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == LearningLoopDecision::Rejected || result.decision == LearningLoopDecision::Failed);

    std::cout << "effect_key_real_effect_rejected passed" << std::endl;
}

static void test_reason_key_raw_state_rejected() {
    LearningLoopInput input;
    input.loop_key = "loop_sec_003";
    input.story_kind = LearningLoopStoryKind::DirectDiscovery;
    input.actor = makeActor("agent_a");
    input.feedback = makeEatFeedback("red_berry");
    input.feedback.reason_keys.push_back("raw_state_detected");
    input.loop_tick = Tick(10);

    LearningLoopService service;
    auto result_r = service.run(input);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == LearningLoopDecision::Rejected || result.decision == LearningLoopDecision::Failed);

    std::cout << "reason_key_raw_state_rejected passed" << std::endl;
}

static void test_input_gamestate_key_rejected() {
    LearningLoopInput input;
    input.loop_key = "loop_sec_004";
    input.story_kind = LearningLoopStoryKind::DirectDiscovery;
    input.actor = makeActor("agent_a");
    input.feedback = makeEatFeedback("red_berry");
    input.reason_keys.push_back("GameState_access");
    input.loop_tick = Tick(10);

    LearningLoopService service;
    auto result_r = service.run(input);
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == LearningLoopDecision::Rejected || result.decision == LearningLoopDecision::Failed);

    std::cout << "input_gamestate_key_rejected passed" << std::endl;
}

static void test_recipient_owner_mismatch_rejected() {
    // Build a recipient claim with wrong owner in existing claims
    LearningLoopInput input;
    input.loop_key = "loop_sec_005";
    input.story_kind = LearningLoopStoryKind::TeachingToRecipient;
    input.actor = makeActor("agent_a");
    input.recipient = makeActor("agent_b");
    input.feedback = makeEatFeedback("red_berry");
    input.loop_tick = Tick(10);

    KnowledgeClaim bad_claim;
    bad_claim.knowledge_id = KnowledgeId("know_bad_001");
    bad_claim.owner.kind = KnowledgeOwnerKind::Agent;
    bad_claim.owner.entity_id = EntityId("agent_c"); // wrong owner
    bad_claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    bad_claim.subject.subject_id = "red_berry";
    bad_claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    bad_claim.predicate.action_key = "eat";
    bad_claim.predicate.effect_key = "restore_hunger";
    bad_claim.confidence.confidence = 0.5;
    bad_claim.status = KnowledgeStatus::Shared;
    bad_claim.created_tick = Tick(1);
    bad_claim.updated_tick = Tick(1);
    bad_claim.evidence_refs.push_back(KnowledgeEvidence{});
    bad_claim.evidence_refs.back().evidence_id = KnowledgeEvidenceId("kevd_bad");
    bad_claim.evidence_refs.back().evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    bad_claim.evidence_refs.back().source_summary_id = MemorySummaryId("sum_bad");
    bad_claim.evidence_refs.back().supports_claim = true;
    bad_claim.evidence_refs.back().reliability = 0.8;
    bad_claim.evidence_refs.back().confidence_delta = 0.3;
    bad_claim.evidence_refs.back().observed_tick = Tick(1);

    input.recipient_existing_claims.push_back(bad_claim);

    LearningLoopService service;
    auto result_r = service.run(input);
    // Should fail due to validation of recipient_existing_claims owner mismatch in propagation attempt
    assert(result_r.is_ok());
    auto result = result_r.value();
    assert(result.decision == LearningLoopDecision::Rejected || result.decision == LearningLoopDecision::Failed);

    std::cout << "recipient_owner_mismatch_rejected passed" << std::endl;
}

static void test_propagation_without_teaching_evidence_rejected() {
    // This test validates that the P20 service itself rejects claims without Teaching evidence.
    // We test via KnowledgeRecipientClaimDraft validation.
    KnowledgeRecipientClaimDraft draft;
    draft.claim.knowledge_id = KnowledgeId("know_test");
    draft.claim.owner.kind = KnowledgeOwnerKind::Agent;
    draft.claim.owner.entity_id = EntityId("agent_b");
    draft.claim.subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    draft.claim.subject.subject_id = "red_berry";
    draft.claim.predicate.relation_type = KnowledgeRelationType::HasEffect;
    draft.claim.predicate.action_key = "eat";
    draft.claim.predicate.effect_key = "restore_hunger";
    draft.claim.confidence.confidence = 0.5;
    draft.claim.status = KnowledgeStatus::Shared;
    draft.claim.created_tick = Tick(1);
    draft.claim.updated_tick = Tick(1);
    draft.source_knowledge_id = KnowledgeId("know_src");
    draft.source_owner.kind = KnowledgeOwnerKind::Agent;
    draft.source_owner.entity_id = EntityId("agent_a");
    draft.channel = KnowledgePropagationChannel::DirectTeaching;
    draft.transfer_score = 0.5;
    // NO Teaching evidence in claim.evidence_refs

    auto r = draft.validateBasic();
    assert(r.is_error());

    std::cout << "propagation_without_teaching_evidence_rejected passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_feedback_key_hidden_truth_rejected();
    test_effect_key_real_effect_rejected();
    test_reason_key_raw_state_rejected();
    test_input_gamestate_key_rejected();
    test_recipient_owner_mismatch_rejected();
    test_propagation_without_teaching_evidence_rejected();
    std::cout << "All p21 security tests passed" << std::endl;
    return 0;
}
