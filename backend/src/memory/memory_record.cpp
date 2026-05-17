#include "pathfinder/memory/memory_record.h"
#include <algorithm>

namespace pathfinder::memory {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// MemoryOwner
// ============================================================

Result<void> MemoryOwner::validateBasic() const {
    if (kind == MemoryOwnerKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryOwner kind is Unknown"));
    }
    if (kind == MemoryOwnerKind::Actor || kind == MemoryOwnerKind::Agent) {
        if (entity_id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryOwner Actor/Agent requires entity_id"));
        }
    }
    if (kind == MemoryOwnerKind::Tribe) {
        if (tribe_id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryOwner Tribe requires tribe_id"));
        }
    }
    if (kind == MemoryOwnerKind::TestOnly) {
        if (external_key.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryOwner TestOnly requires external_key"));
        }
    }
    return Result<void>::ok();
}

// ============================================================
// MemorySubject
// ============================================================

Result<void> MemorySubject::validateBasic() const {
    if (kind == MemorySubjectKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySubject kind is Unknown"));
    }
    if (subject_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySubject subject_id is empty"));
    }
    if (containsMemoryForbiddenKey(subject_id)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySubject subject_id contains forbidden key"));
    }
    if (containsMemoryForbiddenKey(subject_type_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySubject subject_type_key contains forbidden key"));
    }
    if (containsMemoryForbiddenKey(safe_tags)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemorySubject safe_tags contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryEvidenceRef (BLOCKER-3: require traceable source)
// ============================================================

Result<void> MemoryEvidenceRef::validateBasic() const {
    if (source_kind == MemorySourceKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEvidenceRef source_kind is Unknown"));
    }
    // For cognition sources, must have evidence id or event id
    if (source_kind == MemorySourceKind::CognitionEvidence || source_kind == MemorySourceKind::CognitionUpdate) {
        if (cognition_evidence_id.empty() && source_event_id.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEvidenceRef missing cognition_evidence_id or source_event_id"));
        }
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEvidenceRef reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryCandidate
// ============================================================

Result<void> MemoryCandidate::validateBasic() const {
    if (candidate_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCandidate candidate_id is empty"));
    }
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    if (memory_kinds.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCandidate memory_kinds is empty"));
    }
    for (const auto& mk : memory_kinds) {
        if (mk == MemoryKind::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCandidate contains Unknown memory kind"));
        }
    }
    if (source_kind == MemorySourceKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCandidate source_kind is Unknown"));
    }
    if (importance == MemoryImportance::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCandidate importance is Unknown"));
    }
    if (initial_strength < 0.0 || initial_strength > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryCandidate initial_strength out of range"));
    }
    auto evidence_result = evidence_ref.validateBasic();
    if (evidence_result.is_error()) return evidence_result;
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCandidate reason_keys contain forbidden key"));
    }
    if (containsMemoryForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryCandidate warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryRecord (BLOCKER-3: require at least one evidence ref)
// ============================================================

Result<void> MemoryRecord::validateBasic() const {
    if (memory_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord memory_id is empty"));
    }
    if (schema_version != "memory_record.v1") {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord schema_version unsupported"));
    }
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    if (memory_kinds.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord memory_kinds is empty"));
    }
    for (const auto& mk : memory_kinds) {
        if (mk == MemoryKind::Unknown) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord contains Unknown memory kind"));
        }
    }
    if (scope == MemoryScope::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord scope is Unknown"));
    }
    if (lifecycle == MemoryLifecycle::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord lifecycle is Unknown"));
    }
    if (importance == MemoryImportance::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord importance is Unknown"));
    }
    if (strength < 0.0 || strength > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryRecord strength out of range"));
    }
    // BLOCKER-3: must have at least one evidence ref
    if (evidence_refs.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord evidence_refs is empty"));
    }
    for (const auto& ref : evidence_refs) {
        auto ref_result = ref.validateBasic();
        if (ref_result.is_error()) return ref_result;
    }
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord reason_keys contain forbidden key"));
    }
    if (containsMemoryForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryRecord warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryWriteOptions (ISSUE-2: threshold ordering)
// ============================================================

Result<void> MemoryWriteOptions::validateBasic() const {
    if (max_candidates == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryWriteOptions max_candidates is 0"));
    }
    if (max_evidence_refs_per_record == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryWriteOptions max_evidence_refs_per_record is 0"));
    }
    if (duplicate_match_threshold < 0.0 || duplicate_match_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryWriteOptions duplicate_match_threshold out of range"));
    }
    if (short_to_mid_strength_threshold < 0.0 || short_to_mid_strength_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryWriteOptions short_to_mid_strength_threshold out of range"));
    }
    if (mid_to_long_strength_threshold < 0.0 || mid_to_long_strength_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryWriteOptions mid_to_long_strength_threshold out of range"));
    }
    if (short_to_mid_strength_threshold > mid_to_long_strength_threshold) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryWriteOptions short_to_mid_strength_threshold > mid_to_long_strength_threshold"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryDecayOptions (ISSUE-2: threshold ordering)
// ============================================================

Result<void> MemoryDecayOptions::validateBasic() const {
    if (max_records == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryDecayOptions max_records is 0"));
    }
    if (short_term_decay_per_tick < 0.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryDecayOptions short_term_decay_per_tick negative"));
    }
    if (mid_term_decay_per_tick < 0.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryDecayOptions mid_term_decay_per_tick negative"));
    }
    if (long_term_decay_per_tick < 0.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryDecayOptions long_term_decay_per_tick negative"));
    }
    if (fading_threshold < 0.0 || fading_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryDecayOptions fading_threshold out of range"));
    }
    if (forgotten_threshold < 0.0 || forgotten_threshold > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryDecayOptions forgotten_threshold out of range"));
    }
    if (forgotten_threshold > fading_threshold) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "MemoryDecayOptions forgotten_threshold > fading_threshold"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryWriteIssue
// ============================================================

Result<void> MemoryWriteIssue::validateBasic() const {
    if (issue_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryWriteIssue issue_key is empty"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryEventDraft (ISSUE-1: stronger validation)
// ============================================================

Result<void> MemoryEventDraft::validateBasic() const {
    if (event_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEventDraft event_key is empty"));
    }
    if (memory_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEventDraft memory_id is empty"));
    }
    if (scope == MemoryScope::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEventDraft scope is Unknown"));
    }
    if (lifecycle == MemoryLifecycle::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEventDraft lifecycle is Unknown"));
    }
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    if (containsMemoryForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEventDraft reason_keys contain forbidden key"));
    }
    if (containsMemoryForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryEventDraft warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryStateChangeDraft (ISSUE-1: stronger validation)
// ============================================================

Result<void> MemoryStateChangeDraft::validateBasic() const {
    if (change_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryStateChangeDraft change_key is empty"));
    }
    if (memory_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryStateChangeDraft memory_id is empty"));
    }
    if (before_record.has_value()) {
        auto r = before_record->validateBasic();
        if (r.is_error()) return r;
    }
    if (after_record.has_value()) {
        auto r = after_record->validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryWriteResult
// ============================================================

bool MemoryWriteResult::ok() const {
    return decision != MemoryWriteDecision::Unknown && decision != MemoryWriteDecision::Rejected;
}

Result<void> MemoryWriteResult::validateBasic() const {
    if (decision == MemoryWriteDecision::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryWriteResult decision is Unknown"));
    }
    for (const auto& record : created_records) {
        auto r = record.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& record : updated_records) {
        auto r = record.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& issue : issues) {
        auto r = issue.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& draft : event_drafts) {
        auto r = draft.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& change : state_changes) {
        auto r = change.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryDecayResult
// ============================================================

bool MemoryDecayResult::ok() const {
    return decision != MemoryDecayDecision::Unknown;
}

Result<void> MemoryDecayResult::validateBasic() const {
    if (decision == MemoryDecayDecision::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryDecayResult decision is Unknown"));
    }
    for (const auto& record : updated_records) {
        auto r = record.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& draft : event_drafts) {
        auto r = draft.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& change : state_changes) {
        auto r = change.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& issue : issues) {
        auto r = issue.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryQuery
// ============================================================

Result<void> MemoryQuery::validateBasic() const {
    auto owner_result = owner.validateBasic();
    if (owner_result.is_error()) return owner_result;
    if (subject.has_value()) {
        auto subject_result = subject->validateBasic();
        if (subject_result.is_error()) return subject_result;
    }
    if (limit == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "MemoryQuery limit is 0"));
    }
    return Result<void>::ok();
}

// ============================================================
// MemoryQueryResult
// ============================================================

Result<void> MemoryQueryResult::validateBasic() const {
    auto query_result = query.validateBasic();
    if (query_result.is_error()) return query_result;
    for (const auto& record : records) {
        auto r = record.validateBasic();
        if (r.is_error()) return r;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::memory
