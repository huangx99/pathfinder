#include "pathfinder/memory/memory_writer.h"
#include <algorithm>
#include <cctype>

namespace pathfinder::memory {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::MemoryId;
using pathfinder::foundation::Tick;
using namespace pathfinder::cognition;

// ============================================================
// Explicit enum mappings (BLOCKER-2)
// ============================================================

static Result<MemoryOwnerKind> mapCognitionSubjectKindToMemoryOwnerKind(CognitionSubjectKind kind) {
    switch (kind) {
        case CognitionSubjectKind::Actor:   return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Actor);
        case CognitionSubjectKind::Agent:   return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Agent);
        case CognitionSubjectKind::Group:   return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Group);
        case CognitionSubjectKind::Tribe:   return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Tribe);
        case CognitionSubjectKind::Civilization:
            return Result<MemoryOwnerKind>::ok(MemoryOwnerKind::Civilization);
        default:
            return Result<MemoryOwnerKind>::fail(
                makeError(ErrorCode::validation_enum_unknown, "Unsupported CognitionSubjectKind for memory"));
    }
}

static Result<MemorySubjectKind> mapCognitionTargetKindToMemorySubjectKind(CognitionTargetKind kind) {
    switch (kind) {
        case CognitionTargetKind::ObjectDefinition:
            return Result<MemorySubjectKind>::ok(MemorySubjectKind::ObjectDefinition);
        case CognitionTargetKind::WorldObject:
            return Result<MemorySubjectKind>::ok(MemorySubjectKind::WorldObject);
        case CognitionTargetKind::ObjectCategory:
            return Result<MemorySubjectKind>::ok(MemorySubjectKind::ObjectCategory);
        case CognitionTargetKind::AgentSpecies:
            return Result<MemorySubjectKind>::ok(MemorySubjectKind::AgentSpecies);
        case CognitionTargetKind::EnvironmentFeature:
            return Result<MemorySubjectKind>::ok(MemorySubjectKind::EnvironmentFeature);
        default:
            return Result<MemorySubjectKind>::fail(
                makeError(ErrorCode::validation_enum_unknown, "Unsupported CognitionTargetKind for memory"));
    }
}

// ============================================================
// Helpers
// ============================================================

static MemoryKind outcomeToMemoryKind(CognitionOutcomeSignal signal) {
    switch (signal) {
        case CognitionOutcomeSignal::NeedImproved:
        case CognitionOutcomeSignal::HealthImproved:
        case CognitionOutcomeSignal::ToolActivated:
        case CognitionOutcomeSignal::NewObjectProduced:
            return MemoryKind::Success;
        case CognitionOutcomeSignal::NeedWorsened:
        case CognitionOutcomeSignal::HealthWorsened:
        case CognitionOutcomeSignal::NoVisibleEffect:
            return MemoryKind::Failure;
        case CognitionOutcomeSignal::DamageTaken:
            return MemoryKind::Accident;
        case CognitionOutcomeSignal::DangerObserved:
            return MemoryKind::Warning;
        case CognitionOutcomeSignal::ObjectConsumed:
            return MemoryKind::Experiment;
        default:
            return MemoryKind::Experiment;
    }
}

static MemoryImportance riskToImportance(CognitionRiskLevel risk) {
    switch (risk) {
        case CognitionRiskLevel::None:
        case CognitionRiskLevel::Low:
            return MemoryImportance::Normal;
        case CognitionRiskLevel::Medium:
            return MemoryImportance::Important;
        case CognitionRiskLevel::High:
        case CognitionRiskLevel::Critical:
            return MemoryImportance::Critical;
        default:
            return MemoryImportance::Normal;
    }
}

static bool isHighRisk(CognitionRiskLevel risk) {
    return risk == CognitionRiskLevel::High || risk == CognitionRiskLevel::Critical;
}

static bool isCriticalRisk(CognitionRiskLevel risk) {
    return risk == CognitionRiskLevel::Critical;
}

// ============================================================
// MemoryCandidateFactory
// ============================================================

