#include "pathfinder/cognition/cognition_v2_types.h"
#include <algorithm>
#include <cctype>

namespace pathfinder::cognition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// Hidden Truth Validation
// ============================================================

static const std::vector<std::string> kHiddenTruthKeys = {
    "edible_profile", // hidden_truth
    "hunger_delta",   // hidden_truth
    "health_delta",   // hidden_truth
    "effect_kind"     // hidden_truth
};

bool containsHiddenTruthKey(const std::string& text) {
    std::string lower;
    lower.reserve(text.size());
    for (char c : text) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    for (const auto& key : kHiddenTruthKeys) {
        if (lower.find(key) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsHiddenTruthKey(const std::vector<std::string>& texts) {
    for (const auto& text : texts) {
        if (containsHiddenTruthKey(text)) {
            return true;
        }
    }
    return false;
}

// ============================================================
// ID Generation
// ============================================================

CognitionEvidenceRecordId makeEvidenceRecordId(
    const pathfinder::foundation::EventId& source_event_id,
    CognitionAspect aspect,
    size_t index) {
    return CognitionEvidenceRecordId(
        "cognition_evidence_" + source_event_id.value() + "_" + toString(aspect) + "_" + std::to_string(index));
}

CognitionClaimV2Id makeClaimV2Id(
    const pathfinder::foundation::EntityId& subject_id,
    const std::string& target_id,
    const pathfinder::foundation::ActionId& action_id,
    CognitionAspect aspect) {
    return CognitionClaimV2Id(
        "cognition_claim_" + subject_id.value() + "_" + target_id + "_" + action_id.value() + "_" + toString(aspect));
}

// ============================================================
// CognitionSubjectKind
// ============================================================

std::string toString(CognitionSubjectKind kind) {
    switch (kind) {
        case CognitionSubjectKind::Unknown: return "unknown";
        case CognitionSubjectKind::Actor: return "actor";
        case CognitionSubjectKind::Agent: return "agent";
        case CognitionSubjectKind::Group: return "group";
        case CognitionSubjectKind::Tribe: return "tribe";
        case CognitionSubjectKind::Civilization: return "civilization";
        case CognitionSubjectKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CognitionSubjectKind> cognitionSubjectKindFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionSubjectKind>::ok(CognitionSubjectKind::Unknown);
    if (str == "actor") return Result<CognitionSubjectKind>::ok(CognitionSubjectKind::Actor);
    if (str == "agent") return Result<CognitionSubjectKind>::ok(CognitionSubjectKind::Agent);
    if (str == "group") return Result<CognitionSubjectKind>::ok(CognitionSubjectKind::Group);
    if (str == "tribe") return Result<CognitionSubjectKind>::ok(CognitionSubjectKind::Tribe);
    if (str == "civilization") return Result<CognitionSubjectKind>::ok(CognitionSubjectKind::Civilization);
    if (str == "test_only") return Result<CognitionSubjectKind>::ok(CognitionSubjectKind::TestOnly);
    return Result<CognitionSubjectKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionSubjectKind: " + str));
}

// ============================================================
// CognitionTargetKind
// ============================================================

std::string toString(CognitionTargetKind kind) {
    switch (kind) {
        case CognitionTargetKind::Unknown: return "unknown";
        case CognitionTargetKind::ObjectDefinition: return "object_definition"; // hidden_truth: safe enum
        case CognitionTargetKind::WorldObject: return "world_object";
        case CognitionTargetKind::ObjectCategory: return "object_category";
        case CognitionTargetKind::AgentSpecies: return "agent_species";
        case CognitionTargetKind::EnvironmentFeature: return "environment_feature";
        case CognitionTargetKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CognitionTargetKind> cognitionTargetKindFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionTargetKind>::ok(CognitionTargetKind::Unknown);
    if (str == "object_definition") return Result<CognitionTargetKind>::ok(CognitionTargetKind::ObjectDefinition); // hidden_truth: safe enum
    if (str == "world_object") return Result<CognitionTargetKind>::ok(CognitionTargetKind::WorldObject);
    if (str == "object_category") return Result<CognitionTargetKind>::ok(CognitionTargetKind::ObjectCategory);
    if (str == "agent_species") return Result<CognitionTargetKind>::ok(CognitionTargetKind::AgentSpecies);
    if (str == "environment_feature") return Result<CognitionTargetKind>::ok(CognitionTargetKind::EnvironmentFeature);
    if (str == "test_only") return Result<CognitionTargetKind>::ok(CognitionTargetKind::TestOnly);
    return Result<CognitionTargetKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionTargetKind: " + str));
}

// ============================================================
// CognitionAspect
// ============================================================

std::string toString(CognitionAspect aspect) {
    switch (aspect) {
        case CognitionAspect::Unknown: return "unknown";
        case CognitionAspect::Edibility: return "edibility";
        case CognitionAspect::Usability: return "usability";
        case CognitionAspect::ConsumptionEffect: return "consumption_effect";
        case CognitionAspect::UseEffect: return "use_effect";
        case CognitionAspect::Risk: return "risk";
        case CognitionAspect::Utility: return "utility";
    }
    return "unknown";
}

Result<CognitionAspect> cognitionAspectFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionAspect>::ok(CognitionAspect::Unknown);
    if (str == "edibility") return Result<CognitionAspect>::ok(CognitionAspect::Edibility);
    if (str == "usability") return Result<CognitionAspect>::ok(CognitionAspect::Usability);
    if (str == "consumption_effect") return Result<CognitionAspect>::ok(CognitionAspect::ConsumptionEffect);
    if (str == "use_effect") return Result<CognitionAspect>::ok(CognitionAspect::UseEffect);
    if (str == "risk") return Result<CognitionAspect>::ok(CognitionAspect::Risk);
    if (str == "utility") return Result<CognitionAspect>::ok(CognitionAspect::Utility);
    return Result<CognitionAspect>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionAspect: " + str));
}

// ============================================================
// CognitionBeliefPolarity
// ============================================================

std::string toString(CognitionBeliefPolarity polarity) {
    switch (polarity) {
        case CognitionBeliefPolarity::Unknown: return "unknown";
        case CognitionBeliefPolarity::Positive: return "positive";
        case CognitionBeliefPolarity::Negative: return "negative";
        case CognitionBeliefPolarity::Mixed: return "mixed";
    }
    return "unknown";
}

Result<CognitionBeliefPolarity> cognitionBeliefPolarityFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionBeliefPolarity>::ok(CognitionBeliefPolarity::Unknown);
    if (str == "positive") return Result<CognitionBeliefPolarity>::ok(CognitionBeliefPolarity::Positive);
    if (str == "negative") return Result<CognitionBeliefPolarity>::ok(CognitionBeliefPolarity::Negative);
    if (str == "mixed") return Result<CognitionBeliefPolarity>::ok(CognitionBeliefPolarity::Mixed);
    return Result<CognitionBeliefPolarity>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionBeliefPolarity: " + str));
}

// ============================================================
// CognitionEvidenceSupport
// ============================================================

std::string toString(CognitionEvidenceSupport support) {
    switch (support) {
        case CognitionEvidenceSupport::Unknown: return "unknown";
        case CognitionEvidenceSupport::Supports: return "supports";
        case CognitionEvidenceSupport::Refutes: return "refutes";
        case CognitionEvidenceSupport::Neutral: return "neutral";
        case CognitionEvidenceSupport::Conflicts: return "conflicts";
    }
    return "unknown";
}

Result<CognitionEvidenceSupport> cognitionEvidenceSupportFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionEvidenceSupport>::ok(CognitionEvidenceSupport::Unknown);
    if (str == "supports") return Result<CognitionEvidenceSupport>::ok(CognitionEvidenceSupport::Supports);
    if (str == "refutes") return Result<CognitionEvidenceSupport>::ok(CognitionEvidenceSupport::Refutes);
    if (str == "neutral") return Result<CognitionEvidenceSupport>::ok(CognitionEvidenceSupport::Neutral);
    if (str == "conflicts") return Result<CognitionEvidenceSupport>::ok(CognitionEvidenceSupport::Conflicts);
    return Result<CognitionEvidenceSupport>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionEvidenceSupport: " + str));
}

// ============================================================
// CognitionEvidenceSourceKind
// ============================================================

std::string toString(CognitionEvidenceSourceKind kind) {
    switch (kind) {
        case CognitionEvidenceSourceKind::Unknown: return "unknown";
        case CognitionEvidenceSourceKind::DirectActionFeedback: return "direct_action_feedback";
        case CognitionEvidenceSourceKind::ObservedEvent: return "observed_event";
        case CognitionEvidenceSourceKind::StateChangeObservation: return "state_change_observation";
        case CognitionEvidenceSourceKind::TaughtByOther: return "taught_by_other";
        case CognitionEvidenceSourceKind::ImportedSafeProjection: return "imported_safe_projection";
        case CognitionEvidenceSourceKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CognitionEvidenceSourceKind> cognitionEvidenceSourceKindFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionEvidenceSourceKind>::ok(CognitionEvidenceSourceKind::Unknown);
    if (str == "direct_action_feedback") return Result<CognitionEvidenceSourceKind>::ok(CognitionEvidenceSourceKind::DirectActionFeedback);
    if (str == "observed_event") return Result<CognitionEvidenceSourceKind>::ok(CognitionEvidenceSourceKind::ObservedEvent);
    if (str == "state_change_observation") return Result<CognitionEvidenceSourceKind>::ok(CognitionEvidenceSourceKind::StateChangeObservation);
    if (str == "taught_by_other") return Result<CognitionEvidenceSourceKind>::ok(CognitionEvidenceSourceKind::TaughtByOther);
    if (str == "imported_safe_projection") return Result<CognitionEvidenceSourceKind>::ok(CognitionEvidenceSourceKind::ImportedSafeProjection);
    if (str == "test_only") return Result<CognitionEvidenceSourceKind>::ok(CognitionEvidenceSourceKind::TestOnly);
    return Result<CognitionEvidenceSourceKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionEvidenceSourceKind: " + str));
}

// ============================================================
// CognitionActionContextKind
// ============================================================

std::string toString(CognitionActionContextKind kind) {
    switch (kind) {
        case CognitionActionContextKind::Unknown: return "unknown";
        case CognitionActionContextKind::Eat: return "eat";
        case CognitionActionContextKind::Use: return "use";
        case CognitionActionContextKind::Touch: return "touch";
        case CognitionActionContextKind::Combine: return "combine";
        case CognitionActionContextKind::Observe: return "observe";
        case CognitionActionContextKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CognitionActionContextKind> cognitionActionContextKindFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionActionContextKind>::ok(CognitionActionContextKind::Unknown);
    if (str == "eat") return Result<CognitionActionContextKind>::ok(CognitionActionContextKind::Eat);
    if (str == "use") return Result<CognitionActionContextKind>::ok(CognitionActionContextKind::Use);
    if (str == "touch") return Result<CognitionActionContextKind>::ok(CognitionActionContextKind::Touch);
    if (str == "combine") return Result<CognitionActionContextKind>::ok(CognitionActionContextKind::Combine);
    if (str == "observe") return Result<CognitionActionContextKind>::ok(CognitionActionContextKind::Observe);
    if (str == "test_only") return Result<CognitionActionContextKind>::ok(CognitionActionContextKind::TestOnly);
    return Result<CognitionActionContextKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionActionContextKind: " + str));
}

// ============================================================
// CognitionOutcomeSignal
// ============================================================

std::string toString(CognitionOutcomeSignal signal) {
    switch (signal) {
        case CognitionOutcomeSignal::Unknown: return "unknown";
        case CognitionOutcomeSignal::NeedImproved: return "need_improved";
        case CognitionOutcomeSignal::NeedWorsened: return "need_worsened";
        case CognitionOutcomeSignal::HealthImproved: return "health_improved";
        case CognitionOutcomeSignal::HealthWorsened: return "health_worsened";
        case CognitionOutcomeSignal::DamageTaken: return "damage_taken";
        case CognitionOutcomeSignal::ObjectConsumed: return "object_consumed";
        case CognitionOutcomeSignal::ToolActivated: return "tool_activated";
        case CognitionOutcomeSignal::NewObjectProduced: return "new_object_produced";
        case CognitionOutcomeSignal::NoVisibleEffect: return "no_visible_effect";
        case CognitionOutcomeSignal::DangerObserved: return "danger_observed";
        case CognitionOutcomeSignal::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<CognitionOutcomeSignal> cognitionOutcomeSignalFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::Unknown);
    if (str == "need_improved") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::NeedImproved);
    if (str == "need_worsened") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::NeedWorsened);
    if (str == "health_improved") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::HealthImproved);
    if (str == "health_worsened") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::HealthWorsened);
    if (str == "damage_taken") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::DamageTaken);
    if (str == "object_consumed") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::ObjectConsumed);
    if (str == "tool_activated") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::ToolActivated);
    if (str == "new_object_produced") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::NewObjectProduced);
    if (str == "no_visible_effect") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::NoVisibleEffect);
    if (str == "danger_observed") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::DangerObserved);
    if (str == "test_only") return Result<CognitionOutcomeSignal>::ok(CognitionOutcomeSignal::TestOnly);
    return Result<CognitionOutcomeSignal>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionOutcomeSignal: " + str));
}

