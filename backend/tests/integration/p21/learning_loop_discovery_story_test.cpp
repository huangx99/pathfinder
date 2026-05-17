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
    fb.utility_delta = 0.5;
    fb.risk_delta = 0.0;
    fb.observed_tick = Tick(10);
    fb.source_event_ids = {EventId("evt_001")};
    fb.reason_keys = {"direct_experience"};
    return fb;
}

// Story: Agent A eats red_berry, gets NeedImproved feedback, forms cognition,
// memory, knowledge, then teaches B. B gets Teaching-derived claim.
static void test_discovery_and_teaching() {
    LearningLoopInput input;
    input.loop_key = "loop_discovery_red_berry";
    input.story_kind = LearningLoopStoryKind::FullLearningLoop;
    input.actor = makeActor("agent_a");
    input.recipient = makeActor("agent_b");
    input.feedback = makeEatFeedback("red_berry");
    input.loop_tick = Tick(10);
    input.reason_keys = {"discovery_story"};

    LearningLoopOptions opts;
    opts.enable_memory_compression = true;
    opts.enable_memory_recall = true;
    opts.enable_knowledge_formation = true;
    opts.enable_knowledge_revision = true;
    opts.enable_knowledge_propagation = true;
    opts.enable_recipient_projection_check = true;
    opts.require_full_chain = false; // allow some steps to be skipped if no data

    LearningLoopService service;
    auto result_r = service.run(input, opts);
    assert(result_r.is_ok());
    auto result = result_r.value();

    // Verify decision is ok
    assert(result.ok());

    // Verify traces contain required stages
    bool has_cognition = false;
    bool has_memory = false;
    bool has_knowledge_formed = false;
    bool has_propagation = false;
    bool has_recipient = false;

    for (const auto& t : result.traces) {
        if (t.stage == LearningLoopStage::CognitionUpdated && t.decision == LearningStepDecision::Executed) has_cognition = true;
        if (t.stage == LearningLoopStage::MemoryWritten && t.decision == LearningStepDecision::Executed) has_memory = true;
        if (t.stage == LearningLoopStage::KnowledgeFormed && t.decision == LearningStepDecision::Executed) has_knowledge_formed = true;
        if (t.stage == LearningLoopStage::KnowledgePropagated && t.decision == LearningStepDecision::Executed) has_propagation = true;
        if (t.stage == LearningLoopStage::RecipientInfluenced && t.decision == LearningStepDecision::Executed) has_recipient = true;
    }

    assert(has_cognition);
    assert(has_memory);
    assert(has_knowledge_formed);
    assert(has_propagation);
    assert(has_recipient);

    // Actor claim snapshot must have at least 1 claim about red_berry/eat/restore_hunger
    bool actor_has_claim = false;
    for (const auto& c : result.actor_claim_snapshot_after) {
        if (c.owner.entity_id.value() == "agent_a" &&
            c.subject.subject_id == "red_berry" &&
            c.predicate.action_key == "eat" &&
            c.predicate.effect_key == "restore_hunger") {
            actor_has_claim = true;
            break;
        }
    }
    assert(actor_has_claim);

    // Recipient claim must be owned by B and have Teaching evidence
    bool recipient_owner_ok = false;
    bool recipient_has_teaching = false;
    for (const auto& c : result.recipient_claim_snapshot_after) {
        if (c.owner.entity_id.value() == "agent_b" &&
            c.subject.subject_id == "red_berry" &&
            c.predicate.action_key == "eat" &&
            c.predicate.effect_key == "restore_hunger") {
            recipient_owner_ok = true;
            for (const auto& ev : c.evidence_refs) {
                if (ev.evidence_kind == KnowledgeEvidenceKind::Teaching) {
                    recipient_has_teaching = true;
                    break;
                }
            }
        }
    }
    assert(recipient_owner_ok);
    assert(recipient_has_teaching);

    // Validate all ok DTOs
    auto val_r = result.validateBasic();
    assert(val_r.is_ok());

    std::cout << "discovery_and_teaching passed" << std::endl;
}

// Verify no hidden truth / raw state in result
static void test_result_no_hidden_truth() {
    LearningLoopInput input;
    input.loop_key = "loop_clean";
    input.story_kind = LearningLoopStoryKind::DirectDiscovery;
    input.actor = makeActor("agent_a");
    input.feedback = makeEatFeedback("red_berry");
    input.loop_tick = Tick(10);

    LearningLoopService service;
    auto result_r = service.run(input);
    assert(result_r.is_ok());
    auto result = result_r.value();

    // Verify no forbidden keys in reason_keys or warning_keys
    assert(!containsLearningForbiddenKey(result.reason_keys));
    assert(!containsLearningForbiddenKey(result.warning_keys));

    if (result.event_chain.has_value()) {
        assert(!containsLearningForbiddenKey(result.event_chain->chain_reason_keys));
        assert(!containsLearningForbiddenKey(result.event_chain->warning_keys));
    }

    std::cout << "result_no_hidden_truth passed" << std::endl;
}

int main(int argc, char* argv[]) {
    test_discovery_and_teaching();
    test_result_no_hidden_truth();
    std::cout << "All p21 discovery story tests passed" << std::endl;
    return 0;
}