Result<MemoryCandidate> MemoryCandidateFactory::fromCognitionEvidence(
    const CognitionEvidenceRecord& evidence) const {

    // BLOCKER-1: validate evidence first
    auto evidence_validation = evidence.validateBasic();
    if (evidence_validation.is_error()) {
        return Result<MemoryCandidate>::fail(evidence_validation.errors());
    }

    // BLOCKER-2: explicit mapping, reject Unknown/TestOnly
    auto owner_kind_result = mapCognitionSubjectKindToMemoryOwnerKind(evidence.key.subject.kind);
    if (owner_kind_result.is_error()) {
        return Result<MemoryCandidate>::fail(owner_kind_result.errors());
    }
    auto subject_kind_result = mapCognitionTargetKindToMemorySubjectKind(evidence.key.target.kind);
    if (subject_kind_result.is_error()) {
        return Result<MemoryCandidate>::fail(subject_kind_result.errors());
    }

    MemoryCandidate candidate;
    candidate.candidate_id = "candidate_" + evidence.evidence_id.value();

    candidate.owner.kind = owner_kind_result.value();
    candidate.owner.entity_id = evidence.key.subject.subject_id;

    candidate.subject.kind = subject_kind_result.value();
    candidate.subject.subject_id = evidence.key.target.target_id;
    candidate.subject.subject_type_key = evidence.key.target.public_label_key;

    candidate.memory_kinds.push_back(outcomeToMemoryKind(evidence.outcome_signal));
    if (evidence.support == CognitionEvidenceSupport::Conflicts) {
        candidate.memory_kinds.push_back(MemoryKind::Contradiction);
    }

    candidate.source_kind = MemorySourceKind::CognitionEvidence;
    candidate.importance = riskToImportance(evidence.observed_risk);
    candidate.initial_strength = evidence.confidence_weight;

    // BLOCKER-5: High/Critical risk统一保护
    candidate.protect_from_decay = isHighRisk(evidence.observed_risk);
    candidate.protect_from_forgetting = isCriticalRisk(evidence.observed_risk);
    candidate.teaching_candidate = (evidence.confidence_weight >= 0.70);

    candidate.evidence_ref.source_kind = MemorySourceKind::CognitionEvidence;
    candidate.evidence_ref.cognition_evidence_id = evidence.evidence_id;
    candidate.evidence_ref.source_event_id = evidence.source_event_id;
    candidate.evidence_ref.observed_tick = evidence.observed_tick;
    candidate.evidence_ref.reason_keys = evidence.reason_keys;

    candidate.reason_keys = evidence.reason_keys;
    candidate.warning_keys = evidence.warning_keys;

    // BLOCKER-1: factory validates candidate before returning
    auto candidate_validation = candidate.validateBasic();
    if (candidate_validation.is_error()) {
        return Result<MemoryCandidate>::fail(candidate_validation.errors());
    }
    return Result<MemoryCandidate>::ok(std::move(candidate));
}

Result<MemoryCandidate> MemoryCandidateFactory::fromCognitionUpdate(
    const CognitionUpdateResult& update_result,
    const CognitionEvidenceRecord& evidence) const {

    auto evidence_validation = evidence.validateBasic();
    if (evidence_validation.is_error()) {
        return Result<MemoryCandidate>::fail(evidence_validation.errors());
    }
    auto update_validation = update_result.validateBasic();
    if (update_validation.is_error()) {
        return Result<MemoryCandidate>::fail(update_validation.errors());
    }

    auto candidate_result = fromCognitionEvidence(evidence);
    if (candidate_result.is_error()) {
        return candidate_result;
    }
    auto candidate = candidate_result.value();
    candidate.candidate_id = "candidate_update_" + evidence.evidence_id.value();
    candidate.source_kind = MemorySourceKind::CognitionUpdate;
    candidate.evidence_ref.source_kind = MemorySourceKind::CognitionUpdate;

    // BLOCKER-6: Discovery/TeachingRelated/Contradiction mapping
    if (update_result.decision == CognitionUpdateDecision::Created) {
        if (std::find(candidate.memory_kinds.begin(), candidate.memory_kinds.end(), MemoryKind::Discovery)
            == candidate.memory_kinds.end()) {
            candidate.memory_kinds.push_back(MemoryKind::Discovery);
        }
    }
    if (update_result.decision == CognitionUpdateDecision::Conflicted) {
        if (std::find(candidate.memory_kinds.begin(), candidate.memory_kinds.end(), MemoryKind::Contradiction)
            == candidate.memory_kinds.end()) {
            candidate.memory_kinds.push_back(MemoryKind::Contradiction);
        }
    }
    if (update_result.after_claim.confidence_band == CognitionConfidenceBand::Teachable) {
        candidate.teaching_candidate = true;
        if (std::find(candidate.memory_kinds.begin(), candidate.memory_kinds.end(), MemoryKind::TeachingRelated)
            == candidate.memory_kinds.end()) {
            candidate.memory_kinds.push_back(MemoryKind::TeachingRelated);
        }
    }

    auto candidate_validation = candidate.validateBasic();
    if (candidate_validation.is_error()) {
        return Result<MemoryCandidate>::fail(candidate_validation.errors());
    }
    return Result<MemoryCandidate>::ok(std::move(candidate));
}