// ============================================================
// CognitionRiskLevel
// ============================================================

std::string toString(CognitionRiskLevel level) {
    switch (level) {
        case CognitionRiskLevel::Unknown: return "unknown";
        case CognitionRiskLevel::None: return "none";
        case CognitionRiskLevel::Low: return "low";
        case CognitionRiskLevel::Medium: return "medium";
        case CognitionRiskLevel::High: return "high";
        case CognitionRiskLevel::Critical: return "critical";
    }
    return "unknown";
}

Result<CognitionRiskLevel> cognitionRiskLevelFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionRiskLevel>::ok(CognitionRiskLevel::Unknown);
    if (str == "none") return Result<CognitionRiskLevel>::ok(CognitionRiskLevel::None);
    if (str == "low") return Result<CognitionRiskLevel>::ok(CognitionRiskLevel::Low);
    if (str == "medium") return Result<CognitionRiskLevel>::ok(CognitionRiskLevel::Medium);
    if (str == "high") return Result<CognitionRiskLevel>::ok(CognitionRiskLevel::High);
    if (str == "critical") return Result<CognitionRiskLevel>::ok(CognitionRiskLevel::Critical);
    return Result<CognitionRiskLevel>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionRiskLevel: " + str));
}

// ============================================================
// CognitionConfidenceBand
// ============================================================

