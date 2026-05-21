#include "pathfinder/world_learning/world_learning_loop_bridge.h"

namespace pathfinder::world_learning {

WorldLearningLoopBridge::WorldLearningLoopBridge(
    const pathfinder::learning::LearningLoopService& learning_service,
    const pathfinder::learning::LearningLoopOptions& options)
    : learning_service_(learning_service), options_(options) {
    // P49: allow partial completion so that missing inputs do not block knowledge formation
    options_.require_full_chain = false;
}

WorldLearningLoopBridge::BridgeResult WorldLearningLoopBridge::run(
    const pathfinder::learning::LearningSafeFeedbackInput& feedback,
    const std::string& actor_key,
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& existing_claims,
    const std::vector<pathfinder::memory::MemoryRecord>& existing_memories,
    const std::vector<pathfinder::memory::MemorySummary>& existing_summaries,
    uint64_t tick,
    const std::string& loop_key) const {
    BridgeResult result;

    // Build LearningActorRef
    pathfinder::learning::LearningActorRef actor_ref;
    actor_ref.knowledge_owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    actor_ref.knowledge_owner.entity_id = pathfinder::foundation::EntityId(actor_key);
    actor_ref.memory_owner.kind = pathfinder::memory::MemoryOwnerKind::Actor;
    actor_ref.memory_owner.entity_id = pathfinder::foundation::EntityId(actor_key);
    actor_ref.cognition_subject.kind = pathfinder::cognition::CognitionSubjectKind::Actor;
    actor_ref.cognition_subject.subject_id = pathfinder::foundation::EntityId(actor_key);
    actor_ref.display_key = actor_key;

    // Build LearningLoopInput
    pathfinder::learning::LearningLoopInput input;
    input.loop_key = loop_key;
    input.story_kind = pathfinder::learning::LearningLoopStoryKind::DirectDiscovery;
    input.actor = actor_ref;
    input.feedback = feedback;
    input.actor_existing_claims = existing_claims;
    input.actor_existing_memories = existing_memories;
    input.actor_existing_summaries = existing_summaries;
    input.loop_tick = pathfinder::foundation::Tick(tick);
    input.reason_keys = feedback.reason_keys;

    auto validation = input.validateBasic();
    if (!validation.is_ok()) {
        result.failure_kind = WorldLearningFailureKind::FeedbackBuildFailed;
        result.warning_keys.push_back("learning_loop_input_validation_failed");
        return result;
    }

    auto run_result = learning_service_.run(input, options_);
    if (!run_result.is_ok()) {
        result.failure_kind = WorldLearningFailureKind::LearningLoopFailed;
        result.warning_keys.push_back("learning_loop_run_failed:" + loop_key);
        return result;
    }

    result.loop_result = run_result.value();
    if (!result.loop_result.ok()) {
        result.failure_kind = WorldLearningFailureKind::LearningLoopFailed;
        result.warning_keys.push_back("learning_loop_result_not_ok:" + loop_key);
    }

    return result;
}

} // namespace pathfinder::world_learning
