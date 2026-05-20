#pragma once

#include "pathfinder/content/content_types.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace pathfinder::content {

// ============================================================
// ContentRegistry - read-only registry of all content definitions
// ============================================================

class ContentRegistry {
public:
    ContentRegistry() = default;

    // Build from draft registry (takes ownership)
    static std::shared_ptr<ContentRegistry> build(const ContentDraftRegistry& draft);

    // Object queries
    const ObjectDefinitionContent* findObject(const std::string& key) const;
    std::vector<const ObjectDefinitionContent*> allObjects() const;

    // Action queries
    const ActionDefinitionContent* findAction(const std::string& key) const;
    std::vector<const ActionDefinitionContent*> allActions() const;

    // Effect queries
    const EffectDefinitionContent* findEffect(const std::string& key) const;
    std::vector<const EffectDefinitionContent*> allEffects() const;

    // Feedback queries
    const FeedbackDefinitionContent* findFeedback(const std::string& key) const;
    std::vector<const FeedbackDefinitionContent*> feedbacksFor(
        const std::string& object_key,
        const std::string& action_key,
        const std::string& target_key = "") const;
    std::vector<const FeedbackDefinitionContent*> allFeedbacks() const;

    // Reaction queries
    const ReactionDefinitionContent* findReaction(const std::string& key) const;
    std::vector<const ReactionDefinitionContent*> reactionsProducing(const std::string& object_key) const;
    std::vector<const ReactionDefinitionContent*> allReactions() const;

    // Agent queries
    const AgentTemplateContent* findAgent(const std::string& key) const;
    std::vector<const AgentTemplateContent*> allAgents() const;

    // Threat queries
    const ThreatDefinitionContent* findThreat(const std::string& key) const;
    std::vector<const ThreatDefinitionContent*> allThreats() const;

    // Knowledge template queries
    const KnowledgeTemplateContent* findKnowledgeTemplate(const std::string& key) const;
    std::vector<const KnowledgeTemplateContent*> allKnowledgeTemplates() const;

    // Scenario queries
    const ScenarioDefinitionContent* findScenario(const std::string& key) const;
    std::vector<const ScenarioDefinitionContent*> allScenarios() const;

    // Locale queries
    const LocaleMap* findLocale(const std::string& locale_key) const;
    std::string translate(const std::string& locale_key, const std::string& key) const;

    // Content package info
    const std::string& packageKey() const { return package_key_; }
    const std::string& contentVersion() const { return content_version_; }
    const std::string& schemaVersion() const { return schema_version_; }

    void setPackageKey(const std::string& key) { package_key_ = key; }
    void setContentVersion(const std::string& version) { content_version_ = version; }
    void setSchemaVersion(const std::string& version) { schema_version_ = version; }

    // Check for duplicate keys
    pathfinder::foundation::Result<void> checkNoDuplicates() const;

private:
    std::string package_key_;
    std::string content_version_;
    std::string schema_version_;

    std::unordered_map<std::string, ObjectDefinitionContent> objects_;
    std::unordered_map<std::string, ActionDefinitionContent> actions_;
    std::unordered_map<std::string, EffectDefinitionContent> effects_;
    std::unordered_map<std::string, FeedbackDefinitionContent> feedbacks_;
    std::unordered_map<std::string, ReactionDefinitionContent> reactions_;
    std::unordered_map<std::string, AgentTemplateContent> agents_;
    std::unordered_map<std::string, ThreatDefinitionContent> threats_;
    std::unordered_map<std::string, KnowledgeTemplateContent> knowledge_templates_;
    std::unordered_map<std::string, ScenarioDefinitionContent> scenarios_;
    std::map<std::string, LocaleMap> locales_;
};

} // namespace pathfinder::content
