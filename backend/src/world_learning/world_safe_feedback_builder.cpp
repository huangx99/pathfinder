#include "pathfinder/world_learning/world_safe_feedback_builder.h"
#include "pathfinder/content/content_registry.h"
#include <algorithm>
#include <cmath>

namespace pathfinder::world_learning {

WorldSafeFeedbackBuilder::BuildResult WorldSafeFeedbackBuilder::build(
    const world_command::WorldExperienceDto& experience,
    const pathfinder::content::KnowledgeTemplateContent& tmpl,
    const pathfinder::content::EffectDefinitionContent* effect,
    const std::vector<world_command::WorldEventDto>& source_events,
    uint64_t tick) const {
    BuildResult result;

    result.feedback.feedback_key = experience.experience_id + ":feedback";

    // Cognition target: object definition level
    result.feedback.cognition_target.kind = pathfinder::cognition::CognitionTargetKind::ObjectDefinition;
    result.feedback.cognition_target.target_id = !tmpl.subject_object_key.empty()
        ? tmpl.subject_object_key : experience.subject_entity_key;
    result.feedback.cognition_target.public_label_key = result.feedback.cognition_target.target_id;

    // Memory subject
    result.feedback.memory_subject.kind = pathfinder::memory::MemorySubjectKind::ObjectDefinition;
    result.feedback.memory_subject.subject_id = result.feedback.cognition_target.target_id;
    result.feedback.memory_subject.subject_type_key = result.feedback.cognition_target.target_id;

    // Knowledge subject
    result.feedback.knowledge_subject.kind = pathfinder::knowledge::KnowledgeSubjectKind::ObjectDefinition;
    result.feedback.knowledge_subject.subject_id = result.feedback.cognition_target.target_id;
    result.feedback.knowledge_subject.subject_type_key = result.feedback.cognition_target.target_id;

    // Action context
    result.feedback.action_context = mapCommandToActionContext(experience.command_key);

    // Action id/key
    result.feedback.action_id = pathfinder::foundation::ActionId(experience.command_key);
    result.feedback.action_key = experience.command_key;

    // Target subject (effect target)
    result.feedback.target_subject_id = !tmpl.target_object_key.empty()
        ? tmpl.target_object_key : experience.target_entity_key;
    result.feedback.target_subject_type_key = result.feedback.target_subject_id;

    // Outcome signals
    result.feedback.outcome_signals.push_back(
        mapEffectToOutcomeSignal(effect, experience.command_key));

    // Effect key
    result.feedback.effect_key = !tmpl.effect_key.empty()
        ? tmpl.effect_key : experience.effect_key;

    // Condition keys: merge experience + template context
    result.feedback.condition_keys = experience.condition_keys;

    // Reason keys: merge experience + template key
    result.feedback.reason_keys = experience.reason_keys;
    if (!tmpl.key.value().empty()) {
        result.feedback.reason_keys.push_back(tmpl.key.value());
    }
    // Deduplicate reason_keys
    std::sort(result.feedback.reason_keys.begin(), result.feedback.reason_keys.end());
    result.feedback.reason_keys.erase(
        std::unique(result.feedback.reason_keys.begin(), result.feedback.reason_keys.end()),
        result.feedback.reason_keys.end());

    result.feedback.utility_delta = computeUtilityDelta(effect);
    result.feedback.risk_delta = computeRiskDelta(effect);

    // Observed tick
    result.feedback.observed_tick = pathfinder::foundation::Tick(
        experience.tick > 0 ? experience.tick : tick);

    // Source event ids
    for (const auto& evt : source_events) {
        if (!evt.event_id.empty()) {
            result.feedback.source_event_ids.push_back(
                pathfinder::foundation::EventId(evt.event_id));
        }
    }
    // Ensure at least one source event id exists for downstream cognition evidence
    if (result.feedback.source_event_ids.empty()) {
        result.feedback.source_event_ids.push_back(
            pathfinder::foundation::EventId(experience.experience_id + ":event"));
    }

    auto validation = result.feedback.validateBasic();
    if (!validation.is_ok()) {
        result.failure_kind = WorldLearningFailureKind::FeedbackBuildFailed;
        result.warning_keys.push_back("feedback_validation_failed:" + experience.experience_id);
    }

    return result;
}


WorldSafeFeedbackBuilder::BuildResult WorldSafeFeedbackBuilder::build(
    const world_command::WorldExperienceDto& experience,
    const pathfinder::content::KnowledgeTemplateContent& tmpl,
    const std::vector<world_command::WorldEventDto>& source_events,
    uint64_t tick) const {
    return build(experience, tmpl, nullptr, source_events, tick);
}

pathfinder::cognition::CognitionOutcomeSignal WorldSafeFeedbackBuilder::mapEffectToOutcomeSignal(
    const pathfinder::content::EffectDefinitionContent* effect,
    const std::string& command_key) const {
    if (effect != nullptr) {
        bool consumed_object = false;
        for (const auto& op : effect->operations) {
            if (op.op == "change_actor_need") {
                if (op.need_key == "health" && op.delta < 0.0) {
                    return pathfinder::cognition::CognitionOutcomeSignal::HealthWorsened;
                }
                if (op.delta != 0.0) {
                    return pathfinder::cognition::CognitionOutcomeSignal::NeedImproved;
                }
            }
            if (op.op == "produce_object_quantity" || op.op == "add_object_quantity") {
                return pathfinder::cognition::CognitionOutcomeSignal::NewObjectProduced;
            }
            if (op.op == "remove_threat") {
                return pathfinder::cognition::CognitionOutcomeSignal::DangerObserved;
            }
            if (op.op == "consume_object_quantity") {
                consumed_object = true;
            }
            if (op.op == "hint_object_use" || op.op == "instinct_fear_response") {
                return pathfinder::cognition::CognitionOutcomeSignal::ToolActivated;
            }
        }
        if (effect->risk_score >= 5) {
            return pathfinder::cognition::CognitionOutcomeSignal::HealthWorsened;
        }
        if (consumed_object) {
            return pathfinder::cognition::CognitionOutcomeSignal::ObjectConsumed;
        }
    }

    if (command_key == "gather" || command_key == "chop" || command_key == "mine" || command_key == "dig" || command_key == "craft") {
        return pathfinder::cognition::CognitionOutcomeSignal::NewObjectProduced;
    }
    if (command_key == "eat" || command_key == "consume") {
        return pathfinder::cognition::CognitionOutcomeSignal::ObjectConsumed;
    }
    if (command_key == "use" || command_key == "activate" || command_key == "combine") {
        return pathfinder::cognition::CognitionOutcomeSignal::ToolActivated;
    }
    return pathfinder::cognition::CognitionOutcomeSignal::NoVisibleEffect;
}
pathfinder::cognition::CognitionActionContextKind WorldSafeFeedbackBuilder::mapCommandToActionContext(
    const std::string& command_key) const {
    if (command_key == "eat") return pathfinder::cognition::CognitionActionContextKind::Eat;
    if (command_key == "use") return pathfinder::cognition::CognitionActionContextKind::Use;
    // P49: craft/combine maps to Use instead of Combine because Use has richer evidence mapping
    // (e.g. NewObjectProduced with confidence 0.35 vs generic fallback 0.15)
    if (command_key == "craft" || command_key == "combine") return pathfinder::cognition::CognitionActionContextKind::Use;
    if (command_key == "inspect" || command_key == "gather" || command_key == "chop" || command_key == "mine") {
        return pathfinder::cognition::CognitionActionContextKind::Observe;
    }
    return pathfinder::cognition::CognitionActionContextKind::Touch;
}

double WorldSafeFeedbackBuilder::computeUtilityDelta(
    const pathfinder::content::EffectDefinitionContent* effect) const {
    if (effect == nullptr) return 0.35;
    return effect->confidence_delta;
}

double WorldSafeFeedbackBuilder::computeRiskDelta(
    const pathfinder::content::EffectDefinitionContent* effect) const {
    if (effect == nullptr) return 0.0;
    return std::clamp(static_cast<double>(effect->risk_score) / 10.0, 0.0, 1.0);
}

} // namespace pathfinder::world_learning
