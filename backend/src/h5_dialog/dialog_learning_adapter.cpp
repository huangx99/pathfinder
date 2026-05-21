// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#include "pathfinder/h5_dialog/dialog_learning_adapter.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;
using pathfinder::foundation::ActionId;
using pathfinder::foundation::EventId;
using pathfinder::learning::LearningLoopInput;
using pathfinder::learning::LearningLoopResult;
using pathfinder::learning::LearningLoopStoryKind;
using pathfinder::learning::LearningActorRef;
using pathfinder::learning::LearningSafeFeedbackInput;
using pathfinder::learning::LearningLoopOptions;

namespace {

ActionId actionIdFromDialogAction(DialogActionKind action) {
    switch (action) {
        case DialogActionKind::Eat: return ActionId("eat");
        case DialogActionKind::Use: return ActionId("use");
        case DialogActionKind::Observe: return ActionId("observe");
        case DialogActionKind::Teach: return ActionId("teach");
        case DialogActionKind::Inspect: return ActionId("inspect");
        default: return ActionId("unknown");
    }
}

pathfinder::cognition::CognitionActionContextKind cognitionContextFromAction(DialogActionKind action) {
    switch (action) {
        case DialogActionKind::Eat: return pathfinder::cognition::CognitionActionContextKind::Eat;
        case DialogActionKind::Use: return pathfinder::cognition::CognitionActionContextKind::Use;
        case DialogActionKind::Observe: return pathfinder::cognition::CognitionActionContextKind::Observe;
        default: return pathfinder::cognition::CognitionActionContextKind::Unknown;
    }
}

pathfinder::cognition::CognitionTarget cognitionTargetFromObject(const std::string& object_key) {
    pathfinder::cognition::CognitionTarget target;
    target.kind = pathfinder::cognition::CognitionTargetKind::ObjectDefinition;
    target.target_id = object_key;
    target.public_label_key = object_key;
    return target;
}

pathfinder::memory::MemorySubject memorySubjectFromObject(const std::string& object_key) {
    pathfinder::memory::MemorySubject subject;
    subject.kind = pathfinder::memory::MemorySubjectKind::ObjectDefinition;
    subject.subject_id = object_key;
    subject.subject_type_key = "object";
    return subject;
}

std::string learningSubjectKeyFromObject(const std::string& object_key) {
    if (object_key == "decayed_red_berry") {
        return "red_berry";
    }
    return object_key;
}

std::string conditionKeyFromStateCondition(const DialogStateCondition& condition) {
    std::string value = condition.tag_value.empty() ? condition.state_key : condition.tag_value;
    if (condition.op == "has_tag") {
        return "condition:object_state:eq:" + value;
    }
    if (condition.op == "missing_tag") {
        return "condition:object_state:not:" + value;
    }
    return "condition:object_state:" + condition.object_key + ":" + condition.state_key + ":" + condition.op + ":" + std::to_string(condition.number_value);
}

void appendUnique(std::vector<std::string>& values, const std::string& value) {
    if (value.empty()) return;
    if (std::find(values.begin(), values.end(), value) == values.end()) {
        values.push_back(value);
    }
}

pathfinder::knowledge::KnowledgeSubject knowledgeSubjectFromObject(const std::string& object_key) {
    pathfinder::knowledge::KnowledgeSubject subject;
    subject.kind = pathfinder::knowledge::KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = learningSubjectKeyFromObject(object_key);
    subject.subject_type_key = "object";
    return subject;
}


int statusPriority(pathfinder::knowledge::KnowledgeStatus status) {
    switch (status) {
        case pathfinder::knowledge::KnowledgeStatus::Active: return 80;
        case pathfinder::knowledge::KnowledgeStatus::Operational: return 75;
        case pathfinder::knowledge::KnowledgeStatus::Teachable: return 70;
        case pathfinder::knowledge::KnowledgeStatus::Shared: return 65;
        case pathfinder::knowledge::KnowledgeStatus::Hypothesis: return 50;
        case pathfinder::knowledge::KnowledgeStatus::Candidate: return 40;
        case pathfinder::knowledge::KnowledgeStatus::Unknown: return 10;
        case pathfinder::knowledge::KnowledgeStatus::Deprecated: return 5;
        case pathfinder::knowledge::KnowledgeStatus::Disproven: return 0;
        case pathfinder::knowledge::KnowledgeStatus::TestOnly: return 0;
    }
    return 0;
}

bool hasReasonKey(const pathfinder::knowledge::KnowledgeClaim& claim, const std::string& reason_key) {
    return std::find(claim.reason_keys.begin(), claim.reason_keys.end(), reason_key) != claim.reason_keys.end();
}

std::string conditionKey(const pathfinder::knowledge::KnowledgeCondition& condition) {
    auto canonical = pathfinder::knowledge::canonicalKnowledgeConditionKey(condition);
    std::string key = canonical.is_ok() ? canonical.value() : condition.condition_key;
    key += ":" + condition.condition_summary_key;
    key += ":" + condition.region_id.value();
    key += ":" + condition.time_scope_key;
    for (const auto& object_state_key : condition.object_state_keys) {
        key += ":obj=" + object_state_key;
    }
    for (const auto& actor_requirement_key : condition.actor_requirement_keys) {
        key += ":actor=" + actor_requirement_key;
    }
    return key;
}

std::string claimSemanticKey(const pathfinder::knowledge::KnowledgeClaim& claim) {
    std::vector<std::string> condition_keys;
    condition_keys.reserve(claim.conditions.size());
    for (const auto& condition : claim.conditions) {
        condition_keys.push_back(conditionKey(condition));
    }
    std::sort(condition_keys.begin(), condition_keys.end());

    std::string key = pathfinder::knowledge::toString(claim.owner.kind);
    key += "|" + claim.owner.entity_id.value();
    key += "|" + claim.owner.tribe_id.value();
    key += "|" + claim.owner.region_id.value();
    key += "|" + claim.owner.external_key;
    key += "|" + pathfinder::knowledge::toString(claim.subject.kind);
    key += "|" + claim.subject.subject_id;
    key += "|" + claim.subject.subject_type_key;
    key += "|" + claim.subject.relation_group_key;
    for (const auto& related_subject_id : claim.subject.related_subject_ids) {
        key += "|target=" + related_subject_id;
    }
    key += "|" + pathfinder::knowledge::toString(claim.predicate.relation_type);
    key += "|" + claim.predicate.action_key;
    key += "|" + claim.predicate.effect_key;
    for (const auto& condition_key_value : condition_keys) {
        key += "|condition=" + condition_key_value;
    }
    return key;
}

bool shouldReplaceClaim(const pathfinder::knowledge::KnowledgeClaim& current,
                        const pathfinder::knowledge::KnowledgeClaim& candidate) {
    const bool current_ambiguous = hasReasonKey(current, "causal_ambiguous_dose_window");
    const bool candidate_ambiguous = hasReasonKey(candidate, "causal_ambiguous_dose_window");
    if (current_ambiguous != candidate_ambiguous) {
        return !candidate_ambiguous;
    }
    const auto current_priority = statusPriority(current.status);
    const auto candidate_priority = statusPriority(candidate.status);
    if (candidate_priority != current_priority) {
        return candidate_priority > current_priority;
    }
    if (candidate.confidence.confidence != current.confidence.confidence) {
        return candidate.confidence.confidence > current.confidence.confidence;
    }
    if (candidate.updated_tick != current.updated_tick) {
        return candidate.updated_tick > current.updated_tick;
    }
    return candidate.evidence_refs.size() > current.evidence_refs.size();
}

std::vector<pathfinder::knowledge::KnowledgeClaim> deduplicateClaims(
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    std::vector<pathfinder::knowledge::KnowledgeClaim> deduplicated;
    for (const auto& claim : claims) {
        const auto key = claimSemanticKey(claim);
        auto existing = std::find_if(deduplicated.begin(), deduplicated.end(), [&](const auto& value) {
            return claimSemanticKey(value) == key;
        });
        if (existing == deduplicated.end()) {
            deduplicated.push_back(claim);
        } else if (shouldReplaceClaim(*existing, claim)) {
            *existing = claim;
        }
    }
    return deduplicated;
}

LearningLoopStoryKind storyKindFromIntent(const DialogIntent& intent) {
    switch (intent.kind) {
        case DialogIntentKind::TryEat:
        case DialogIntentKind::TryUse:
            if (intent.object_key == "decayed_red_berry") {
                return LearningLoopStoryKind::CorrectionAfterContradiction;
            }
            return LearningLoopStoryKind::DirectDiscovery;
        case DialogIntentKind::TeachRecipient:
            return LearningLoopStoryKind::TeachingToRecipient;
        case DialogIntentKind::RepeatLastAction:
            return LearningLoopStoryKind::RepeatedLearning;
        default:
            return LearningLoopStoryKind::FullLearningLoop;
    }
}

} // namespace

