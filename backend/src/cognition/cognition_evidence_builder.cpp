#include "pathfinder/cognition/cognition_evidence_builder.h"
#include <algorithm>

namespace pathfinder::cognition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// fromFeedbackSignal
// ============================================================

Result<std::vector<CognitionEvidenceRecord>> CognitionEvidenceBuilder::fromFeedbackSignal(
    const CognitionFeedbackSignal& signal) const {

    auto vr = signal.validateBasic();
    if (vr.is_error()) {
        return Result<std::vector<CognitionEvidenceRecord>>::fail(vr.errors()[0]);
    }

    std::vector<CognitionEvidenceRecord> records;
    size_t index = 0;

    auto make_base = [&]() -> CognitionEvidenceRecord {
        CognitionEvidenceRecord rec;
        rec.key.subject = signal.subject;
        rec.key.target = signal.target;
        rec.key.action_id = signal.action_id;
        rec.key.action_context = signal.action_context;
        rec.source_event_id = signal.source_event_id;
        rec.observed_tick = signal.observed_tick;
        rec.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        return rec;
    };

    auto add_record = [&](CognitionAspect aspect, CognitionEvidenceSupport support,
                          CognitionOutcomeSignal outcome, CognitionRiskLevel risk,
                          double confidence_weight, double utility_signal,
                          const std::vector<std::string>& reason_keys) {
        auto rec = make_base();
        rec.evidence_id = makeEvidenceRecordId(signal.source_event_id, aspect, index++);
        rec.key.aspect = aspect;
        rec.support = support;
        rec.outcome_signal = outcome;
        rec.observed_risk = risk;
        rec.confidence_weight = confidence_weight;
        rec.utility_signal = utility_signal;
        rec.reason_keys = reason_keys;
        records.push_back(std::move(rec));
    };

    switch (signal.action_context) {
        case CognitionActionContextKind::Eat: {
            switch (signal.outcome_signal) {
                case CognitionOutcomeSignal::NeedImproved:
                    add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"eat_need_improved", "edibility_positive"});
                    add_record(CognitionAspect::ConsumptionEffect, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"consumption_effect_positive"});
                    add_record(CognitionAspect::Risk, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.25, 0.3,
                               {"risk_none_observed"});
                    add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.30, 0.6,
                               {"utility_positive"});
                    break;
                case CognitionOutcomeSignal::NeedWorsened:
                    add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::Low,
                               0.35, -0.5,
                               {"eat_need_worsened", "edibility_negative"});
                    add_record(CognitionAspect::ConsumptionEffect, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::Low,
                               0.35, -0.5,
                               {"consumption_effect_negative"});
                    add_record(CognitionAspect::Risk, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::Low,
                               0.30, -0.4,
                               {"risk_low_observed"});
                    add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::Low,
                               0.25, -0.6,
                               {"utility_negative"});
                    break;
                case CognitionOutcomeSignal::HealthImproved:
                    add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"eat_health_improved", "edibility_positive"});
                    add_record(CognitionAspect::ConsumptionEffect, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"consumption_effect_positive"});
                    add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.30, 0.6,
                               {"utility_positive"});
                    break;
                case CognitionOutcomeSignal::HealthWorsened:
                case CognitionOutcomeSignal::DamageTaken:
                    add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::High,
                               0.35, -0.7,
                               {"eat_health_worsened", "edibility_negative"});
                    add_record(CognitionAspect::ConsumptionEffect, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::High,
                               0.35, -0.7,
                               {"consumption_effect_negative"});
                    add_record(CognitionAspect::Risk, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::High,
                               0.40, -0.8,
                               {"risk_high_observed"});
                    add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::High,
                               0.30, -0.8,
                               {"utility_negative"});
                    break;
                case CognitionOutcomeSignal::ObjectConsumed:
                    add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.20, 0.1,
                               {"object_consumed"});
                    break;
                case CognitionOutcomeSignal::NoVisibleEffect:
                    add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Neutral,
                               signal.outcome_signal, CognitionRiskLevel::Unknown,
                               0.15, 0.0,
                               {"no_visible_effect"});
                    break;
                default:
                    // Unhandled outcome for Eat context: create a single generic record
                    add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Neutral,
                               signal.outcome_signal, signal.risk_level,
                               0.15, signal.utility_signal,
                               {"eat_generic"});
                    break;
            }
            break;
        }
        case CognitionActionContextKind::Use: {
            switch (signal.outcome_signal) {
                case CognitionOutcomeSignal::ToolActivated:
                    add_record(CognitionAspect::Usability, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"use_tool_activated", "usability_positive"});
                    add_record(CognitionAspect::UseEffect, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"use_effect_positive"});
                    add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.30, 0.6,
                               {"utility_positive"});
                    break;
                case CognitionOutcomeSignal::NewObjectProduced:
                    add_record(CognitionAspect::Usability, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"use_new_object_produced", "usability_positive"});
                    add_record(CognitionAspect::UseEffect, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.35, 0.5,
                               {"use_effect_positive"});
                    add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::None,
                               0.30, 0.6,
                               {"utility_positive"});
                    break;
                case CognitionOutcomeSignal::NoVisibleEffect:
                    add_record(CognitionAspect::Usability, CognitionEvidenceSupport::Refutes,
                               signal.outcome_signal, CognitionRiskLevel::Unknown,
                               0.20, -0.1,
                               {"use_no_visible_effect", "usability_negative"});
                    break;
                case CognitionOutcomeSignal::DamageTaken:
                    add_record(CognitionAspect::Risk, CognitionEvidenceSupport::Supports,
                               signal.outcome_signal, CognitionRiskLevel::High,
                               0.35, -0.6,
                               {"use_damage_taken", "risk_high"});
                    break;
                default:
                    add_record(CognitionAspect::Usability, CognitionEvidenceSupport::Neutral,
                               signal.outcome_signal, signal.risk_level,
                               0.15, signal.utility_signal,
                               {"use_generic"});
                    break;
            }
            break;
        }
        case CognitionActionContextKind::Observe: {
            if (signal.outcome_signal == CognitionOutcomeSignal::DangerObserved) {
                add_record(CognitionAspect::Risk, CognitionEvidenceSupport::Supports,
                           signal.outcome_signal, CognitionRiskLevel::High,
                           0.30, -0.5,
                           {"danger_observed", "risk_positive"});
            } else {
                add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Neutral,
                           signal.outcome_signal, signal.risk_level,
                           0.15, signal.utility_signal,
                           {"observe_generic"});
            }
            break;
        }
        default: {
            // Generic fallback for other contexts
            add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Neutral,
                       signal.outcome_signal, signal.risk_level,
                       0.15, signal.utility_signal,
                       {"generic_feedback"});
            break;
        }
    }

    return Result<std::vector<CognitionEvidenceRecord>>::ok(std::move(records));
}

