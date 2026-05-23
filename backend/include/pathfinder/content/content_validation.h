#pragma once

#include "pathfinder/content/content_types.h"
#include "pathfinder/config/validation.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>

namespace pathfinder::content {

// ============================================================
// ContentStructuralValidator - validates DTO field structure
// ============================================================

class ContentStructuralValidator {
public:
    pathfinder::config::ConfigValidationReport validate(const ContentDraftRegistry& draft) const;

private:
    void validateObject(const ObjectDefinitionContent& obj, pathfinder::config::ConfigValidationReport& report) const;
    void validateEffect(const EffectDefinitionContent& effect, pathfinder::config::ConfigValidationReport& report) const;
    void validateFeedback(const FeedbackDefinitionContent& fb, pathfinder::config::ConfigValidationReport& report) const;
    void validateReaction(const ReactionDefinitionContent& reaction, pathfinder::config::ConfigValidationReport& report) const;
    void validateAgent(const AgentTemplateContent& agent, pathfinder::config::ConfigValidationReport& report) const;
    void validateThreat(const ThreatDefinitionContent& threat, pathfinder::config::ConfigValidationReport& report) const;
    void validateKnowledgeTemplate(const KnowledgeTemplateContent& kt, pathfinder::config::ConfigValidationReport& report) const;
    void validateScenario(const ScenarioDefinitionContent& scenario, pathfinder::config::ConfigValidationReport& report) const;
    void validateWorldgenProfile(const WorldgenProfileDefinitionContent& profile, pathfinder::config::ConfigValidationReport& report) const;

    static bool isValidKey(const std::string& key);
    static bool isValidOperationOp(const std::string& op);
};

// ============================================================
// ContentReferenceValidator - validates cross-file references
// ============================================================

class ContentReferenceValidator {
public:
    pathfinder::config::ConfigValidationReport validate(const ContentDraftRegistry& draft) const;

private:
    void validateObjectReferences(const ContentDraftRegistry& draft, pathfinder::config::ConfigValidationReport& report) const;
    void validateFeedbackReferences(const ContentDraftRegistry& draft, pathfinder::config::ConfigValidationReport& report) const;
    void validateReactionReferences(const ContentDraftRegistry& draft, pathfinder::config::ConfigValidationReport& report) const;
    void validateThreatReferences(const ContentDraftRegistry& draft, pathfinder::config::ConfigValidationReport& report) const;
    void validateScenarioReferences(const ContentDraftRegistry& draft, pathfinder::config::ConfigValidationReport& report) const;
    void validateKnowledgeReferences(const ContentDraftRegistry& draft, pathfinder::config::ConfigValidationReport& report) const;
    void validateWorldgenReferences(const ContentDraftRegistry& draft, pathfinder::config::ConfigValidationReport& report) const;
};

} // namespace pathfinder::content