Result<LearningLoopInput> DialogLearningAdapter::buildLearningInput(
    const DialogSessionState& state,
    const DialogIntent& intent,
    const DialogFeedbackTemplate& feedback) const {

    LearningLoopInput input;
    input.loop_key = "loop_" + state.session_id + "_" + std::to_string(state.turn_index);
    input.story_kind = storyKindFromIntent(intent);
    input.actor = state.actor;
    if (intent.kind == DialogIntentKind::TeachRecipient) {
        input.recipient = state.recipient;
    }
    input.loop_tick = state.current_tick;
    input.actor_existing_memories = state.actor_memories;
    input.actor_existing_summaries = state.actor_summaries;
    input.actor_existing_claims = state.actor_claims;
    if (intent.kind == DialogIntentKind::TeachRecipient) {
        input.recipient_existing_claims = state.recipient_claims;
    }

    LearningSafeFeedbackInput fb;
    fb.feedback_key = feedback.feedback_key;
    const auto learning_subject_key = learningSubjectKeyFromObject(feedback.object_key);
    fb.cognition_target = cognitionTargetFromObject(learning_subject_key);
    fb.memory_subject = memorySubjectFromObject(learning_subject_key);
    fb.knowledge_subject = knowledgeSubjectFromObject(learning_subject_key);
    if (!feedback.target_object_key.empty()) {
        fb.target_subject_id = learningSubjectKeyFromObject(feedback.target_object_key);
        fb.target_subject_type_key = "object";
    }
    fb.action_context = cognitionContextFromAction(feedback.action);
    fb.action_id = actionIdFromDialogAction(feedback.action);
    fb.action_key = toString(feedback.action);
    fb.outcome_signals = feedback.outcome_signals;
    fb.effect_key = feedback.effect_key;
    fb.condition_keys = feedback.condition_keys;
    for (const auto& state_condition : feedback.state_conditions) {
        appendUnique(fb.condition_keys, conditionKeyFromStateCondition(state_condition));
    }
    fb.utility_delta = feedback.utility_delta;
    fb.risk_delta = feedback.risk_delta;
    fb.reason_keys = feedback.reason_keys;
    fb.warning_keys = feedback.warning_keys;
    fb.observed_tick = state.current_tick;
    if (fb.source_event_ids.empty()) {
        fb.source_event_ids = {EventId("h5_dialog_event_" + state.session_id + "_" + std::to_string(state.turn_index))};
    }
    input.feedback = fb;

    auto input_r = input.validateBasic();
    if (input_r.is_error()) {
        return Result<LearningLoopInput>::fail(input_r.errors());
    }
    return Result<LearningLoopInput>::ok(input);
}

