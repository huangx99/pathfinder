#pragma once

#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_repository.h"

namespace pathfinder::knowledge {

// ============================================================
// KnowledgeFormationPlanner
// ============================================================

class KnowledgeFormationPlanner {
public:
    pathfinder::foundation::Result<KnowledgeFormationPlan> planFromMemorySummary(
        const KnowledgeFormationInput& input,
        const KnowledgeFormationOptions& options = {}) const;
};

// ============================================================
// KnowledgeFormationService
// ============================================================

class KnowledgeFormationService {
public:
    pathfinder::foundation::Result<KnowledgeFormationResult> formFromMemorySummary(
        const KnowledgeFormationInput& input,
        const KnowledgeFormationOptions& options = {}) const;
};

} // namespace pathfinder::knowledge
