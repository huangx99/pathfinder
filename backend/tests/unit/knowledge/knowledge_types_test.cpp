#include "pathfinder/knowledge/knowledge_types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::knowledge;
using namespace pathfinder::foundation;

void run_knowledge_types_tests() {
    // ============================================================
    // Enum roundtrips
    // ============================================================
    {
        assert(toString(KnowledgeOwnerKind::Agent) == "agent");
        assert(knowledgeOwnerKindFromString("agent").value() == KnowledgeOwnerKind::Agent);
        assert(knowledgeOwnerKindFromString("invalid").is_error());
        assert(knowledgeOwnerKindFromString("unknown").value() == KnowledgeOwnerKind::Unknown);

        assert(toString(KnowledgeSubjectKind::ObjectDefinition) == "object_definition");
        assert(knowledgeSubjectKindFromString("object_definition").value() == KnowledgeSubjectKind::ObjectDefinition);
        assert(knowledgeSubjectKindFromString("invalid").is_error());

        assert(toString(KnowledgeRelationType::HasEffect) == "has_effect");
        assert(knowledgeRelationTypeFromString("has_effect").value() == KnowledgeRelationType::HasEffect);
        assert(knowledgeRelationTypeFromString("invalid").is_error());

        assert(toString(KnowledgeConfidenceBand::Reliable) == "reliable");
        assert(knowledgeConfidenceBandFromString("reliable").value() == KnowledgeConfidenceBand::Reliable);
        assert(knowledgeConfidenceBandFromString("invalid").is_error());

        assert(toString(KnowledgeStatus::Active) == "active");
        assert(knowledgeStatusFromString("active").value() == KnowledgeStatus::Active);
        assert(knowledgeStatusFromString("invalid").is_error());

        assert(toString(KnowledgeEvidenceKind::MemorySummary) == "memory_summary");
        assert(knowledgeEvidenceKindFromString("memory_summary").value() == KnowledgeEvidenceKind::MemorySummary);
        assert(knowledgeEvidenceKindFromString("invalid").is_error());

        assert(toString(KnowledgeFormationDecision::CreatedClaim) == "created_claim");
        assert(knowledgeFormationDecisionFromString("created_claim").value() == KnowledgeFormationDecision::CreatedClaim);
        assert(knowledgeFormationDecisionFromString("invalid").is_error());

        assert(toString(KnowledgeQueryMode::Teachable) == "teachable");
        assert(knowledgeQueryModeFromString("teachable").value() == KnowledgeQueryMode::Teachable);
        assert(knowledgeQueryModeFromString("invalid").is_error());
    }

    // ============================================================
    // Hidden truth guard
    // ============================================================
    {
        assert(containsKnowledgeForbiddenKey("edible_profile"));
        assert(containsKnowledgeForbiddenKey("Hunger_Delta"));
        assert(containsKnowledgeForbiddenKey("GAMESTATE"));
        assert(containsKnowledgeForbiddenKey("raw_state"));
        assert(!containsKnowledgeForbiddenKey("safe_summary"));
        assert(!containsKnowledgeForbiddenKey(""));

        std::vector<std::string> bad_keys = {"safe", "edible_profile"};
        assert(containsKnowledgeForbiddenKey(bad_keys));

        std::vector<std::string> good_keys = {"safe", "berry_red"};
        assert(!containsKnowledgeForbiddenKey(good_keys));
    }

    // ============================================================
    // confidenceToBand
    // ============================================================
    {
        assert(confidenceToBand(0.0) == KnowledgeConfidenceBand::Unknown);
        assert(confidenceToBand(0.09) == KnowledgeConfidenceBand::Unknown);
        assert(confidenceToBand(0.10) == KnowledgeConfidenceBand::Weak);
        assert(confidenceToBand(0.34) == KnowledgeConfidenceBand::Weak);
        assert(confidenceToBand(0.35) == KnowledgeConfidenceBand::Plausible);
        assert(confidenceToBand(0.54) == KnowledgeConfidenceBand::Plausible);
        assert(confidenceToBand(0.55) == KnowledgeConfidenceBand::Reliable);
        assert(confidenceToBand(0.74) == KnowledgeConfidenceBand::Reliable);
        assert(confidenceToBand(0.75) == KnowledgeConfidenceBand::Strong);
        assert(confidenceToBand(0.89) == KnowledgeConfidenceBand::Strong);
        assert(confidenceToBand(0.90) == KnowledgeConfidenceBand::Established);
        assert(confidenceToBand(1.0) == KnowledgeConfidenceBand::Established);
    }

    std::cout << "knowledge_types tests passed" << std::endl;
}

int main(int argc, char* argv[]) {
    run_knowledge_types_tests();
    return 0;
}
