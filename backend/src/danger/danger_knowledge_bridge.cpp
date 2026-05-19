#include "pathfinder/danger/danger_knowledge_bridge.h"
#include "pathfinder/foundation/error.h"
#include <utility>

namespace pathfinder::danger {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

Result<void> DangerKnowledgePlannerInput::validateBasic() const {
    if (actor_id.empty() || !pathfinder::foundation::isValidIdString(actor_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "DangerKnowledgePlannerInput actor_id invalid"));
    }
    for (const auto& id : observer_ids) {
        if (id.empty() || !pathfinder::foundation::isValidIdString(id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "observer_id invalid"));
    }
    for (const auto& id : taught_ids) {
        if (id.empty() || !pathfinder::foundation::isValidIdString(id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "taught_id invalid"));
    }
    for (const auto& draft : knowledge_drafts) {
        auto valid = draft.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<pathfinder::reaction::ReactionKnowledgeDraft> toReactionKnowledgeDraft(const DangerKnowledgeDraft& draft) {
    auto valid = draft.validateBasic();
    if (valid.is_error()) return Result<pathfinder::reaction::ReactionKnowledgeDraft>::fail(valid.errors());
    pathfinder::reaction::ReactionKnowledgeDraft reaction;
    reaction.knowledge_key = draft.knowledge_key;
    reaction.subject_id = draft.subject_id;
    reaction.action_key = draft.action_key;
    reaction.effect_key = draft.effect_key;
    reaction.conditions = draft.conditions;
    reaction.reason_keys = draft.reason_keys;
    auto reaction_valid = reaction.validateBasic();
    if (reaction_valid.is_error()) return Result<pathfinder::reaction::ReactionKnowledgeDraft>::fail(reaction_valid.errors());
    return Result<pathfinder::reaction::ReactionKnowledgeDraft>::ok(std::move(reaction));
}

static pathfinder::reaction::ReactionKnowledgeDeliveryDraft delivery(
    pathfinder::foundation::EntityId owner_id,
    pathfinder::reaction::ReactionLearningSourceKind source,
    const pathfinder::reaction::ReactionKnowledgeDraft& knowledge,
    const DangerKnowledgeDraft& danger,
    double confidence) {
    pathfinder::reaction::ReactionKnowledgeDeliveryDraft out;
    out.owner_id = std::move(owner_id);
    out.source_kind = source;
    out.knowledge = knowledge;
    out.confidence_hint = confidence;
    out.shareable = danger.shareable;
    out.teachable = danger.teachable;
    out.npc_learnable = danger.npc_learnable;
    return out;
}

Result<pathfinder::reaction::ReactionKnowledgePlan> DangerKnowledgePlanner::plan(const DangerKnowledgePlannerInput& input) const {
    auto valid = input.validateBasic();
    if (valid.is_error()) return Result<pathfinder::reaction::ReactionKnowledgePlan>::fail(valid.errors());

    pathfinder::reaction::ReactionKnowledgePlan plan;
    for (const auto& draft : input.knowledge_drafts) {
        auto knowledge = toReactionKnowledgeDraft(draft);
        if (knowledge.is_error()) return Result<pathfinder::reaction::ReactionKnowledgePlan>::fail(knowledge.errors());
        plan.deliveries.push_back(delivery(input.actor_id, pathfinder::reaction::ReactionLearningSourceKind::DirectExperience, knowledge.value(), draft, 1.0));
        if (draft.npc_learnable) {
            for (const auto& observer_id : input.observer_ids) {
                plan.deliveries.push_back(delivery(observer_id, pathfinder::reaction::ReactionLearningSourceKind::Observation, knowledge.value(), draft, 0.6));
            }
        }
        if (draft.teachable) {
            for (const auto& taught_id : input.taught_ids) {
                plan.deliveries.push_back(delivery(taught_id, pathfinder::reaction::ReactionLearningSourceKind::Taught, knowledge.value(), draft, 0.75));
            }
        }
    }

    auto plan_valid = plan.validateBasic();
    if (plan_valid.is_error()) return Result<pathfinder::reaction::ReactionKnowledgePlan>::fail(plan_valid.errors());
    return Result<pathfinder::reaction::ReactionKnowledgePlan>::ok(std::move(plan));
}

} // namespace pathfinder::danger
