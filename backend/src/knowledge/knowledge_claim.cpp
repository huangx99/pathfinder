#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/condition/condition_normalizer.h"
#include "pathfinder/condition/condition_summary.h"
#include "pathfinder/foundation/id.h"
#include <algorithm>
#include <sstream>

namespace pathfinder::knowledge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;

// ============================================================
// KnowledgeOwner
// ============================================================

Result<void> KnowledgeOwner::validateBasic() const {
    if (kind == KnowledgeOwnerKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner kind is Unknown"));
    }
    if (kind == KnowledgeOwnerKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner TestOnly not allowed"));
    }
    if (kind == KnowledgeOwnerKind::Agent || kind == KnowledgeOwnerKind::Actor) {
        if (entity_id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner Agent/Actor requires entity_id"));
        }
    }
    if (kind == KnowledgeOwnerKind::Tribe) {
        if (tribe_id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner Tribe requires tribe_id"));
        }
    }
    if (kind == KnowledgeOwnerKind::Region) {
        if (region_id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner Region requires region_id"));
        }
    }
    if (kind == KnowledgeOwnerKind::ExternalRecord) {
        if (external_key.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner ExternalRecord requires external_key"));
        }
    }
    if (containsKnowledgeForbiddenKey(external_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner external_key contains forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeSubject
// ============================================================

Result<void> KnowledgeSubject::validateBasic() const {
    if (kind == KnowledgeSubjectKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject kind is Unknown"));
    }
    if (kind == KnowledgeSubjectKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject TestOnly not allowed"));
    }
    if (subject_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject subject_id is empty"));
    }
    if (containsKnowledgeForbiddenKey(subject_id)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject subject_id contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(subject_type_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject subject_type_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(related_subject_ids)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject related_subject_ids contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(relation_group_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject relation_group_key contains forbidden key"));
    }
    if (kind == KnowledgeSubjectKind::Combination) {
        if (related_subject_ids.empty() && relation_group_key.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject Combination requires related_subject_ids or relation_group_key"));
        }
    }
    if (containsKnowledgeForbiddenKey(safe_tags)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject safe_tags contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePredicate
// ============================================================

Result<void> KnowledgePredicate::validateBasic() const {
    if (relation_type == KnowledgeRelationType::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate relation_type is Unknown"));
    }
    if (relation_type == KnowledgeRelationType::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate relation_type TestOnly not allowed"));
    }
    if (action_id.empty() && action_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate requires action_id or action_key"));
    }
    if (containsKnowledgeForbiddenKey(action_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate action_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(effect_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate effect_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(polarity_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate polarity_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(value_summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate value_summary_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(safe_tags)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate safe_tags contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeCondition
// ============================================================

Result<void> KnowledgeCondition::validateBasic() const {
    if (condition_key.empty() && canonical_condition_key.empty() && condition_ref.empty() &&
        expression_ids.empty() && object_state_keys.empty() && actor_requirement_keys.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition condition identity is empty"));
    }
    if (containsKnowledgeForbiddenKey(condition_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition condition_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(canonical_condition_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition canonical_condition_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(condition_summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition condition_summary_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(time_scope_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition time_scope_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(object_state_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition object_state_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(actor_requirement_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition actor_requirement_keys contain forbidden key"));
    }
    if (!condition_ref.empty()) {
        auto ref_valid = condition_ref.validateBasic();
        if (ref_valid.is_error()) return ref_valid;
    }
    return Result<void>::ok();
}

Result<KnowledgeCondition> normalizeKnowledgeCondition(const KnowledgeCondition& condition) {
    auto valid = condition.validateBasic();
    if (valid.is_error()) return Result<KnowledgeCondition>::fail(valid.errors());

    KnowledgeCondition normalized = condition;
    pathfinder::condition::ConditionSummaryBuilder summary_builder;

    if (!normalized.canonical_condition_key.empty()) {
        if (normalized.condition_ref.empty()) {
            normalized.condition_ref.inline_canonical_key = normalized.canonical_condition_key;
            normalized.condition_ref.inline_legacy_condition = !normalized.condition_key.empty() && normalized.condition_key != normalized.canonical_condition_key;
        }
        if (normalized.condition_key.empty()) normalized.condition_key = normalized.canonical_condition_key;
        if (normalized.condition_summary_key.empty()) {
            auto summary = summary_builder.summarizeCanonicalKey(normalized.canonical_condition_key);
            if (summary.is_error()) return Result<KnowledgeCondition>::fail(summary.errors());
            normalized.condition_summary_key = summary.value().summary_key;
        }
        auto normalized_valid = normalized.validateBasic();
        if (normalized_valid.is_error()) return Result<KnowledgeCondition>::fail(normalized_valid.errors());
        return Result<KnowledgeCondition>::ok(std::move(normalized));
    }

    if (!normalized.condition_ref.inline_canonical_key.empty()) {
        normalized.canonical_condition_key = normalized.condition_ref.inline_canonical_key;
        if (normalized.condition_key.empty()) normalized.condition_key = normalized.canonical_condition_key;
        if (normalized.condition_summary_key.empty()) {
            auto summary = summary_builder.summarizeCanonicalKey(normalized.canonical_condition_key);
            if (summary.is_error()) return Result<KnowledgeCondition>::fail(summary.errors());
            normalized.condition_summary_key = summary.value().summary_key;
        }
        auto normalized_valid = normalized.validateBasic();
        if (normalized_valid.is_error()) return Result<KnowledgeCondition>::fail(normalized_valid.errors());
        return Result<KnowledgeCondition>::ok(std::move(normalized));
    }

    if (!normalized.condition_ref.expression_id.empty()) {
        normalized.canonical_condition_key = "condition:expression_ref:eq:" + normalized.condition_ref.expression_id.value();
        if (normalized.condition_key.empty()) normalized.condition_key = normalized.canonical_condition_key;
        if (normalized.condition_summary_key.empty()) normalized.condition_summary_key = "condition.summary.expression_ref";
        auto normalized_valid = normalized.validateBasic();
        if (normalized_valid.is_error()) return Result<KnowledgeCondition>::fail(normalized_valid.errors());
        return Result<KnowledgeCondition>::ok(std::move(normalized));
    }

    if (!normalized.expression_ids.empty() && normalized.condition_key.empty() && normalized.object_state_keys.empty() && normalized.actor_requirement_keys.empty()) {
        normalized.condition_ref.expression_id = normalized.expression_ids.front();
        normalized.canonical_condition_key = "condition:expression_ref:eq:" + normalized.expression_ids.front().value();
        normalized.condition_key = normalized.canonical_condition_key;
        normalized.condition_summary_key = "condition.summary.expression_ref";
        auto normalized_valid = normalized.validateBasic();
        if (normalized_valid.is_error()) return Result<KnowledgeCondition>::fail(normalized_valid.errors());
        return Result<KnowledgeCondition>::ok(std::move(normalized));
    }

    pathfinder::condition::LegacyConditionInput input;
    input.condition_key = normalized.condition_key;
    input.object_state_keys = normalized.object_state_keys;
    input.actor_requirement_keys = normalized.actor_requirement_keys;

    pathfinder::condition::ConditionNormalizer normalizer;
    auto result = normalizer.normalizeLegacyInput(input);
    if (result.is_error()) return Result<KnowledgeCondition>::fail(result.errors());

    normalized.condition_ref = result.value().expression_ref;
    normalized.canonical_condition_key = result.value().canonical_condition_key;
    if (normalized.condition_key.empty()) normalized.condition_key = normalized.canonical_condition_key;
    if (normalized.condition_summary_key.empty()) normalized.condition_summary_key = result.value().summary_key;

    auto normalized_valid = normalized.validateBasic();
    if (normalized_valid.is_error()) return Result<KnowledgeCondition>::fail(normalized_valid.errors());
    return Result<KnowledgeCondition>::ok(std::move(normalized));
}

Result<std::vector<KnowledgeCondition>> normalizeKnowledgeConditions(const std::vector<KnowledgeCondition>& conditions) {
    std::vector<KnowledgeCondition> normalized;
    normalized.reserve(conditions.size());
    for (const auto& condition : conditions) {
        auto result = normalizeKnowledgeCondition(condition);
        if (result.is_error()) return Result<std::vector<KnowledgeCondition>>::fail(result.errors());
        normalized.push_back(std::move(result.value()));
    }
    return Result<std::vector<KnowledgeCondition>>::ok(std::move(normalized));
}

Result<std::string> canonicalKnowledgeConditionKey(const KnowledgeCondition& condition) {
    auto normalized = normalizeKnowledgeCondition(condition);
    if (normalized.is_error()) return Result<std::string>::fail(normalized.errors());
    return Result<std::string>::ok(normalized.value().canonical_condition_key);
}

// ============================================================
// KnowledgeEvidence
// ============================================================

Result<void> KnowledgeEvidence::validateBasic() const {
    if (evidence_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEvidence evidence_id is empty"));
    }
    if (!isValidIdString(evidence_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEvidence evidence_id format invalid"));
    }
    if (evidence_kind == KnowledgeEvidenceKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEvidence evidence_kind is Unknown"));
    }
    if (evidence_kind == KnowledgeEvidenceKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEvidence evidence_kind TestOnly not allowed"));
    }
    if (source_summary_id.empty() && source_memory_ids.empty() && memory_evidence_refs.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEvidence requires at least one source trace"));
    }
    for (const auto& ref : memory_evidence_refs) {
        auto ref_result = ref.validateBasic();
        if (ref_result.is_error()) return ref_result;
    }
    if (confidence_delta < -1.0 || confidence_delta > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeEvidence confidence_delta out of range"));
    }
    if (reliability < 0.0 || reliability > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeEvidence reliability out of range"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEvidence reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeConfidence
// ============================================================

Result<void> KnowledgeConfidence::validateBasic() const {
    if (confidence < 0.0 || confidence > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeConfidence confidence out of range"));
    }
    auto expected_band = confidenceToBand(confidence);
    if (band != KnowledgeConfidenceBand::Unknown && band != expected_band) {
        // Allow slight mismatch for boundary values
        if (!(confidence >= 0.10 && confidence < 0.35 && band == KnowledgeConfidenceBand::Weak) &&
            !(confidence >= 0.35 && confidence < 0.55 && band == KnowledgeConfidenceBand::Plausible) &&
            !(confidence >= 0.55 && confidence < 0.75 && band == KnowledgeConfidenceBand::Reliable) &&
            !(confidence >= 0.75 && confidence < 0.90 && band == KnowledgeConfidenceBand::Strong) &&
            !(confidence >= 0.90 && confidence <= 1.00 && band == KnowledgeConfidenceBand::Established)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeConfidence band does not match confidence"));
        }
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeTeachingProfile
// ============================================================

Result<void> KnowledgeTeachingProfile::validateBasic() const {
    if (required_confidence < 0.0 || required_confidence > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeTeachingProfile required_confidence out of range"));
    }
    if (teachable && teaching_message_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeTeachingProfile teachable requires teaching_message_key"));
    }
    if (containsKnowledgeForbiddenKey(teaching_message_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeTeachingProfile teaching_message_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeTeachingProfile reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeProjectionFlags
// ============================================================

Result<void> KnowledgeProjectionFlags::validateBasic() const {
    if (debug_only) {
        if (usable_for_action || usable_for_teaching || usable_for_civilization) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeProjectionFlags debug_only cannot coexist with usable_for_action/teaching/civilization"));
        }
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeClaim
// ============================================================

Result<void> KnowledgeClaim::validateBasic() const {
    if (knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim knowledge_id is empty"));
    }
    if (!isValidIdString(knowledge_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim knowledge_id format invalid"));
    }
    if (schema_version != "knowledge_claim.v1") {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim schema_version unsupported"));
    }
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    auto predicate_result = predicate.validateBasic();
    if (predicate_result.is_error()) return predicate_result;
    auto confidence_result = confidence.validateBasic();
    if (confidence_result.is_error()) return confidence_result;

    if (status == KnowledgeStatus::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim status is Unknown"));
    }
    if (status == KnowledgeStatus::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim status TestOnly not allowed"));
    }
    if (status == KnowledgeStatus::Deprecated || status == KnowledgeStatus::Disproven) {
        // P19 allows Deprecated/Disproven with stricter evidence requirements
        if (status == KnowledgeStatus::Deprecated) {
            if (superseded_by_knowledge_id.empty()) {
                bool has_valid_reason = false;
                for (const auto& rk : reason_keys) {
                    if (rk.find("deprecated_by_specialization") != std::string::npos ||
                        rk.find("deprecated_by_revision") != std::string::npos) {
                        has_valid_reason = true;
                        break;
                    }
                }
                if (!has_valid_reason) {
                    return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim Deprecated requires superseded_by_knowledge_id or valid reason"));
                }
            }
            if (evidence_refs.empty()) {
                return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim Deprecated requires evidence_refs"));
            }
        }
        if (status == KnowledgeStatus::Disproven) {
            if (confidence.conflict_count == 0) {
                return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim Disproven requires conflict_count > 0"));
            }
            bool has_negative_evidence = false;
            for (const auto& ev : evidence_refs) {
                if (!ev.supports_claim) {
                    has_negative_evidence = true;
                    break;
                }
            }
            if (!has_negative_evidence) {
                return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim Disproven requires negative evidence"));
            }
            bool has_valid_reason = false;
            for (const auto& rk : reason_keys) {
                if (rk.find("disproven_by_contradiction") != std::string::npos ||
                    rk.find("disproven_by_revision") != std::string::npos) {
                    has_valid_reason = true;
                    break;
                }
            }
            if (!has_valid_reason) {
                return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim Disproven requires valid reason_key"));
            }
        }
    }

    if (evidence_refs.empty()) {
        if (status == KnowledgeStatus::Active || status == KnowledgeStatus::Teachable) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim Active/Teachable requires at least one evidence_refs"));
        }
    } else {
        for (const auto& ev : evidence_refs) {
            auto ev_result = ev.validateBasic();
            if (ev_result.is_error()) return ev_result;
        }
    }

    if (status == KnowledgeStatus::Teachable) {
        auto tp_result = teaching_profile.validateBasic();
        if (tp_result.is_error()) return tp_result;
        if (!teaching_profile.teachable) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim Teachable requires teaching_profile.teachable=true"));
        }
    }

    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim warning_keys contain forbidden key"));
    }

    auto pf_result = projection_flags.validateBasic();
    if (pf_result.is_error()) return pf_result;

    return Result<void>::ok();
}

// ============================================================
// KnowledgeFormationOptions
// ============================================================

Result<void> KnowledgeFormationOptions::validateBasic() const {
    if (min_source_memory_count == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationOptions min_source_memory_count is 0"));
    }
    if (max_evidence_refs == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationOptions max_evidence_refs is 0"));
    }
    if (min_summary_strength < 0.0 || min_summary_strength > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeFormationOptions min_summary_strength out of range"));
    }
    if (min_hypothesis_confidence < 0.0 || min_hypothesis_confidence > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeFormationOptions min_hypothesis_confidence out of range"));
    }
    if (active_confidence_threshold < 0.0 || active_confidence_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeFormationOptions active_confidence_threshold out of range"));
    }
    if (teachable_confidence_threshold < 0.0 || teachable_confidence_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeFormationOptions teachable_confidence_threshold out of range"));
    }
    if (teachable_confidence_threshold < active_confidence_threshold) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationOptions teachable_confidence_threshold < active_confidence_threshold"));
    }
    if (active_confidence_threshold < min_hypothesis_confidence) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationOptions active_confidence_threshold < min_hypothesis_confidence"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeFormationInput
// ============================================================

Result<void> KnowledgeFormationInput::validateBasic() const {
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    auto summary_result = summary.validateBasic();
    if (summary_result.is_error()) return summary_result;

    // BLOCKER-3: owner must match summary.key.owner
    bool owner_match = false;
    if (owner.kind == KnowledgeOwnerKind::Agent && summary.key.owner.kind == pathfinder::memory::MemoryOwnerKind::Agent) {
        owner_match = (owner.entity_id == pathfinder::foundation::EntityId(summary.key.owner.entity_id.value()));
    } else if (owner.kind == KnowledgeOwnerKind::Actor && summary.key.owner.kind == pathfinder::memory::MemoryOwnerKind::Actor) {
        owner_match = (owner.entity_id == pathfinder::foundation::EntityId(summary.key.owner.entity_id.value()));
    } else if (owner.kind == KnowledgeOwnerKind::Tribe && summary.key.owner.kind == pathfinder::memory::MemoryOwnerKind::Tribe) {
        owner_match = (owner.tribe_id == pathfinder::foundation::TribeId(summary.key.owner.tribe_id.value()));
    }
    if (!owner_match) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput owner does not match summary.key.owner"));
    }

    if (containsKnowledgeForbiddenKey(action_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput action_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(effect_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput effect_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(related_subject_ids)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput related_subject_ids contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(relation_group_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput relation_group_key contains forbidden key"));
    }
    for (const auto& cond : candidate_conditions) {
        auto cond_result = cond.validateBasic();
        if (cond_result.is_error()) return cond_result;
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeFormationPlan
// ============================================================

Result<void> KnowledgeFormationPlan::validateBasic() const {
    if (plan_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationPlan plan_key is empty"));
    }
    if (containsKnowledgeForbiddenKey(plan_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationPlan plan_key contains forbidden key"));
    }
    auto input_result = input.validateBasic();
    if (input_result.is_error()) return input_result;
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    auto predicate_result = predicate.validateBasic();
    if (predicate_result.is_error()) return predicate_result;
    if (evidence_refs.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationPlan evidence_refs is empty"));
    }
    for (const auto& ev : evidence_refs) {
        auto ev_result = ev.validateBasic();
        if (ev_result.is_error()) return ev_result;
    }
    auto conf_result = projected_confidence.validateBasic();
    if (conf_result.is_error()) return conf_result;
    if (projected_status == KnowledgeStatus::Unknown ||
        projected_status == KnowledgeStatus::TestOnly ||
        projected_status == KnowledgeStatus::Deprecated ||
        projected_status == KnowledgeStatus::Disproven) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationPlan projected_status is invalid"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationPlan reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationPlan warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeEventDraft
// ============================================================

Result<void> KnowledgeEventDraft::validateBasic() const {
    if (event_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft event_key is empty"));
    }
    if (containsKnowledgeForbiddenKey(event_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft event_key contains forbidden key"));
    }
    if (knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft knowledge_id is empty"));
    }
    if (relation_type == KnowledgeRelationType::Unknown ||
        relation_type == KnowledgeRelationType::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft relation_type is invalid"));
    }
    if (status == KnowledgeStatus::Unknown || status == KnowledgeStatus::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft status is invalid"));
    }
    if (decision == KnowledgeFormationDecision::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft decision is Unknown"));
    }
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeStateChangeDraft
// ============================================================

Result<void> KnowledgeStateChangeDraft::validateBasic() const {
    if (change_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeStateChangeDraft change_key is empty"));
    }
    if (containsKnowledgeForbiddenKey(change_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeStateChangeDraft change_key contains forbidden key"));
    }
    if (knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeStateChangeDraft knowledge_id is empty"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeStateChangeDraft reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeFormationResult
// ============================================================

bool KnowledgeFormationResult::ok() const {
    return decision == KnowledgeFormationDecision::Skipped ||
           decision == KnowledgeFormationDecision::CreatedClaim ||
           decision == KnowledgeFormationDecision::UpdatedClaim;
}

Result<void> KnowledgeFormationResult::validateBasic() const {
    if (decision == KnowledgeFormationDecision::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationResult decision is Unknown"));
    }
    if (plan.has_value()) {
        auto r = plan->validateBasic();
        if (r.is_error()) return r;
    }
    if (claim.has_value()) {
        auto r = claim->validateBasic();
        if (r.is_error()) return r;
    }
    if (decision == KnowledgeFormationDecision::CreatedClaim || decision == KnowledgeFormationDecision::UpdatedClaim) {
        if (!claim.has_value()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationResult CreatedClaim/UpdatedClaim requires claim"));
        }
    }
    for (const auto& draft : event_drafts) {
        auto r = draft.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& change : state_changes) {
        auto r = change.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationResult reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationResult warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeIdFactory
// ============================================================

Result<pathfinder::foundation::KnowledgeId> KnowledgeIdFactory::makeKnowledgeId(
    const KnowledgeOwner& owner,
    const KnowledgeSubject& subject,
    const KnowledgePredicate& predicate,
    pathfinder::foundation::Tick created_tick,
    size_t sequence_index) const {

    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) {
        return Result<pathfinder::foundation::KnowledgeId>::fail(owner_result.errors());
    }
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) {
        return Result<pathfinder::foundation::KnowledgeId>::fail(subject_result.errors());
    }
    auto predicate_result = predicate.validateBasic();
    if (predicate_result.is_error()) {
        return Result<pathfinder::foundation::KnowledgeId>::fail(predicate_result.errors());
    }

    std::string owner_key;
    if (!owner.entity_id.empty()) owner_key = owner.entity_id.value();
    else if (!owner.tribe_id.empty()) owner_key = owner.tribe_id.value();
    else if (!owner.region_id.empty()) owner_key = owner.region_id.value();
    else owner_key = owner.external_key;

    std::string sanitized_subject = subject.subject_id;
    for (char& c : sanitized_subject) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        } else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            c = '_';
        }
    }

    std::ostringstream oss;
    oss << "know_"
        << owner_key << "_"
        << sanitized_subject << "_"
        << toString(predicate.relation_type) << "_"
        << created_tick.value() << "_"
        << sequence_index;

    std::string id_str = oss.str();
    if (!isValidIdString(id_str)) {
        return Result<pathfinder::foundation::KnowledgeId>::fail(
            makeError(ErrorCode::validation_failed, "KnowledgeIdFactory generated invalid id: " + id_str));
    }

    return Result<pathfinder::foundation::KnowledgeId>::ok(pathfinder::foundation::KnowledgeId(id_str));
}

Result<KnowledgeEvidenceId> KnowledgeIdFactory::makeEvidenceId(
    const pathfinder::foundation::KnowledgeId& knowledge_id,
    size_t evidence_index) const {

    if (knowledge_id.empty()) {
        return Result<KnowledgeEvidenceId>::fail(
            makeError(ErrorCode::validation_failed, "KnowledgeIdFactory makeEvidenceId knowledge_id is empty"));
    }

    std::ostringstream oss;
    oss << "kevd_" << knowledge_id.value() << "_" << evidence_index;
    std::string id_str = oss.str();
    if (!isValidIdString(id_str)) {
        return Result<KnowledgeEvidenceId>::fail(
            makeError(ErrorCode::validation_failed, "KnowledgeIdFactory generated invalid evidence id: " + id_str));
    }

    return Result<KnowledgeEvidenceId>::ok(KnowledgeEvidenceId(id_str));
}

Result<KnowledgeTraceId> KnowledgeIdFactory::makeTraceId(
    const pathfinder::foundation::KnowledgeId& knowledge_id,
    size_t trace_index) const {

    if (knowledge_id.empty()) {
        return Result<KnowledgeTraceId>::fail(
            makeError(ErrorCode::validation_failed, "KnowledgeIdFactory makeTraceId knowledge_id is empty"));
    }

    std::ostringstream oss;
    oss << "ktrace_" << knowledge_id.value() << "_" << trace_index;
    std::string id_str = oss.str();
    if (!isValidIdString(id_str)) {
        return Result<KnowledgeTraceId>::fail(
            makeError(ErrorCode::validation_failed, "KnowledgeIdFactory generated invalid trace id: " + id_str));
    }

    return Result<KnowledgeTraceId>::ok(KnowledgeTraceId(id_str));
}

} // namespace pathfinder::knowledge