// ============================================================
// MemoryIdFactory (ISSUE-3: return Result<MemoryId>)
// ============================================================

Result<MemoryId> MemoryIdFactory::makeMemoryId(
    const MemoryOwner& owner,
    const MemorySubject& subject,
    const MemoryEvidenceRef& evidence_ref,
    size_t sequence_index) const {

    auto owner_validation = owner.validateBasic();
    if (owner_validation.is_error()) {
        return Result<MemoryId>::fail(owner_validation.errors());
    }
    auto subject_validation = subject.validateBasic();
    if (subject_validation.is_error()) {
        return Result<MemoryId>::fail(subject_validation.errors());
    }
    auto ref_validation = evidence_ref.validateBasic();
    if (ref_validation.is_error()) {
        return Result<MemoryId>::fail(ref_validation.errors());
    }

    std::string owner_key;
    if (owner.kind == MemoryOwnerKind::Actor || owner.kind == MemoryOwnerKind::Agent) {
        owner_key = owner.entity_id.value();
    } else if (owner.kind == MemoryOwnerKind::Tribe) {
        owner_key = owner.tribe_id.value();
    } else if (owner.kind == MemoryOwnerKind::TestOnly) {
        owner_key = owner.external_key;
    } else {
        return Result<MemoryId>::fail(
            makeError(ErrorCode::validation_failed, "Unsupported MemoryOwnerKind for MemoryId"));
    }

    std::string tick_str = std::to_string(evidence_ref.observed_tick.value());
    std::string id_str = "mem_" + owner_key + "_" + subject.subject_id + "_" + tick_str + "_" + std::to_string(sequence_index);
    return Result<MemoryId>::ok(MemoryId(id_str));
}

// ============================================================
// MemoryWriteService
// ============================================================

static bool isSimilarMemory(const MemoryCandidate& candidate, const MemoryRecord& record) {
    if (!(candidate.owner == record.owner)) return false;
    if (!(candidate.subject == record.subject)) return false;
    if (record.lifecycle == MemoryLifecycle::Forgotten) return false;

    // BLOCKER-RECHECK-1: contradiction candidate can match same owner+subject record
    // even if record does not have Contradiction, because conflict targets the old memory
    bool candidate_has_contradiction = std::find(candidate.memory_kinds.begin(), candidate.memory_kinds.end(), MemoryKind::Contradiction) != candidate.memory_kinds.end();
    if (candidate_has_contradiction) {
        return true;
    }

    // memory_kinds intersection
    bool has_intersection = false;
    for (const auto& ck : candidate.memory_kinds) {
        for (const auto& rk : record.memory_kinds) {
            if (ck == rk) {
                has_intersection = true;
                break;
            }
        }
        if (has_intersection) break;
    }
    if (!has_intersection) return false;

    return true;
}

static MemoryRecord candidateToRecord(const MemoryCandidate& candidate, const MemoryId& memory_id, Tick tick) {
    MemoryRecord record;
    record.memory_id = memory_id;
    record.owner = candidate.owner;
    record.subject = candidate.subject;
    record.memory_kinds = candidate.memory_kinds;
    record.scope = MemoryScope::ShortTerm;
    record.lifecycle = MemoryLifecycle::Active;
    record.importance = candidate.importance;
    record.strength = candidate.initial_strength > 0.0 ? candidate.initial_strength : importanceToInitialStrength(candidate.importance);
    record.strength_band = strengthToBand(record.strength);
    record.reinforcement_count = 0;
    record.contradiction_count = 0;
    record.protect_from_decay = candidate.protect_from_decay;
    record.protect_from_forgetting = candidate.protect_from_forgetting;
    record.teaching_candidate = candidate.teaching_candidate;
    record.created_tick = tick;
    record.last_touched_tick = tick;
    record.evidence_refs.push_back(candidate.evidence_ref);
    record.reason_keys = candidate.reason_keys;
    record.warning_keys = candidate.warning_keys;
    return record;
}

