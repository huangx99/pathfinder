#include "pathfinder/content/content_registry.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::content {

std::shared_ptr<ContentRegistry> ContentRegistry::build(const ContentDraftRegistry& draft) {
    auto registry = std::make_shared<ContentRegistry>();

    for (const auto& obj : draft.objects()) {
        registry->objects_[obj.key.value()] = obj;
    }
    for (const auto& action : draft.actions()) {
        registry->actions_[action.key.value()] = action;
    }
    for (const auto& effect : draft.effects()) {
        registry->effects_[effect.key.value()] = effect;
    }
    for (const auto& fb : draft.feedbacks()) {
        registry->feedbacks_[fb.key.value()] = fb;
    }
    for (const auto& reaction : draft.reactions()) {
        registry->reactions_[reaction.key.value()] = reaction;
    }
    for (const auto& agent : draft.agents()) {
        registry->agents_[agent.key.value()] = agent;
    }
    for (const auto& threat : draft.threats()) {
        registry->threats_[threat.key.value()] = threat;
    }
    for (const auto& kt : draft.knowledge_templates()) {
        registry->knowledge_templates_[kt.key.value()] = kt;
    }
    for (const auto& scenario : draft.scenarios()) {
        registry->scenarios_[scenario.key.value()] = scenario;
    }
    for (const auto& profile : draft.worldgen_profiles()) {
        registry->worldgen_profiles_[profile.profile_key] = profile;
    }
    for (const auto& [locale_key, locale_map] : draft.locales()) {
        registry->locales_[locale_key] = locale_map;
    }

    return registry;
}

// Object queries
const ObjectDefinitionContent* ContentRegistry::findObject(const std::string& key) const {
    auto it = objects_.find(key);
    return (it != objects_.end()) ? &it->second : nullptr;
}

std::vector<const ObjectDefinitionContent*> ContentRegistry::allObjects() const {
    std::vector<const ObjectDefinitionContent*> result;
    for (const auto& [k, v] : objects_) result.push_back(&v);
    return result;
}

// Action queries
const ActionDefinitionContent* ContentRegistry::findAction(const std::string& key) const {
    auto it = actions_.find(key);
    return (it != actions_.end()) ? &it->second : nullptr;
}

std::vector<const ActionDefinitionContent*> ContentRegistry::allActions() const {
    std::vector<const ActionDefinitionContent*> result;
    for (const auto& [k, v] : actions_) result.push_back(&v);
    return result;
}

// Effect queries
const EffectDefinitionContent* ContentRegistry::findEffect(const std::string& key) const {
    auto it = effects_.find(key);
    return (it != effects_.end()) ? &it->second : nullptr;
}

std::vector<const EffectDefinitionContent*> ContentRegistry::allEffects() const {
    std::vector<const EffectDefinitionContent*> result;
    for (const auto& [k, v] : effects_) result.push_back(&v);
    return result;
}

// Feedback queries
const FeedbackDefinitionContent* ContentRegistry::findFeedback(const std::string& key) const {
    auto it = feedbacks_.find(key);
    return (it != feedbacks_.end()) ? &it->second : nullptr;
}

std::vector<const FeedbackDefinitionContent*> ContentRegistry::feedbacksFor(
    const std::string& object_key,
    const std::string& action_key,
    const std::string& target_key) const {
    std::vector<const FeedbackDefinitionContent*> result;
    for (const auto& [k, fb] : feedbacks_) {
        if (fb.object_key == object_key && fb.action_key == action_key) {
            if (target_key.empty() || fb.target_object_key == target_key || fb.target_object_key.empty()) {
                result.push_back(&fb);
            }
        }
    }
    // Sort by priority descending
    std::sort(result.begin(), result.end(),
        [](const FeedbackDefinitionContent* a, const FeedbackDefinitionContent* b) {
            return a->priority > b->priority;
        });
    return result;
}

std::vector<const FeedbackDefinitionContent*> ContentRegistry::allFeedbacks() const {
    std::vector<const FeedbackDefinitionContent*> result;
    for (const auto& [k, v] : feedbacks_) result.push_back(&v);
    return result;
}