// ============================================================
// fromLegacyEvidence
// ============================================================

Result<std::vector<CognitionEvidenceRecord>> CognitionEvidenceBuilder::fromLegacyEvidence(
    const CognitionEvidence& legacy_evidence) const {

    auto vr = legacy_evidence.validateBasic();
    if (vr.is_error()) {
        return Result<std::vector<CognitionEvidenceRecord>>::fail(vr.errors()[0]);
    }

    std::vector<CognitionEvidenceRecord> records;
    size_t index = 0;

    auto add_record = [&](CognitionAspect aspect, CognitionEvidenceSupport support,
                          CognitionOutcomeSignal outcome, CognitionRiskLevel risk,
                          double confidence_weight, double utility_signal,
                          const std::vector<std::string>& reason_keys) {
        CognitionEvidenceRecord rec;
        rec.evidence_id = makeEvidenceRecordId(legacy_evidence.source_event_id, aspect, index++);
        rec.key.subject.kind = CognitionSubjectKind::Actor;
        rec.key.subject.subject_id = legacy_evidence.key.subject_id;
        rec.key.target.kind = CognitionTargetKind::ObjectDefinition; // hidden_truth: safe enum usage
        rec.key.target.target_id = legacy_evidence.key.object_definition_id.value();
        rec.key.action_id = legacy_evidence.key.action_id;
        rec.key.action_context = CognitionActionContextKind::Eat; // Legacy was eat-only in P3
        rec.key.aspect = aspect;
        rec.source_kind = CognitionEvidenceSourceKind::DirectActionFeedback;
        rec.support = support;
        rec.outcome_signal = outcome;
        rec.observed_risk = risk;
        rec.confidence_weight = confidence_weight;
        rec.utility_signal = utility_signal;
        rec.source_event_id = legacy_evidence.source_event_id;
        rec.reason_keys = reason_keys;
        // legacy hidden_truth: do NOT copy observed_hunger_delta / observed_health_delta into reason_keys
        records.push_back(std::move(rec));
    };

    // Map legacy effect_kind and observed deltas to multiple V2 evidence records
    bool is_edible = (legacy_evidence.key.effect_kind == CognitionEffectKind::Edible);
    bool is_harmful = (legacy_evidence.key.effect_kind == CognitionEffectKind::Harmful);
    bool hunger_changed = (legacy_evidence.key.effect_kind == CognitionEffectKind::HungerChanged);
    bool health_changed = (legacy_evidence.key.effect_kind == CognitionEffectKind::HealthChanged);
    bool hunger_negative = legacy_evidence.observed_hunger_delta < 0;
    bool hunger_positive = legacy_evidence.observed_hunger_delta > 0;
    bool health_negative = legacy_evidence.observed_health_delta < 0;
    bool health_positive = legacy_evidence.observed_health_delta > 0;

    double base_confidence = std::clamp(std::abs(legacy_evidence.confidence_delta), 0.0, 1.0);

    // Edibility aspect
    if (is_edible || hunger_negative) {
        add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Supports,
                   CognitionOutcomeSignal::NeedImproved, CognitionRiskLevel::None,
                   base_confidence, 0.5,
                   {"legacy_edible_observed"});
    } else if (is_harmful || hunger_positive) {
        add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Refutes,
                   CognitionOutcomeSignal::NeedWorsened, CognitionRiskLevel::Low,
                   base_confidence, -0.5,
                   {"legacy_edibility_negative"});
    }

    // ConsumptionEffect aspect
    if (hunger_negative || health_positive) {
        add_record(CognitionAspect::ConsumptionEffect, CognitionEvidenceSupport::Supports,
                   hunger_negative ? CognitionOutcomeSignal::NeedImproved : CognitionOutcomeSignal::HealthImproved,
                   CognitionRiskLevel::None,
                   base_confidence, 0.3,
                   {"legacy_consumption_effect_positive"});
    } else if (hunger_positive || health_negative) {
        add_record(CognitionAspect::ConsumptionEffect, CognitionEvidenceSupport::Refutes,
                   hunger_positive ? CognitionOutcomeSignal::NeedWorsened : CognitionOutcomeSignal::HealthWorsened,
                   CognitionRiskLevel::Low,
                   base_confidence, -0.3,
                   {"legacy_consumption_effect_negative"});
    } else if (hunger_changed || health_changed) {
        add_record(CognitionAspect::ConsumptionEffect, CognitionEvidenceSupport::Supports,
                   CognitionOutcomeSignal::NeedImproved, CognitionRiskLevel::Unknown,
                   base_confidence, 0.3,
                   {"legacy_consumption_effect_observed"});
    }

    // Risk aspect
    if (is_harmful || health_negative) {
        add_record(CognitionAspect::Risk, CognitionEvidenceSupport::Supports,
                   CognitionOutcomeSignal::HealthWorsened, CognitionRiskLevel::High,
                   base_confidence, -0.7,
                   {"legacy_risk_observed"});
    } else if (is_edible || hunger_negative || health_positive) {
        add_record(CognitionAspect::Risk, CognitionEvidenceSupport::Refutes,
                   CognitionOutcomeSignal::NeedImproved, CognitionRiskLevel::None,
                   base_confidence * 0.7, 0.2,
                   {"legacy_risk_low"});
    }

    // Utility aspect
    if (is_edible || hunger_negative || health_positive) {
        add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Supports,
                   CognitionOutcomeSignal::NeedImproved, CognitionRiskLevel::None,
                   base_confidence * 0.8, 0.5,
                   {"legacy_utility_positive"});
    } else if (is_harmful || hunger_positive || health_negative) {
        add_record(CognitionAspect::Utility, CognitionEvidenceSupport::Refutes,
                   CognitionOutcomeSignal::HealthWorsened, CognitionRiskLevel::High,
                   base_confidence * 0.8, -0.7,
                   {"legacy_utility_negative"});
    }

    if (records.empty()) {
        // Fallback for truly unknown
        add_record(CognitionAspect::Edibility, CognitionEvidenceSupport::Neutral,
                   CognitionOutcomeSignal::NoVisibleEffect, CognitionRiskLevel::Unknown,
                   base_confidence * 0.5, 0.0,
                   {"legacy_unknown_effect"});
    }

    return Result<std::vector<CognitionEvidenceRecord>>::ok(std::move(records));
}

} // namespace pathfinder::cognition
