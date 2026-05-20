#include "pathfinder/content/content_types.h"
#include "pathfinder/foundation/id.h"
#include <stdexcept>

namespace pathfinder::content {

// ============================================================
// ContentFileCategory
// ============================================================

std::string toString(ContentFileCategory category) {
    switch (category) {
        case ContentFileCategory::Objects: return "objects";
        case ContentFileCategory::Actions: return "actions";
        case ContentFileCategory::Effects: return "effects";
        case ContentFileCategory::Feedbacks: return "feedbacks";
        case ContentFileCategory::Reactions: return "reactions";
        case ContentFileCategory::Agents: return "agents";
        case ContentFileCategory::Threats: return "threats";
        case ContentFileCategory::Knowledge: return "knowledge";
        case ContentFileCategory::Scenarios: return "scenarios";
        case ContentFileCategory::Locales: return "locales";
        case ContentFileCategory::Tests: return "tests";
    }
    return "unknown";
}

pathfinder::foundation::Result<ContentFileCategory> contentFileCategoryFromString(const std::string& value) {
    if (value == "objects") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Objects);
    if (value == "actions") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Actions);
    if (value == "effects") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Effects);
    if (value == "feedbacks") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Feedbacks);
    if (value == "reactions") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Reactions);
    if (value == "agents") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Agents);
    if (value == "threats") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Threats);
    if (value == "knowledge") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Knowledge);
    if (value == "scenarios") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Scenarios);
    if (value == "locales") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Locales);
    if (value == "tests") return pathfinder::foundation::Result<ContentFileCategory>::ok(ContentFileCategory::Tests);
    return pathfinder::foundation::Result<ContentFileCategory>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown ContentFileCategory: " + value
        ));
}

// ============================================================
// ContentDefinitionKind
// ============================================================

std::string toString(ContentDefinitionKind kind) {
    switch (kind) {
        case ContentDefinitionKind::Object: return "object";
        case ContentDefinitionKind::Action: return "action";
        case ContentDefinitionKind::Effect: return "effect";
        case ContentDefinitionKind::Feedback: return "feedback";
        case ContentDefinitionKind::Reaction: return "reaction";
        case ContentDefinitionKind::Agent: return "agent";
        case ContentDefinitionKind::Threat: return "threat";
        case ContentDefinitionKind::KnowledgeTemplate: return "knowledge_template";
        case ContentDefinitionKind::Scenario: return "scenario";
        case ContentDefinitionKind::Locale: return "locale";
    }
    return "unknown";
}

pathfinder::foundation::Result<ContentDefinitionKind> contentDefinitionKindFromString(const std::string& value) {
    if (value == "object") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Object);
    if (value == "action") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Action);
    if (value == "effect") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Effect);
    if (value == "feedback") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Feedback);
    if (value == "reaction") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Reaction);
    if (value == "agent") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Agent);
    if (value == "threat") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Threat);
    if (value == "knowledge_template") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::KnowledgeTemplate);
    if (value == "scenario") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Scenario);
    if (value == "locale") return pathfinder::foundation::Result<ContentDefinitionKind>::ok(ContentDefinitionKind::Locale);
    return pathfinder::foundation::Result<ContentDefinitionKind>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown ContentDefinitionKind: " + value
        ));
}

// ============================================================
// ContentLoadStage
// ============================================================

std::string toString(ContentLoadStage stage) {
    switch (stage) {
        case ContentLoadStage::ReadManifest: return "read_manifest";
        case ContentLoadStage::ReadFiles: return "read_files";
        case ContentLoadStage::ParseJson: return "parse_json";
        case ContentLoadStage::ValidateStructure: return "validate_structure";
        case ContentLoadStage::ValidateReferences: return "validate_references";
        case ContentLoadStage::BuildRegistry: return "build_registry";
        case ContentLoadStage::BindRuntimeAdapters: return "bind_runtime_adapters";
    }
    return "unknown";
}

// ============================================================
// ContentOverridePolicy
// ============================================================

std::string toString(ContentOverridePolicy policy) {
    switch (policy) {
        case ContentOverridePolicy::Forbidden: return "forbidden";
        case ContentOverridePolicy::AllowSamePackagePatch: return "allow_same_package_patch";
        case ContentOverridePolicy::AllowHigherPriorityPackage: return "allow_higher_priority_package";
    }
    return "unknown";
}

pathfinder::foundation::Result<ContentOverridePolicy> contentOverridePolicyFromString(const std::string& value) {
    if (value == "forbidden") return pathfinder::foundation::Result<ContentOverridePolicy>::ok(ContentOverridePolicy::Forbidden);
    if (value == "allow_same_package_patch") return pathfinder::foundation::Result<ContentOverridePolicy>::ok(ContentOverridePolicy::AllowSamePackagePatch);
    if (value == "allow_higher_priority_package") return pathfinder::foundation::Result<ContentOverridePolicy>::ok(ContentOverridePolicy::AllowHigherPriorityPackage);
    return pathfinder::foundation::Result<ContentOverridePolicy>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown ContentOverridePolicy: " + value
        ));
}