// Reaction queries
const ReactionDefinitionContent* ContentRegistry::findReaction(const std::string& key) const {
    auto it = reactions_.find(key);
    return (it != reactions_.end()) ? &it->second : nullptr;
}

std::vector<const ReactionDefinitionContent*> ContentRegistry::reactionsProducing(const std::string& object_key) const {
    std::vector<const ReactionDefinitionContent*> result;
    for (const auto& [k, r] : reactions_) {
        for (const auto& output : r.outputs) {
            if (output.object_key == object_key) {
                result.push_back(&r);
                break;
            }
        }
    }
    return result;
}

std::vector<const ReactionDefinitionContent*> ContentRegistry::allReactions() const {
    std::vector<const ReactionDefinitionContent*> result;
    for (const auto& [k, v] : reactions_) result.push_back(&v);
    return result;
}

// Agent queries
const AgentTemplateContent* ContentRegistry::findAgent(const std::string& key) const {
    auto it = agents_.find(key);
    return (it != agents_.end()) ? &it->second : nullptr;
}

std::vector<const AgentTemplateContent*> ContentRegistry::allAgents() const {
    std::vector<const AgentTemplateContent*> result;
    for (const auto& [k, v] : agents_) result.push_back(&v);
    return result;
}

// Threat queries
const ThreatDefinitionContent* ContentRegistry::findThreat(const std::string& key) const {
    auto it = threats_.find(key);
    return (it != threats_.end()) ? &it->second : nullptr;
}

std::vector<const ThreatDefinitionContent*> ContentRegistry::allThreats() const {
    std::vector<const ThreatDefinitionContent*> result;
    for (const auto& [k, v] : threats_) result.push_back(&v);
    return result;
}

// Knowledge template queries
const KnowledgeTemplateContent* ContentRegistry::findKnowledgeTemplate(const std::string& key) const {
    auto it = knowledge_templates_.find(key);
    return (it != knowledge_templates_.end()) ? &it->second : nullptr;
}

std::vector<const KnowledgeTemplateContent*> ContentRegistry::allKnowledgeTemplates() const {
    std::vector<const KnowledgeTemplateContent*> result;
    for (const auto& [k, v] : knowledge_templates_) result.push_back(&v);
    return result;
}

// Scenario queries
const ScenarioDefinitionContent* ContentRegistry::findScenario(const std::string& key) const {
    auto it = scenarios_.find(key);
    return (it != scenarios_.end()) ? &it->second : nullptr;
}

std::vector<const ScenarioDefinitionContent*> ContentRegistry::allScenarios() const {
    std::vector<const ScenarioDefinitionContent*> result;
    for (const auto& [k, v] : scenarios_) result.push_back(&v);
    return result;
}

const WorldgenProfileDefinitionContent* ContentRegistry::findWorldgenProfile(const std::string& key) const {
    auto it = worldgen_profiles_.find(key);
    return (it != worldgen_profiles_.end()) ? &it->second : nullptr;
}

std::vector<const WorldgenProfileDefinitionContent*> ContentRegistry::allWorldgenProfiles() const {
    std::vector<const WorldgenProfileDefinitionContent*> result;
    for (const auto& [k, v] : worldgen_profiles_) result.push_back(&v);
    return result;
}

// Locale queries
const LocaleMap* ContentRegistry::findLocale(const std::string& locale_key) const {
    auto it = locales_.find(locale_key);
    return (it != locales_.end()) ? &it->second : nullptr;
}

std::string ContentRegistry::translate(const std::string& locale_key, const std::string& key) const {
    auto locale_it = locales_.find(locale_key);
    if (locale_it == locales_.end()) return key;
    auto text_it = locale_it->second.find(key);
    return (text_it != locale_it->second.end()) ? text_it->second : key;
}

// Duplicate key detection
pathfinder::foundation::Result<void> ContentRegistry::checkNoDuplicates() const {
    // The unordered_map nature prevents duplicates by construction.
    // This method is a placeholder for cross-category duplicate detection if needed.
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::content