std::string toString(CognitionConfidenceBand band) {
    switch (band) {
        case CognitionConfidenceBand::Unknown: return "unknown";
        case CognitionConfidenceBand::Untrusted: return "untrusted";
        case CognitionConfidenceBand::Hypothesis: return "hypothesis";
        case CognitionConfidenceBand::Actionable: return "actionable";
        case CognitionConfidenceBand::Reliable: return "reliable";
        case CognitionConfidenceBand::Teachable: return "teachable";
    }
    return "unknown";
}

Result<CognitionConfidenceBand> cognitionConfidenceBandFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionConfidenceBand>::ok(CognitionConfidenceBand::Unknown);
    if (str == "untrusted") return Result<CognitionConfidenceBand>::ok(CognitionConfidenceBand::Untrusted);
    if (str == "hypothesis") return Result<CognitionConfidenceBand>::ok(CognitionConfidenceBand::Hypothesis);
    if (str == "actionable") return Result<CognitionConfidenceBand>::ok(CognitionConfidenceBand::Actionable);
    if (str == "reliable") return Result<CognitionConfidenceBand>::ok(CognitionConfidenceBand::Reliable);
    if (str == "teachable") return Result<CognitionConfidenceBand>::ok(CognitionConfidenceBand::Teachable);
    return Result<CognitionConfidenceBand>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionConfidenceBand: " + str));
}