static bool shouldPromoteShortToMid(const MemoryRecord& record, const MemoryWriteOptions& options) {
    if (record.scope != MemoryScope::ShortTerm) return false;
    if (record.strength >= options.short_to_mid_strength_threshold) return true;
    if (record.reinforcement_count >= options.short_to_mid_reinforcement_count) return true;
    if (record.importance == MemoryImportance::Critical) return true;
    for (const auto& mk : record.memory_kinds) {
        if (mk == MemoryKind::Accident || mk == MemoryKind::Warning || mk == MemoryKind::Discovery ||
            mk == MemoryKind::Contradiction || mk == MemoryKind::TeachingRelated) {
            return true;
        }
    }
    return false;
}

static bool shouldPromoteMidToLong(const MemoryRecord& record, const MemoryWriteOptions& options) {
    if (record.scope != MemoryScope::MidTerm) return false;
    if (record.strength >= options.mid_to_long_strength_threshold) return true;
    if (record.reinforcement_count >= options.mid_to_long_reinforcement_count) return true;
    if (record.importance == MemoryImportance::Critical && record.protect_from_forgetting) return true;
    for (const auto& mk : record.memory_kinds) {
        if (mk == MemoryKind::Accident || mk == MemoryKind::Contradiction) {
            return true;
        }
    }
    return false;
}

