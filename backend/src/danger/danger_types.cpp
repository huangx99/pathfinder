#include "pathfinder/danger/danger_types.h"
#include "pathfinder/condition/condition_expression_context.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

namespace pathfinder::danger {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static Result<void> validateSafeKey(const std::string& key, const std::string& field, bool required = true) {
    if (key.empty()) {
        if (required) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " empty"));
        return Result<void>::ok();
    }
    if (containsDangerForbiddenKey(key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " forbidden"));
    return Result<void>::ok();
}

static Result<void> validateSafeKeys(const std::vector<std::string>& keys, const std::string& field) {
    if (containsDangerForbiddenKey(keys)) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " forbidden"));
    return Result<void>::ok();
}

static Result<void> validateRatio(double value, const std::string& field) {
    if (!validDangerRatio(value)) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be 0..1"));
    return Result<void>::ok();
}

static Result<void> validateDelta(double value, const std::string& field) {
    if (!std::isfinite(value) || value < -1.0 || value > 1.0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " must be -1..1"));
    return Result<void>::ok();
}

std::string toString(DangerSourceKind kind) {
    switch (kind) {
        case DangerSourceKind::Unknown: return "unknown";
        case DangerSourceKind::Environment: return "environment";
        case DangerSourceKind::ObjectReaction: return "object_reaction";
        case DangerSourceKind::Creature: return "creature";
        case DangerSourceKind::TribeConflict: return "tribe_conflict";
        case DangerSourceKind::ResourcePressure: return "resource_pressure";
        case DangerSourceKind::KnowledgeConflict: return "knowledge_conflict";
        case DangerSourceKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(HazardTriggerKind kind) {
    switch (kind) {
        case HazardTriggerKind::Unknown: return "unknown";
        case HazardTriggerKind::Explore: return "explore";
        case HazardTriggerKind::Approach: return "approach";
        case HazardTriggerKind::Touch: return "touch";
        case HazardTriggerKind::Eat: return "eat";
        case HazardTriggerKind::Use: return "use";
        case HazardTriggerKind::Combine: return "combine";
        case HazardTriggerKind::Rest: return "rest";
        case HazardTriggerKind::Teach: return "teach";
        case HazardTriggerKind::Conflict: return "conflict";
        case HazardTriggerKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(DangerSeverity severity) {
    switch (severity) {
        case DangerSeverity::Unknown: return "unknown";
        case DangerSeverity::None: return "none";
        case DangerSeverity::Notice: return "notice";
        case DangerSeverity::Strain: return "strain";
        case DangerSeverity::Harm: return "harm";
        case DangerSeverity::Severe: return "severe";
        case DangerSeverity::Critical: return "critical";
        case DangerSeverity::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(DangerDecision decision) {
    switch (decision) {
        case DangerDecision::Unknown: return "unknown";
        case DangerDecision::Safe: return "safe";
        case DangerDecision::Warned: return "warned";
        case DangerDecision::Exposed: return "exposed";
        case DangerDecision::Avoided: return "avoided";
        case DangerDecision::Escaped: return "escaped";
        case DangerDecision::BlockedByCondition: return "blocked_by_condition";
        case DangerDecision::NoMatchingHazard: return "no_matching_hazard";
        case DangerDecision::Rejected: return "rejected";
        case DangerDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(FearSignalKind kind) {
    switch (kind) {
        case FearSignalKind::Unknown: return "unknown";
        case FearSignalKind::None: return "none";
        case FearSignalKind::Caution: return "caution";
        case FearSignalKind::Alarm: return "alarm";
        case FearSignalKind::Panic: return "panic";
        case FearSignalKind::GroupAnxiety: return "group_anxiety";
        case FearSignalKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(CountermeasureKind kind) {
    switch (kind) {
        case CountermeasureKind::Unknown: return "unknown";
        case CountermeasureKind::Knowledge: return "knowledge";
        case CountermeasureKind::Tool: return "tool";
        case CountermeasureKind::GroupSupport: return "group_support";
        case CountermeasureKind::Distance: return "distance";
        case CountermeasureKind::EscapeRoute: return "escape_route";
        case CountermeasureKind::Civilization: return "civilization";
        case CountermeasureKind::TestOnly: return "test_only";
    }
    return "unknown";
}

#define FROM_STRING_FN(Type, fn, ITEMS) \
Result<Type> fn(const std::string& value) { \
    ITEMS \
    return Result<Type>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid " #Type ": " + value)); \
}

FROM_STRING_FN(DangerSourceKind, dangerSourceKindFromString,
    if (value == "unknown") return Result<DangerSourceKind>::ok(DangerSourceKind::Unknown);
    if (value == "environment") return Result<DangerSourceKind>::ok(DangerSourceKind::Environment);
    if (value == "object_reaction") return Result<DangerSourceKind>::ok(DangerSourceKind::ObjectReaction);
    if (value == "creature") return Result<DangerSourceKind>::ok(DangerSourceKind::Creature);
    if (value == "tribe_conflict") return Result<DangerSourceKind>::ok(DangerSourceKind::TribeConflict);
    if (value == "resource_pressure") return Result<DangerSourceKind>::ok(DangerSourceKind::ResourcePressure);
    if (value == "knowledge_conflict") return Result<DangerSourceKind>::ok(DangerSourceKind::KnowledgeConflict);
    if (value == "test_only") return Result<DangerSourceKind>::ok(DangerSourceKind::TestOnly);
)

FROM_STRING_FN(HazardTriggerKind, hazardTriggerKindFromString,
    if (value == "unknown") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Unknown);
    if (value == "explore") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Explore);
    if (value == "approach") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Approach);
    if (value == "touch") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Touch);
    if (value == "eat") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Eat);
    if (value == "use") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Use);
    if (value == "combine") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Combine);
    if (value == "rest") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Rest);
    if (value == "teach") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Teach);
    if (value == "conflict") return Result<HazardTriggerKind>::ok(HazardTriggerKind::Conflict);
    if (value == "test_only") return Result<HazardTriggerKind>::ok(HazardTriggerKind::TestOnly);
)

FROM_STRING_FN(DangerSeverity, dangerSeverityFromString,
    if (value == "unknown") return Result<DangerSeverity>::ok(DangerSeverity::Unknown);
    if (value == "none") return Result<DangerSeverity>::ok(DangerSeverity::None);
    if (value == "notice") return Result<DangerSeverity>::ok(DangerSeverity::Notice);
    if (value == "strain") return Result<DangerSeverity>::ok(DangerSeverity::Strain);
    if (value == "harm") return Result<DangerSeverity>::ok(DangerSeverity::Harm);
    if (value == "severe") return Result<DangerSeverity>::ok(DangerSeverity::Severe);
    if (value == "critical") return Result<DangerSeverity>::ok(DangerSeverity::Critical);
    if (value == "test_only") return Result<DangerSeverity>::ok(DangerSeverity::TestOnly);
)

FROM_STRING_FN(DangerDecision, dangerDecisionFromString,
    if (value == "unknown") return Result<DangerDecision>::ok(DangerDecision::Unknown);
    if (value == "safe") return Result<DangerDecision>::ok(DangerDecision::Safe);
    if (value == "warned") return Result<DangerDecision>::ok(DangerDecision::Warned);
    if (value == "exposed") return Result<DangerDecision>::ok(DangerDecision::Exposed);
    if (value == "avoided") return Result<DangerDecision>::ok(DangerDecision::Avoided);
    if (value == "escaped") return Result<DangerDecision>::ok(DangerDecision::Escaped);
    if (value == "blocked_by_condition") return Result<DangerDecision>::ok(DangerDecision::BlockedByCondition);
    if (value == "no_matching_hazard") return Result<DangerDecision>::ok(DangerDecision::NoMatchingHazard);
    if (value == "rejected") return Result<DangerDecision>::ok(DangerDecision::Rejected);
    if (value == "test_only") return Result<DangerDecision>::ok(DangerDecision::TestOnly);
)

FROM_STRING_FN(FearSignalKind, fearSignalKindFromString,
    if (value == "unknown") return Result<FearSignalKind>::ok(FearSignalKind::Unknown);
    if (value == "none") return Result<FearSignalKind>::ok(FearSignalKind::None);
    if (value == "caution") return Result<FearSignalKind>::ok(FearSignalKind::Caution);
    if (value == "alarm") return Result<FearSignalKind>::ok(FearSignalKind::Alarm);
    if (value == "panic") return Result<FearSignalKind>::ok(FearSignalKind::Panic);
    if (value == "group_anxiety") return Result<FearSignalKind>::ok(FearSignalKind::GroupAnxiety);
    if (value == "test_only") return Result<FearSignalKind>::ok(FearSignalKind::TestOnly);
)

FROM_STRING_FN(CountermeasureKind, countermeasureKindFromString,
    if (value == "unknown") return Result<CountermeasureKind>::ok(CountermeasureKind::Unknown);
    if (value == "knowledge") return Result<CountermeasureKind>::ok(CountermeasureKind::Knowledge);
    if (value == "tool") return Result<CountermeasureKind>::ok(CountermeasureKind::Tool);
    if (value == "group_support") return Result<CountermeasureKind>::ok(CountermeasureKind::GroupSupport);
    if (value == "distance") return Result<CountermeasureKind>::ok(CountermeasureKind::Distance);
    if (value == "escape_route") return Result<CountermeasureKind>::ok(CountermeasureKind::EscapeRoute);
    if (value == "civilization") return Result<CountermeasureKind>::ok(CountermeasureKind::Civilization);
    if (value == "test_only") return Result<CountermeasureKind>::ok(CountermeasureKind::TestOnly);
)

#undef FROM_STRING_FN

bool containsDangerForbiddenKey(const std::string& key) {
    if (pathfinder::condition::containsConditionForbiddenKey(key)) return true;
    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    static const std::vector<std::string> forbidden = {"hp", "hp_delta", "true_hp", "death", "kill", "corpse", "loot", "drop", "random_damage", "critical_hit"};
    return std::any_of(forbidden.begin(), forbidden.end(), [&](const auto& token) { return lower.find(token) != std::string::npos; });
}

bool containsDangerForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) { return containsDangerForbiddenKey(key); });
}