// ============================================================
// CognitionUpdateDecision
// ============================================================

std::string toString(CognitionUpdateDecision decision) {
    switch (decision) {
        case CognitionUpdateDecision::Unknown: return "unknown";
        case CognitionUpdateDecision::Created: return "created";
        case CognitionUpdateDecision::Reinforced: return "reinforced";
        case CognitionUpdateDecision::Weakened: return "weakened";
        case CognitionUpdateDecision::Conflicted: return "conflicted";
        case CognitionUpdateDecision::Ignored: return "ignored";
    }
    return "unknown";
}

Result<CognitionUpdateDecision> cognitionUpdateDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionUpdateDecision>::ok(CognitionUpdateDecision::Unknown);
    if (str == "created") return Result<CognitionUpdateDecision>::ok(CognitionUpdateDecision::Created);
    if (str == "reinforced") return Result<CognitionUpdateDecision>::ok(CognitionUpdateDecision::Reinforced);
    if (str == "weakened") return Result<CognitionUpdateDecision>::ok(CognitionUpdateDecision::Weakened);
    if (str == "conflicted") return Result<CognitionUpdateDecision>::ok(CognitionUpdateDecision::Conflicted);
    if (str == "ignored") return Result<CognitionUpdateDecision>::ok(CognitionUpdateDecision::Ignored);
    return Result<CognitionUpdateDecision>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionUpdateDecision: " + str));
}

// ============================================================
// CognitionQueryMode
// ============================================================

