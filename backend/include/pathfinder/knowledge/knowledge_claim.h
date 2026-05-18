#pragma once

#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/memory/memory_record.h"
#include "pathfinder/memory/memory_summary.h"
#include "pathfinder/foundation/id.h"
#include <optional>
#include <vector>

namespace pathfinder::knowledge {

// ============================================================
// DTOs
// ============================================================

struct KnowledgeOwner {
    KnowledgeOwnerKind kind = KnowledgeOwnerKind::Unknown;
    pathfinder::foundation::EntityId entity_id;
    pathfinder::foundation::TribeId tribe_id;
    pathfinder::foundation::RegionId region_id;
    std::string external_key;

    bool operator==(const KnowledgeOwner& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeSubject {
    KnowledgeSubjectKind kind = KnowledgeSubjectKind::Unknown;
    std::string subject_id;
    std::string subject_type_key;
    std::vector<std::string> related_subject_ids;
    std::string relation_group_key;
    std::vector<std::string> safe_tags;

    bool operator==(const KnowledgeSubject& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePredicate {
    KnowledgeRelationType relation_type = KnowledgeRelationType::Unknown;
    pathfinder::foundation::ActionId action_id;
    std::string action_key;
    std::string effect_key;
    std::vector<pathfinder::foundation::EffectId> effect_ids;
    std::vector<pathfinder::foundation::TraitId> trait_ids;
    std::string polarity_key;
    std::string value_summary_key;
    std::vector<std::string> safe_tags;

    bool operator==(const KnowledgePredicate& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeCondition {
    std::string condition_key;
    pathfinder::condition::ConditionExpressionRef condition_ref;
    std::string canonical_condition_key;
    std::string condition_summary_key;
    std::vector<pathfinder::foundation::ExpressionId> expression_ids;
    pathfinder::foundation::RegionId region_id;
    std::string time_scope_key;
    std::vector<std::string> object_state_keys;
    std::vector<std::string> actor_requirement_keys;

    bool operator==(const KnowledgeCondition& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

pathfinder::foundation::Result<KnowledgeCondition> normalizeKnowledgeCondition(const KnowledgeCondition& condition);
pathfinder::foundation::Result<std::vector<KnowledgeCondition>> normalizeKnowledgeConditions(const std::vector<KnowledgeCondition>& conditions);
pathfinder::foundation::Result<std::string> canonicalKnowledgeConditionKey(const KnowledgeCondition& condition);

struct KnowledgeEvidence {
    KnowledgeEvidenceId evidence_id;
    KnowledgeEvidenceKind evidence_kind = KnowledgeEvidenceKind::Unknown;
    pathfinder::memory::MemorySummaryId source_summary_id;
    std::vector<pathfinder::foundation::MemoryId> source_memory_ids;
    std::vector<pathfinder::memory::MemoryEvidenceRef> memory_evidence_refs;
    std::vector<pathfinder::foundation::EventId> source_event_ids;
    bool supports_claim = true;
    double confidence_delta = 0.0;
    double reliability = 1.0;
    pathfinder::foundation::Tick observed_tick;
    std::vector<std::string> reason_keys;

    bool operator==(const KnowledgeEvidence& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeConfidence {
    double confidence = 0.0;
    KnowledgeConfidenceBand band = KnowledgeConfidenceBand::Unknown;
    size_t support_count = 0;
    size_t conflict_count = 0;
    size_t source_memory_count = 0;
    size_t source_summary_count = 0;
    size_t long_term_source_count = 0;
    std::string last_change_reason_key;
    double last_delta = 0.0;
    pathfinder::foundation::Tick last_verified_tick;
    pathfinder::foundation::Tick last_contradicted_tick;

    bool operator==(const KnowledgeConfidence& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeTeachingProfile {
    bool teachable = false;
    double required_confidence = 0.75;
    bool risk_warning_required = false;
    std::string teaching_message_key;
    std::vector<pathfinder::foundation::KnowledgeId> prerequisite_knowledge_ids;
    std::vector<std::string> reason_keys;

    bool operator==(const KnowledgeTeachingProfile& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeProjectionFlags {
    bool visible_to_player = true;
    bool usable_by_ai = true;
    bool usable_for_action = false;
    bool usable_for_teaching = false;
    bool usable_for_civilization = false;
    bool debug_only = false;

    bool operator==(const KnowledgeProjectionFlags& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeClaim {
    pathfinder::foundation::KnowledgeId knowledge_id;
    std::string schema_version = "knowledge_claim.v1";
    KnowledgeOwner owner;
    KnowledgeSubject subject;
    KnowledgePredicate predicate;
    std::vector<KnowledgeCondition> conditions;
    KnowledgeConfidence confidence;
    KnowledgeStatus status = KnowledgeStatus::Unknown;
    std::vector<KnowledgeEvidence> evidence_refs;
    std::vector<pathfinder::foundation::KnowledgeId> related_knowledge_ids;
    std::vector<pathfinder::foundation::KnowledgeId> supersedes_knowledge_ids;
    pathfinder::foundation::KnowledgeId superseded_by_knowledge_id;
    KnowledgeTeachingProfile teaching_profile;
    KnowledgeProjectionFlags projection_flags;
    pathfinder::foundation::Tick created_tick;
    pathfinder::foundation::Tick updated_tick;
    std::vector<KnowledgeTraceId> trace_ids;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool operator==(const KnowledgeClaim& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeFormationOptions {
    bool allow_test_only = false;
    size_t min_source_memory_count = 3;
    size_t min_representative_count = 1;
    double min_summary_strength = 0.55;
    double min_hypothesis_confidence = 0.35;
    double active_confidence_threshold = 0.55;
    double teachable_confidence_threshold = 0.75;
    size_t max_evidence_refs = 50;
    bool allow_contradiction_summary = false;
    bool allow_risk_knowledge = true;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeFormationInput {
    KnowledgeOwner owner;
    pathfinder::memory::MemorySummary summary;
    std::vector<pathfinder::memory::MemoryRecord> representative_records;
    KnowledgeRelationType target_relation = KnowledgeRelationType::Unknown;
    std::string action_key;
    std::string effect_key;
    std::vector<KnowledgeCondition> candidate_conditions;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeFormationPlan {
    std::string plan_key;
    KnowledgeFormationInput input;
    KnowledgeSubject subject;
    KnowledgePredicate predicate;
    std::vector<KnowledgeEvidence> evidence_refs;
    KnowledgeConfidence projected_confidence;
    KnowledgeStatus projected_status = KnowledgeStatus::Unknown;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeEventDraft {
    std::string event_key;
    pathfinder::foundation::KnowledgeId knowledge_id;
    KnowledgeOwner owner;
    KnowledgeSubject subject;
    KnowledgeRelationType relation_type = KnowledgeRelationType::Unknown;
    KnowledgeStatus status = KnowledgeStatus::Unknown;
    KnowledgeFormationDecision decision = KnowledgeFormationDecision::Unknown;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeStateChangeDraft {
    std::string change_key;
    pathfinder::foundation::KnowledgeId knowledge_id;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeFormationResult {
    KnowledgeFormationDecision decision = KnowledgeFormationDecision::Unknown;
    std::optional<KnowledgeFormationPlan> plan;
    std::optional<KnowledgeClaim> claim;
    std::vector<KnowledgeEventDraft> event_drafts;
    std::vector<KnowledgeStateChangeDraft> state_changes;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// Services
// ============================================================

class KnowledgeIdFactory {
public:
    pathfinder::foundation::Result<pathfinder::foundation::KnowledgeId> makeKnowledgeId(
        const KnowledgeOwner& owner,
        const KnowledgeSubject& subject,
        const KnowledgePredicate& predicate,
        pathfinder::foundation::Tick created_tick,
        size_t sequence_index) const;

    pathfinder::foundation::Result<KnowledgeEvidenceId> makeEvidenceId(
        const pathfinder::foundation::KnowledgeId& knowledge_id,
        size_t evidence_index) const;

    pathfinder::foundation::Result<KnowledgeTraceId> makeTraceId(
        const pathfinder::foundation::KnowledgeId& knowledge_id,
        size_t trace_index) const;
};

} // namespace pathfinder::knowledge