bool validDangerRatio(double value) { return std::isfinite(value) && value >= 0.0 && value <= 1.0; }

double clampDangerRatio(double value) {
    if (!std::isfinite(value)) return 0.0;
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

int severityRank(DangerSeverity severity) {
    switch (severity) {
        case DangerSeverity::None: return 0;
        case DangerSeverity::Notice: return 1;
        case DangerSeverity::Strain: return 2;
        case DangerSeverity::Harm: return 3;
        case DangerSeverity::Severe: return 4;
        case DangerSeverity::Critical: return 5;
        default: return -1;
    }
}

DangerSeverity severityFromRank(int rank) {
    if (rank <= 0) return DangerSeverity::None;
    if (rank == 1) return DangerSeverity::Notice;
    if (rank == 2) return DangerSeverity::Strain;
    if (rank == 3) return DangerSeverity::Harm;
    if (rank == 4) return DangerSeverity::Severe;
    return DangerSeverity::Critical;
}

DangerSeverity reduceSeverity(DangerSeverity severity, double mitigation_score) {
    int rank = severityRank(severity);
    if (rank < 0) return DangerSeverity::Unknown;
    int reduction = mitigation_score >= 0.75 ? 3 : (mitigation_score >= 0.45 ? 2 : (mitigation_score >= 0.20 ? 1 : 0));
    return severityFromRank(std::max(0, rank - reduction));
}

FearSignalKind fearKindForSeverity(DangerSeverity severity, bool affects_group) {
    if (affects_group && severityRank(severity) >= severityRank(DangerSeverity::Strain)) return FearSignalKind::GroupAnxiety;
    if (severityRank(severity) >= severityRank(DangerSeverity::Severe)) return FearSignalKind::Panic;
    if (severityRank(severity) >= severityRank(DangerSeverity::Harm)) return FearSignalKind::Alarm;
    if (severityRank(severity) >= severityRank(DangerSeverity::Notice)) return FearSignalKind::Caution;
    return FearSignalKind::None;
}

Result<void> ThreatProfile::validateBasic() const {
    auto key = validateSafeKey(threat_key, "ThreatProfile threat_key"); if (key.is_error()) return key;
    if (source_kind == DangerSourceKind::Unknown || source_kind == DangerSourceKind::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ThreatProfile source_kind invalid"));
    auto public_name = validateSafeKey(public_name_key, "ThreatProfile public_name_key"); if (public_name.is_error()) return public_name;
    auto bp = validateRatio(base_pressure, "ThreatProfile base_pressure"); if (bp.is_error()) return bp;
    auto pp = validateRatio(proximity_pressure, "ThreatProfile proximity_pressure"); if (pp.is_error()) return pp;
    auto vis = validateRatio(visibility, "ThreatProfile visibility"); if (vis.is_error()) return vis;
    auto pred = validateRatio(predictability, "ThreatProfile predictability"); if (pred.is_error()) return pred;
    if (base_severity == DangerSeverity::Unknown || base_severity == DangerSeverity::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ThreatProfile base_severity invalid"));
    auto tags = validateSafeKeys(safe_tag_keys, "ThreatProfile safe_tag_keys"); if (tags.is_error()) return tags;
    auto states = validateSafeKeys(state_keys, "ThreatProfile state_keys"); if (states.is_error()) return states;
    return validateSafeKeys(reason_keys, "ThreatProfile reason_keys");
}

Result<void> CountermeasureRequirement::validateBasic() const {
    if (kind == CountermeasureKind::Unknown || kind == CountermeasureKind::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "CountermeasureRequirement kind invalid"));
    auto key = validateSafeKey(requirement_key, "CountermeasureRequirement requirement_key"); if (key.is_error()) return key;
    auto mit = validateRatio(mitigation_score, "CountermeasureRequirement mitigation_score"); if (mit.is_error()) return mit;
    if (!condition_ref.empty()) { auto cond = condition_ref.validateBasic(); if (cond.is_error()) return cond; }
    return validateSafeKeys(reason_keys, "CountermeasureRequirement reason_keys");
}

Result<void> HazardRule::validateBasic() const {
    auto key = validateSafeKey(rule_key, "HazardRule rule_key"); if (key.is_error()) return key;
    if (trigger_kind == HazardTriggerKind::Unknown || trigger_kind == HazardTriggerKind::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardRule trigger_kind invalid"));
    if (source_kind == DangerSourceKind::Unknown || source_kind == DangerSourceKind::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardRule source_kind invalid"));
    if (severity == DangerSeverity::Unknown || severity == DangerSeverity::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardRule severity invalid"));
    auto pd = validateRatio(pressure_delta, "HazardRule pressure_delta"); if (pd.is_error()) return pd;
    auto fd = validateRatio(fear_delta, "HazardRule fear_delta"); if (fd.is_error()) return fd;
    auto md = validateDelta(morale_delta, "HazardRule morale_delta"); if (md.is_error()) return md;
    auto td = validateDelta(trust_delta, "HazardRule trust_delta"); if (td.is_error()) return td;
    auto sd = validateRatio(split_risk_delta, "HazardRule split_risk_delta"); if (sd.is_error()) return sd;
    auto feedback = validateSafeKey(feedback_key, "HazardRule feedback_key"); if (feedback.is_error()) return feedback;
    if (containsDangerForbiddenKey(feedback_text)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "HazardRule feedback_text forbidden"));
    auto knowledge = validateSafeKey(knowledge_effect_key, "HazardRule knowledge_effect_key", false); if (knowledge.is_error()) return knowledge;
    for (const auto& ref : condition_refs) { auto r = ref.validateBasic(); if (r.is_error()) return r; }
    for (const auto& requirement : countermeasure_requirements) { auto r = requirement.validateBasic(); if (r.is_error()) return r; }
    auto tags = validateSafeKeys(safe_tags, "HazardRule safe_tags"); if (tags.is_error()) return tags;
    return Result<void>::ok();
}

Result<void> HazardExposureInput::validateBasic() const {
    auto key = validateSafeKey(input_key, "HazardExposureInput input_key"); if (key.is_error()) return key;
    if (actor_id.empty() || !isValidIdString(actor_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "HazardExposureInput actor_id invalid"));
    if (!tribe_id.empty() && !isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "HazardExposureInput tribe_id invalid"));
    if (trigger_kind == HazardTriggerKind::Unknown || trigger_kind == HazardTriggerKind::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardExposureInput trigger_kind invalid"));
    for (const auto& threat : threats) { auto r = threat.validateBasic(); if (r.is_error()) return r; }
    for (const auto& object : available_objects) { auto r = object.validateBasic(); if (r.is_error()) return r; }
    for (const auto& claim : actor_knowledge_claims) { auto r = claim.validateBasic(); if (r.is_error()) return r; }
    for (const auto& claim : tribe_shared_claims) { auto r = claim.validateBasic(); if (r.is_error()) return r; }
    if (tribe_projection) { auto r = tribe_projection->validateBasic(); if (r.is_error()) return r; }
    if (coordination_projection) { auto r = coordination_projection->validateBasic(); if (r.is_error()) return r; }
    return validateSafeKeys(safe_context_keys, "HazardExposureInput safe_context_keys");
}

Result<void> InjuryStateDraft::validateBasic() const {
    if (actor_id.empty() || !isValidIdString(actor_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "InjuryStateDraft actor_id invalid"));
    if (severity == DangerSeverity::Unknown || severity == DangerSeverity::TestOnly || severity == DangerSeverity::None) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "InjuryStateDraft severity invalid"));
    auto key = validateSafeKey(injury_key, "InjuryStateDraft injury_key"); if (key.is_error()) return key;
    auto pd = validateRatio(pressure_delta, "InjuryStateDraft pressure_delta"); if (pd.is_error()) return pd;
    return validateSafeKeys(reason_keys, "InjuryStateDraft reason_keys");
}

Result<void> FearSignal::validateBasic() const {
    if (kind == FearSignalKind::Unknown || kind == FearSignalKind::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "FearSignal kind invalid"));
    auto fp = validateRatio(fear_pressure, "FearSignal fear_pressure"); if (fp.is_error()) return fp;
    auto conf = validateRatio(confidence, "FearSignal confidence"); if (conf.is_error()) return conf;
    return validateSafeKeys(reason_keys, "FearSignal reason_keys");
}

Result<void> DangerFeedbackDraft::validateBasic() const {
    auto key = validateSafeKey(feedback_key, "DangerFeedbackDraft feedback_key"); if (key.is_error()) return key;
    if (safe_text.empty() || containsDangerForbiddenKey(safe_text)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "DangerFeedbackDraft safe_text invalid"));
    if (visible_severity == DangerSeverity::Unknown || visible_severity == DangerSeverity::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "DangerFeedbackDraft visible_severity invalid"));
    auto warnings = validateSafeKeys(warning_keys, "DangerFeedbackDraft warning_keys"); if (warnings.is_error()) return warnings;
    return validateSafeKeys(reason_keys, "DangerFeedbackDraft reason_keys");
}