Result<MemoryWriteResult> MemoryWriteService::writeCandidate(
    const MemoryCandidate& candidate,
    const std::vector<MemoryRecord>& existing_records,
    const MemoryWriteOptions& options) const {

    MemoryWriteResult result;

    // Validate candidate
    auto candidate_validation = candidate.validateBasic();
    if (candidate_validation.is_error()) {
        result.decision = MemoryWriteDecision::Rejected;
        MemoryWriteIssue issue;
        issue.reason = MemoryRejectReason::EmptyCandidateId;
        issue.issue_key = "candidate_validation_failed";
        result.issues.push_back(issue);
        return Result<MemoryWriteResult>::ok(std::move(result));
    }

    // TestOnly gate
    if (!options.allow_test_only) {
        if (candidate.owner.kind == MemoryOwnerKind::TestOnly ||
            candidate.subject.kind == MemorySubjectKind::TestOnly) {
            result.decision = MemoryWriteDecision::Rejected;
            MemoryWriteIssue issue;
            issue.reason = MemoryRejectReason::UnknownOwnerKind;
            issue.issue_key = "test_only_not_allowed";
            result.issues.push_back(issue);
            return Result<MemoryWriteResult>::ok(std::move(result));
        }
    }

    // Validate options
    auto options_validation = options.validateBasic();
    if (options_validation.is_error()) {
        result.decision = MemoryWriteDecision::Rejected;
        MemoryWriteIssue issue;
        issue.reason = MemoryRejectReason::SchemaVersionUnsupported;
        issue.issue_key = "options_validation_failed";
        result.issues.push_back(issue);
        return Result<MemoryWriteResult>::ok(std::move(result));
    }

    // Find similar existing memory
    const MemoryRecord* best_match = nullptr;
    for (const auto& record : existing_records) {
        if (isSimilarMemory(candidate, record)) {
            if (!best_match || record.last_touched_tick.value() > best_match->last_touched_tick.value() ||
                (record.last_touched_tick.value() == best_match->last_touched_tick.value() && record.memory_id.value() < best_match->memory_id.value())) {
                best_match = &record;
            }
        }
    }

    if (best_match) {
        // Reinforce existing
        MemoryRecord updated = *best_match;
        result.before_record = *best_match;

        double reinforce_gain = importanceToReinforceGain(candidate.importance);
        updated.strength = updated.strength + (1.0 - updated.strength) * reinforce_gain;
        if (updated.strength > 1.0) updated.strength = 1.0;
        updated.strength_band = strengthToBand(updated.strength);
        updated.reinforcement_count += 1;
        updated.last_touched_tick = candidate.evidence_ref.observed_tick;

        // Append evidence ref if not full
        if (updated.evidence_refs.size() < options.max_evidence_refs_per_record) {
            updated.evidence_refs.push_back(candidate.evidence_ref);
        }

        // Handle contradiction
        if (std::find(candidate.memory_kinds.begin(), candidate.memory_kinds.end(), MemoryKind::Contradiction) != candidate.memory_kinds.end()) {
            updated.contradiction_count += 1;
            if (std::find(updated.memory_kinds.begin(), updated.memory_kinds.end(), MemoryKind::Contradiction) == updated.memory_kinds.end()) {
                updated.memory_kinds.push_back(MemoryKind::Contradiction);
            }
            updated.reason_keys.push_back("memory.contradiction_observed");
        }

        // BLOCKER-5: High/Critical reinforcement should protect/upgrade
        if (candidate.importance == MemoryImportance::Critical) {
            updated.protect_from_decay = true;
            updated.protect_from_forgetting = true;
            updated.lifecycle = MemoryLifecycle::Protected;
        } else if (candidate.protect_from_decay && !updated.protect_from_decay) {
            updated.protect_from_decay = true;
        }
        if (candidate.protect_from_forgetting) {
            updated.protect_from_forgetting = true;
        }

        // Promotion checks
        bool promoted = false;
        if (shouldPromoteShortToMid(updated, options)) {
            if (updated.scope == MemoryScope::ShortTerm) {
                updated.scope = MemoryScope::MidTerm;
                promoted = true;
            }
        }
        if (shouldPromoteMidToLong(updated, options)) {
            if (updated.scope == MemoryScope::MidTerm) {
                updated.scope = MemoryScope::LongTerm;
                promoted = true;
            }
        }

        result.after_record = updated;
        result.updated_records.push_back(updated);

        if (promoted) {
            result.decision = MemoryWriteDecision::Promoted;
        } else {
            result.decision = MemoryWriteDecision::Reinforced;
        }

        // Event draft
        MemoryEventDraft event;
        event.event_key = promoted ? "memory.promoted" : "memory.reinforced";
        event.memory_id = updated.memory_id;
        event.owner = updated.owner;
        event.subject = updated.subject;
        event.scope = updated.scope;
        event.lifecycle = updated.lifecycle;
        event.write_decision = result.decision;
        result.event_drafts.push_back(event);

        // State change draft
        MemoryStateChangeDraft change;
        change.change_key = promoted ? "memory.promote" : "memory.reinforce";
        change.memory_id = updated.memory_id;
        change.before_record = *best_match;
        change.after_record = updated;
        result.state_changes.push_back(change);

    } else {
        // Create new memory
        MemoryIdFactory id_factory;
        auto id_result = id_factory.makeMemoryId(candidate.owner, candidate.subject, candidate.evidence_ref, 0);
        if (id_result.is_error()) {
            result.decision = MemoryWriteDecision::Rejected;
            MemoryWriteIssue issue;
            issue.reason = MemoryRejectReason::EmptyMemoryId;
            issue.issue_key = "memory_id_generation_failed";
            result.issues.push_back(issue);
            return Result<MemoryWriteResult>::ok(std::move(result));
        }
        MemoryId memory_id = id_result.value();
        MemoryRecord record = candidateToRecord(candidate, memory_id, candidate.evidence_ref.observed_tick);

        // Protection
        if (candidate.protect_from_decay) {
            record.lifecycle = MemoryLifecycle::Protected;
        }

        // Promotion for critical / important
        if (record.importance == MemoryImportance::Critical) {
            record.scope = MemoryScope::LongTerm;
            record.protect_from_decay = true;
            record.protect_from_forgetting = true;
        } else if (shouldPromoteShortToMid(record, options)) {
            record.scope = MemoryScope::MidTerm;
        }

        record.strength_band = strengthToBand(record.strength);

        result.after_record = record;
        result.created_records.push_back(record);
        result.decision = MemoryWriteDecision::Created;

        // Event draft
        MemoryEventDraft event;
        event.event_key = "memory.created";
        event.memory_id = record.memory_id;
        event.owner = record.owner;
        event.subject = record.subject;
        event.scope = record.scope;
        event.lifecycle = record.lifecycle;
        event.write_decision = MemoryWriteDecision::Created;
        result.event_drafts.push_back(event);

        // State change draft
        MemoryStateChangeDraft change;
        change.change_key = "memory.create";
        change.memory_id = record.memory_id;
        change.after_record = record;
        result.state_changes.push_back(change);
    }

    return Result<MemoryWriteResult>::ok(std::move(result));
}

} // namespace pathfinder::memory