Result<void> DialogLearningAdapter::applyLearningResult(
    DialogSessionState& state,
    const LearningLoopResult& result) const {

    state.actor_claims = deduplicateClaims(result.actor_claim_snapshot_after);
    if (!result.recipient_claim_snapshot_after.empty()) {
        state.recipient_claims = deduplicateClaims(result.recipient_claim_snapshot_after);
    } else {
        state.recipient_claims = deduplicateClaims(state.recipient_claims);
    }

    if (result.memory_write_result.has_value()) {
        const auto& mwr = result.memory_write_result.value();
        for (const auto& rec : mwr.created_records) {
            state.actor_memories.push_back(rec);
        }
        for (const auto& rec : mwr.updated_records) {
            auto it = std::find_if(state.actor_memories.begin(), state.actor_memories.end(),
                [&rec](const auto& m) { return m.memory_id == rec.memory_id; });
            if (it != state.actor_memories.end()) {
                *it = rec;
            } else {
                state.actor_memories.push_back(rec);
            }
        }
    }

    if (result.memory_compression_result.has_value()) {
        const auto& mcr = result.memory_compression_result.value();
        if (mcr.summary.has_value()) {
            auto it = std::find_if(state.actor_summaries.begin(), state.actor_summaries.end(),
                [&mcr](const auto& s) { return s.summary_id == mcr.summary.value().summary_id; });
            if (it != state.actor_summaries.end()) {
                *it = mcr.summary.value();
            } else {
                state.actor_summaries.push_back(mcr.summary.value());
            }
        }
    }

    // Debug keys
    for (const auto& trace : result.traces) {
        if (trace.decision == pathfinder::learning::LearningStepDecision::Executed) {
            auto stage_str = pathfinder::learning::toString(trace.stage);
            state.debug_keys.push_back(stage_str);
        }
    }

    return Result<void>::ok();
}

} // namespace pathfinder::h5_dialog
