#pragma once

#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_repository.h"

namespace pathfinder::knowledge {

// ============================================================
// KnowledgeProjectionItem
// ============================================================

struct KnowledgeProjectionItem {
    pathfinder::foundation::KnowledgeId knowledge_id;
    KnowledgeSubject subject;
    KnowledgePredicate predicate;
    std::vector<KnowledgeCondition> conditions;
    KnowledgeConfidence confidence;
    KnowledgeStatus status = KnowledgeStatus::Unknown;
    KnowledgeTeachingProfile teaching_profile;
    KnowledgeProjectionFlags projection_flags;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// KnowledgeProjection
// ============================================================

struct KnowledgeProjection {
    KnowledgeOwner owner;
    std::vector<KnowledgeProjectionItem> items;
    bool truncated = false;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// KnowledgeProjectionBuilder
// ============================================================

class KnowledgeProjectionBuilder {
public:
    pathfinder::foundation::Result<KnowledgeProjection> buildProjection(
        const std::vector<KnowledgeClaim>& claims,
        const KnowledgeQuery& query) const;
};

} // namespace pathfinder::knowledge