Result<void> DangerKnowledgeDraft::validateBasic() const {
    auto k = validateSafeKey(knowledge_key, "DangerKnowledgeDraft knowledge_key"); if (k.is_error()) return k;
    auto s = validateSafeKey(subject_id, "DangerKnowledgeDraft subject_id"); if (s.is_error()) return s;
    auto a = validateSafeKey(action_key, "DangerKnowledgeDraft action_key"); if (a.is_error()) return a;
    auto e = validateSafeKey(effect_key, "DangerKnowledgeDraft effect_key"); if (e.is_error()) return e;
    if (severity == DangerSeverity::Unknown || severity == DangerSeverity::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "DangerKnowledgeDraft severity invalid"));
    for (const auto& condition : conditions) { auto r = condition.validateBasic(); if (r.is_error()) return r; }
    return validateSafeKeys(reason_keys, "DangerKnowledgeDraft reason_keys");
}

Result<void> TribeDangerPressureDraft::validateBasic() const {
    if (!tribe_id.empty() && !isValidIdString(tribe_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "TribeDangerPressureDraft tribe_id invalid"));
    auto md = validateDelta(morale_delta, "TribeDangerPressureDraft morale_delta"); if (md.is_error()) return md;
    auto td = validateDelta(trust_delta, "TribeDangerPressureDraft trust_delta"); if (td.is_error()) return td;
    for (const auto& item : {std::pair<double, const char*>{safety_pressure, "safety_pressure"}, {resource_pressure, "resource_pressure"}, {knowledge_conflict_pressure, "knowledge_conflict_pressure"}, {casualty_pressure, "casualty_pressure"}, {split_risk_hint, "split_risk_hint"}}) {
        auto r = validateRatio(item.first, std::string("TribeDangerPressureDraft ") + item.second); if (r.is_error()) return r;
    }
    return validateSafeKeys(reason_keys, "TribeDangerPressureDraft reason_keys");
}

