#include "pathfinder/reaction/reaction_learning_bridge.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static bool validConfidence(double value) {
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

static const ReactionKnowledgeTemplate* findTemplate(const std::vector<ReactionKnowledgeTemplate>& templates, const std::string& rule_key) {
    for (const auto& tmpl : templates) {
        if (tmpl.rule_key == rule_key) return &tmpl;
    }
    return nullptr;
}

static ReactionKnowledgeDraft applyTemplate(ReactionKnowledgeDraft knowledge, const ReactionKnowledgeTemplate* tmpl) {
    if (!tmpl) return knowledge;
    if (tmpl->subject_policy == ReactionKnowledgeSubjectPolicy::ExplicitSubject && !tmpl->subject_id.empty()) {
        knowledge.subject_id = tmpl->subject_id;
    }
    if (!tmpl->action_key.empty()) knowledge.action_key = tmpl->action_key;
    if (!tmpl->effect_key.empty()) knowledge.effect_key = tmpl->effect_key;
    return knowledge;
}

static ReactionKnowledgeDeliveryDraft makeDelivery(
    pathfinder::foundation::EntityId owner_id,
    ReactionLearningSourceKind source_kind,
    const ReactionKnowledgeDraft& knowledge,
    const ReactionKnowledgeTemplate* tmpl,
    double confidence_hint) {

    ReactionKnowledgeDeliveryDraft delivery;
    delivery.owner_id = std::move(owner_id);
    delivery.source_kind = source_kind;
    delivery.knowledge = applyTemplate(knowledge, tmpl);
    delivery.confidence_hint = confidence_hint;
    if (tmpl) {
        delivery.shareable = tmpl->shareable;
        delivery.teachable = tmpl->teachable;
        delivery.npc_learnable = tmpl->npc_learnable;
    }
    return delivery;
}

std::string toString(ReactionLearningSourceKind kind) {
    switch (kind) {
        case ReactionLearningSourceKind::Unknown: return "unknown";
        case ReactionLearningSourceKind::DirectExperience: return "direct_experience";
        case ReactionLearningSourceKind::Observation: return "observation";
        case ReactionLearningSourceKind::Taught: return "taught";
        case ReactionLearningSourceKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<ReactionLearningSourceKind> reactionLearningSourceKindFromString(const std::string& value) {
    if (value == "unknown") return Result<ReactionLearningSourceKind>::ok(ReactionLearningSourceKind::Unknown);
    if (value == "direct_experience") return Result<ReactionLearningSourceKind>::ok(ReactionLearningSourceKind::DirectExperience);
    if (value == "observation") return Result<ReactionLearningSourceKind>::ok(ReactionLearningSourceKind::Observation);
    if (value == "taught") return Result<ReactionLearningSourceKind>::ok(ReactionLearningSourceKind::Taught);
    if (value == "test_only") return Result<ReactionLearningSourceKind>::ok(ReactionLearningSourceKind::TestOnly);
    return Result<ReactionLearningSourceKind>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ReactionLearningSourceKind: " + value));
}

Result<void> ReactionKnowledgeDeliveryDraft::validateBasic() const {
    if (owner_id.empty() || !isValidIdString(owner_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionKnowledgeDeliveryDraft owner_id invalid"));
    }
    if (source_kind == ReactionLearningSourceKind::Unknown || source_kind == ReactionLearningSourceKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionKnowledgeDeliveryDraft source_kind invalid"));
    }
    if (!validConfidence(confidence_hint)) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ReactionKnowledgeDeliveryDraft confidence_hint must be 0..1"));
    }
    return knowledge.validateBasic();
}

Result<void> ReactionKnowledgePlan::validateBasic() const {
    for (const auto& delivery : deliveries) {
        auto valid = delivery.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> ReactionKnowledgePlannerInput::validateBasic() const {
    if (actor_id.empty() || !isValidIdString(actor_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionKnowledgePlannerInput actor_id invalid"));
    }
    for (const auto& observer : observer_ids) {
        if (observer.empty() || !isValidIdString(observer.value())) {
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionKnowledgePlannerInput observer_id invalid"));
        }
    }
    for (const auto& taught : taught_ids) {
        if (taught.empty() || !isValidIdString(taught.value())) {
            return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionKnowledgePlannerInput taught_id invalid"));
        }
    }
    auto result_valid = result.validateBasic();
    if (result_valid.is_error()) return result_valid;
    for (const auto& tmpl : knowledge_templates) {
        auto valid = tmpl.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<ReactionKnowledgePlan> ReactionKnowledgePlanner::plan(const ReactionKnowledgePlannerInput& input) const {
    auto valid = input.validateBasic();
    if (valid.is_error()) return Result<ReactionKnowledgePlan>::fail(valid.errors());

    ReactionKnowledgePlan plan;
    if (input.result.decision != ReactionDecision::Reacted || input.result.knowledge_drafts.empty()) {
        return Result<ReactionKnowledgePlan>::ok(std::move(plan));
    }

    const auto* tmpl = findTemplate(input.knowledge_templates, input.result.selected_rule_key);

    for (const auto& knowledge : input.result.knowledge_drafts) {
        auto direct = makeDelivery(input.actor_id, ReactionLearningSourceKind::DirectExperience, knowledge, tmpl, 1.0);
        auto direct_valid = direct.validateBasic();
        if (direct_valid.is_error()) return Result<ReactionKnowledgePlan>::fail(direct_valid.errors());
        plan.deliveries.push_back(std::move(direct));

        if (!tmpl || (tmpl->npc_learnable && tmpl->observation_learnable)) {
            for (const auto& observer : input.observer_ids) {
                auto observed = makeDelivery(observer, ReactionLearningSourceKind::Observation, knowledge, tmpl, 0.6);
                auto observed_valid = observed.validateBasic();
                if (observed_valid.is_error()) return Result<ReactionKnowledgePlan>::fail(observed_valid.errors());
                plan.deliveries.push_back(std::move(observed));
            }
        }

        if (!tmpl || tmpl->teachable) {
            for (const auto& taught : input.taught_ids) {
                auto taught_delivery = makeDelivery(taught, ReactionLearningSourceKind::Taught, knowledge, tmpl, 0.75);
                auto taught_valid = taught_delivery.validateBasic();
                if (taught_valid.is_error()) return Result<ReactionKnowledgePlan>::fail(taught_valid.errors());
                plan.deliveries.push_back(std::move(taught_delivery));
            }
        }
    }

    auto plan_valid = plan.validateBasic();
    if (plan_valid.is_error()) return Result<ReactionKnowledgePlan>::fail(plan_valid.errors());
    return Result<ReactionKnowledgePlan>::ok(std::move(plan));
}

} // namespace pathfinder::reaction
