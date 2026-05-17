#pragma once

#include "pathfinder/knowledge/knowledge_claim.h"
#include <unordered_map>
#include <optional>

namespace pathfinder::knowledge {

// ============================================================
// KnowledgeQuery
// ============================================================

struct KnowledgeQuery {
    KnowledgeOwner owner;
    KnowledgeQueryMode mode = KnowledgeQueryMode::Unknown;
    std::optional<KnowledgeSubject> subject;
    std::optional<KnowledgeRelationType> relation_type;
    std::optional<KnowledgeStatus> min_status;
    std::optional<KnowledgeConfidenceBand> min_confidence_band;
    bool include_teachable = true;
    bool include_inactive = false;
    bool include_deprecated = false;
    bool include_disproven = false;
    size_t limit = 20;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// KnowledgeRepository
// ============================================================

class KnowledgeRepository {
public:
    pathfinder::foundation::Result<void> put(KnowledgeClaim claim);
    pathfinder::foundation::Result<std::optional<KnowledgeClaim>> find(
        const pathfinder::foundation::KnowledgeId& knowledge_id) const;
    pathfinder::foundation::Result<std::vector<KnowledgeClaim>> query(
        const KnowledgeQuery& query) const;
    pathfinder::foundation::Result<std::vector<KnowledgeClaim>> listByOwner(
        const KnowledgeOwner& owner) const;
    pathfinder::foundation::Result<void> remove(const pathfinder::foundation::KnowledgeId& knowledge_id);
    size_t size() const;
    void clear();

private:
    std::unordered_map<std::string, KnowledgeClaim> claims_;
};

} // namespace pathfinder::knowledge