Result<void> HazardTrace::validateBasic() const {
    auto key = validateSafeKey(trace_key, "HazardTrace trace_key"); if (key.is_error()) return key;
    if (decision == DangerDecision::Unknown || decision == DangerDecision::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardTrace decision invalid"));
    if (final_severity == DangerSeverity::Unknown || final_severity == DangerSeverity::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardTrace final_severity invalid"));
    for (const auto& keys : {matched_rule_keys, blocked_rule_keys, applied_countermeasure_keys, condition_trace_keys, reason_keys}) {
        auto r = validateSafeKeys(keys, "HazardTrace keys"); if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

Result<void> HazardResolutionResult::validateBasic() const {
    if (decision == DangerDecision::Unknown || decision == DangerDecision::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardResolutionResult decision invalid"));
    if (severity == DangerSeverity::Unknown || severity == DangerSeverity::TestOnly) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "HazardResolutionResult severity invalid"));
    for (const auto& item : injury_drafts) { auto r = item.validateBasic(); if (r.is_error()) return r; }
    for (const auto& item : fear_signals) { auto r = item.validateBasic(); if (r.is_error()) return r; }
    for (const auto& item : feedbacks) { auto r = item.validateBasic(); if (r.is_error()) return r; }
    for (const auto& item : knowledge_drafts) { auto r = item.validateBasic(); if (r.is_error()) return r; }
    for (const auto& item : tribe_pressure_drafts) { auto r = item.validateBasic(); if (r.is_error()) return r; }
    for (const auto& item : event_drafts) { auto r = item.validateBasic(); if (r.is_error()) return r; }
    return trace.validateBasic();
}

Result<void> CountermeasureAssessment::validateBasic() const {
    auto score = validateRatio(mitigation_score, "CountermeasureAssessment mitigation_score"); if (score.is_error()) return score;
    auto applied = validateSafeKeys(applied_countermeasure_keys, "CountermeasureAssessment applied_countermeasure_keys"); if (applied.is_error()) return applied;
    return validateSafeKeys(reason_keys, "CountermeasureAssessment reason_keys");
}

} // namespace pathfinder::danger
