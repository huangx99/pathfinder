#include "pathfinder/knowledge/knowledge_claim.h"
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
    if (kind == KnowledgeOwnerKind::ExternalRecord || kind == KnowledgeOwnerKind::TestOnly) {
        if (external_key.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeOwner ExternalRecord/TestOnly requires external_key"));
        }
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
    if (subject_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject subject_id is empty"));
    }
    if (containsKnowledgeForbiddenKey(subject_id)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject subject_id contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(subject_type_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeSubject subject_type_key contains forbidden key"));
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
    if (action_id.empty() && action_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate requires action_id or action_key"));
    }
    if (containsKnowledgeForbiddenKey(effect_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgePredicate effect_key contains forbidden key"));
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
    if (condition_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition condition_key is empty"));
    }
    if (containsKnowledgeForbiddenKey(condition_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeCondition condition_key contains forbidden key"));
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
    return Result<void>::ok();
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
    if (source_summary_id.empty() && source_memory_ids.empty() && memory_evidence_refs.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEvidence requires at least one source trace"));
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
    if (status == KnowledgeStatus::Deprecated || status == KnowledgeStatus::Disproven) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaim P18 cannot create Deprecated/Disproven"));
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
    if (active_confidence_threshold < 0.0 || active_confidence_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeFormationOptions active_confidence_threshold out of range"));
    }
    if (teachable_confidence_threshold < 0.0 || teachable_confidence_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeFormationOptions teachable_confidence_threshold out of range"));
    }
    if (teachable_confidence_threshold < active_confidence_threshold) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationOptions teachable_confidence_threshold < active_confidence_threshold"));
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
    if (containsKnowledgeForbiddenKey(action_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput action_key contains forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(effect_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationInput effect_key contains forbidden key"));
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
    if (projected_status == KnowledgeStatus::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeFormationPlan projected_status is Unknown"));
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
    if (knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeEventDraft knowledge_id is empty"));
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