std::string toString(CognitionQueryMode mode) {
    switch (mode) {
        case CognitionQueryMode::Unknown: return "unknown";
        case CognitionQueryMode::ExactTarget: return "exact_target";
        case CognitionQueryMode::IncludeTargetKindFallback: return "include_target_kind_fallback";
        case CognitionQueryMode::BestActionable: return "best_actionable";
        case CognitionQueryMode::TeachableCandidates: return "teachable_candidates";
    }
    return "unknown";
}

Result<CognitionQueryMode> cognitionQueryModeFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionQueryMode>::ok(CognitionQueryMode::Unknown);
    if (str == "exact_target") return Result<CognitionQueryMode>::ok(CognitionQueryMode::ExactTarget);
    if (str == "include_target_kind_fallback") return Result<CognitionQueryMode>::ok(CognitionQueryMode::IncludeTargetKindFallback);
    if (str == "best_actionable") return Result<CognitionQueryMode>::ok(CognitionQueryMode::BestActionable);
    if (str == "teachable_candidates") return Result<CognitionQueryMode>::ok(CognitionQueryMode::TeachableCandidates);
    return Result<CognitionQueryMode>::fail(
        makeError(ErrorCode::validation_enum_unknown, "invalid CognitionQueryMode: " + str));
}

// ============================================================
// DTO validateBasic
// ============================================================

Result<void> CognitionSubject::validateBasic() const {
    if (kind == CognitionSubjectKind::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionSubject: kind is Unknown"));
    }
    if (subject_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionSubject: subject_id is empty"));
    }
    if (!pathfinder::foundation::isValidIdString(subject_id.value())) {
        return Result<void>::fail(
            makeError(ErrorCode::id_invalid_format, "CognitionSubject: subject_id has invalid format"));
    }
    return Result<void>::ok();
}

Result<void> CognitionTarget::validateBasic() const {
    if (kind == CognitionTargetKind::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionTarget: kind is Unknown"));
    }
    if (target_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionTarget: target_id is empty"));
    }
    if (!pathfinder::foundation::isValidIdString(target_id)) {
        return Result<void>::fail(
            makeError(ErrorCode::id_invalid_format, "CognitionTarget: target_id has invalid format"));
    }
    if (containsHiddenTruthKey(public_label_key)) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionTarget: public_label_key contains hidden truth"));
    }
    return Result<void>::ok();
}

Result<void> CognitionClaimKeyV2::validateBasic() const {
    auto sr = subject.validateBasic();
    if (sr.is_error()) return sr;
    auto tr = target.validateBasic();
    if (tr.is_error()) return tr;
    if (action_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionClaimKeyV2: action_id is empty"));
    }
    if (!pathfinder::foundation::isValidIdString(action_id.value())) {
        return Result<void>::fail(
            makeError(ErrorCode::id_invalid_format, "CognitionClaimKeyV2: action_id has invalid format"));
    }
    if (action_context == CognitionActionContextKind::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionClaimKeyV2: action_context is Unknown"));
    }
    if (aspect == CognitionAspect::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionClaimKeyV2: aspect is Unknown"));
    }
    return Result<void>::ok();
}

Result<void> CognitionEvidenceRecord::validateBasic() const {
    auto kr = key.validateBasic();
    if (kr.is_error()) return kr;
    if (source_kind == CognitionEvidenceSourceKind::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionEvidenceRecord: source_kind is Unknown"));
    }
    if (support == CognitionEvidenceSupport::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionEvidenceRecord: support is Unknown"));
    }
    if (confidence_weight < 0.0 || confidence_weight > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionEvidenceRecord: confidence_weight must be 0.0 to 1.0"));
    }
    if (utility_signal < -1.0 || utility_signal > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionEvidenceRecord: utility_signal must be -1.0 to 1.0"));
    }
    if (containsHiddenTruthKey(reason_keys)) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionEvidenceRecord: reason_keys contains hidden truth"));
    }
    if (containsHiddenTruthKey(warning_keys)) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionEvidenceRecord: warning_keys contains hidden truth"));
    }
    if (evidence_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionEvidenceRecord: evidence_id is empty"));
    }
    if (source_event_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionEvidenceRecord: source_event_id is empty"));
    }
    if (outcome_signal == CognitionOutcomeSignal::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionEvidenceRecord: outcome_signal is Unknown"));
    }
    return Result<void>::ok();
}