// ============================================================
// ContentLoadMode
// ============================================================

std::string toString(ContentLoadMode mode) {
    switch (mode) {
        case ContentLoadMode::StrictContentRequired: return "strict_content_required";
        case ContentLoadMode::AllowBuiltinFallback: return "allow_builtin_fallback";
    }
    return "unknown";
}

pathfinder::foundation::Result<ContentLoadMode> contentLoadModeFromString(const std::string& value) {
    if (value == "strict_content_required") return pathfinder::foundation::Result<ContentLoadMode>::ok(ContentLoadMode::StrictContentRequired);
    if (value == "allow_builtin_fallback") return pathfinder::foundation::Result<ContentLoadMode>::ok(ContentLoadMode::AllowBuiltinFallback);
    return pathfinder::foundation::Result<ContentLoadMode>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown ContentLoadMode: " + value
        ));
}

// ============================================================
// ContentVisibility
// ============================================================

std::string toString(ContentVisibility visibility) {
    switch (visibility) {
        case ContentVisibility::RuntimeVisible: return "runtime_visible";
        case ContentVisibility::HiddenUntilDiscovered: return "hidden_until_discovered";
        case ContentVisibility::SystemOnly: return "system_only";
        case ContentVisibility::TestOnly: return "test_only";
    }
    return "unknown";
}

pathfinder::foundation::Result<ContentVisibility> contentVisibilityFromString(const std::string& value) {
    if (value == "runtime_visible") return pathfinder::foundation::Result<ContentVisibility>::ok(ContentVisibility::RuntimeVisible);
    if (value == "hidden_until_discovered") return pathfinder::foundation::Result<ContentVisibility>::ok(ContentVisibility::HiddenUntilDiscovered);
    if (value == "system_only") return pathfinder::foundation::Result<ContentVisibility>::ok(ContentVisibility::SystemOnly);
    if (value == "test_only") return pathfinder::foundation::Result<ContentVisibility>::ok(ContentVisibility::TestOnly);
    return pathfinder::foundation::Result<ContentVisibility>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown ContentVisibility: " + value
        ));
}

// ============================================================
// Content definition validateBasic implementations
// ============================================================

pathfinder::foundation::Result<void> ObjectDefinitionContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "object key is empty"));
    }
    if (display_key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "object display_key is empty"));
    }
    if (kind.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "object kind is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> ActionDefinitionContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "action key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> EffectDefinitionContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "effect key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> FeedbackDefinitionContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "feedback key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> ReactionDefinitionContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "reaction key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> AgentTemplateContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "agent key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> ThreatDefinitionContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "threat key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> KnowledgeTemplateContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "knowledge key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> ScenarioDefinitionContent::validateBasic() const {
    if (key.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed, "scenario key is empty"));
    }
    return pathfinder::foundation::Result<void>::ok();
}

// ============================================================
// ContentDraftRegistry
// ============================================================

void ContentDraftRegistry::addObject(ObjectDefinitionContent obj) { objects_.push_back(std::move(obj)); }
void ContentDraftRegistry::addAction(ActionDefinitionContent action) { actions_.push_back(std::move(action)); }
void ContentDraftRegistry::addEffect(EffectDefinitionContent effect) { effects_.push_back(std::move(effect)); }
void ContentDraftRegistry::addFeedback(FeedbackDefinitionContent feedback) { feedbacks_.push_back(std::move(feedback)); }
void ContentDraftRegistry::addReaction(ReactionDefinitionContent reaction) { reactions_.push_back(std::move(reaction)); }
void ContentDraftRegistry::addAgent(AgentTemplateContent agent) { agents_.push_back(std::move(agent)); }
void ContentDraftRegistry::addThreat(ThreatDefinitionContent threat) { threats_.push_back(std::move(threat)); }
void ContentDraftRegistry::addKnowledgeTemplate(KnowledgeTemplateContent knowledge) { knowledge_templates_.push_back(std::move(knowledge)); }
void ContentDraftRegistry::addScenario(ScenarioDefinitionContent scenario) { scenarios_.push_back(std::move(scenario)); }
void ContentDraftRegistry::addLocale(const std::string& locale_key, LocaleMap locales) { locales_[locale_key] = std::move(locales); }

void ContentDraftRegistry::clear() {
    objects_.clear();
    actions_.clear();
    effects_.clear();
    feedbacks_.clear();
    reactions_.clear();
    agents_.clear();
    threats_.clear();
    knowledge_templates_.clear();
    scenarios_.clear();
    locales_.clear();
}

} // namespace pathfinder::content