Result<void> CognitionClaimV2::validateBasic() const {
    auto kr = key.validateBasic();
    if (kr.is_error()) return kr;
    if (confidence < 0.0 || confidence > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionClaimV2: confidence must be 0.0 to 1.0"));
    }
    if (utility_score < -1.0 || utility_score > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionClaimV2: utility_score must be -1.0 to 1.0"));
    }
    if (conflicted && polarity != CognitionBeliefPolarity::Mixed) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed,
                "CognitionClaimV2: conflicted=true requires polarity=Mixed"));
    }
    if (containsHiddenTruthKey(reason_keys)) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionClaimV2: reason_keys contains hidden truth"));
    }
    if (containsHiddenTruthKey(warning_keys)) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionClaimV2: warning_keys contains hidden truth"));
    }
    if (claim_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionClaimV2: claim_id is empty"));
    }
    if (evidence_count < supporting_evidence_count + refuting_evidence_count) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed,
                "CognitionClaimV2: evidence_count is less than supporting + refuting"));
    }
    return Result<void>::ok();
}

Result<void> CognitionFeedbackSignal::validateBasic() const {
    auto sr = subject.validateBasic();
    if (sr.is_error()) return sr;
    auto tr = target.validateBasic();
    if (tr.is_error()) return tr;
    if (action_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionFeedbackSignal: action_id is empty"));
    }
    if (action_context == CognitionActionContextKind::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionFeedbackSignal: action_context is Unknown"));
    }
    if (utility_signal < -1.0 || utility_signal > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionFeedbackSignal: utility_signal must be -1.0 to 1.0"));
    }
    if (containsHiddenTruthKey(reason_keys)) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionFeedbackSignal: reason_keys contains hidden truth"));
    }
    return Result<void>::ok();
}

Result<void> CognitionConfidenceModelOptions::validateBasic() const {
    if (initial_direct_feedback_confidence < 0.0 || initial_direct_feedback_confidence > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionConfidenceModelOptions: initial_direct_feedback_confidence must be 0.0 to 1.0"));
    }
    if (actionable_threshold < 0.0 || actionable_threshold > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionConfidenceModelOptions: actionable_threshold must be 0.0 to 1.0"));
    }
    if (reliable_threshold < 0.0 || reliable_threshold > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionConfidenceModelOptions: reliable_threshold must be 0.0 to 1.0"));
    }
    if (teachable_threshold < 0.0 || teachable_threshold > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionConfidenceModelOptions: teachable_threshold must be 0.0 to 1.0"));
    }
    if (min_confidence < 0.0 || min_confidence > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionConfidenceModelOptions: min_confidence must be 0.0 to 1.0"));
    }
    if (max_confidence < 0.0 || max_confidence > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionConfidenceModelOptions: max_confidence must be 0.0 to 1.0"));
    }
    if (min_confidence >= max_confidence) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed,
                "CognitionConfidenceModelOptions: min_confidence must be < max_confidence"));
    }
    if (actionable_threshold >= reliable_threshold || reliable_threshold >= teachable_threshold) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed,
                "CognitionConfidenceModelOptions: thresholds must be strictly increasing"));
    }
    return Result<void>::ok();
}

Result<void> CognitionUpdateRequest::validateBasic() const {
    auto er = evidence.validateBasic();
    if (er.is_error()) return er;
    auto or_ = options.validateBasic();
    if (or_.is_error()) return or_;
    return Result<void>::ok();
}

Result<void> CognitionUpdateResult::validateBasic() const {
    auto ar = after_claim.validateBasic();
    if (ar.is_error()) return ar;
    if (before_claim) {
        auto br = before_claim->validateBasic();
        if (br.is_error()) return br;
    }
    return Result<void>::ok();
}

Result<void> CognitionQuery::validateBasic() const {
    auto sr = subject.validateBasic();
    if (sr.is_error()) return sr;
    if (target) {
        auto tr = target->validateBasic();
        if (tr.is_error()) return tr;
    }
    if (min_confidence < 0.0 || min_confidence > 1.0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_value_out_of_range,
                "CognitionQuery: min_confidence must be 0.0 to 1.0"));
    }
    if (max_results == 0) {
        return Result<void>::fail(
            makeError(ErrorCode::validation_failed, "CognitionQuery: max_results must be > 0"));
    }
    return Result<void>::ok();
}

Result<void> CognitionQueryResult::validateBasic() const {
    auto qr = query.validateBasic();
    if (qr.is_error()) return qr;
    for (const auto& claim : claims) {
        auto cr = claim.validateBasic();
        if (cr.is_error()) return cr;
    }
    if (best_claim) {
        auto br = best_claim->validateBasic();
        if (br.is_error()) return br;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::cognition
