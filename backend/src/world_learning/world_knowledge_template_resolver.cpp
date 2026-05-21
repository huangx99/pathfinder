#include "pathfinder/world_learning/world_knowledge_template_resolver.h"
#include <algorithm>

namespace pathfinder::world_learning {

WorldKnowledgeTemplateResolver::WorldKnowledgeTemplateResolver(
    const pathfinder::content::ContentRegistry& content_registry)
    : content_registry_(content_registry) {}

WorldKnowledgeTemplateResolver::ResolveResult WorldKnowledgeTemplateResolver::resolve(
    const world_command::WorldExperienceDto& experience) const {
    ResolveResult result;

    // Step 1: Try reason_keys first (highest priority)
    auto from_reason = resolveFromReasonKeys(experience.reason_keys);
    if (!from_reason.empty()) {
        result.templates = std::move(from_reason);
        for (const auto& t : result.templates) {
            result.matched_keys.push_back(t.key.value());
        }
        // Sort for stability
        std::sort(result.matched_keys.begin(), result.matched_keys.end());
        return result;
    }

    // Step 2: Fallback to action/effect matching
    auto from_fallback = resolveFromActionEffectFallback(experience);
    if (!from_fallback.empty()) {
        result.templates = std::move(from_fallback);
        for (const auto& t : result.templates) {
            result.matched_keys.push_back(t.key.value());
        }
        std::sort(result.matched_keys.begin(), result.matched_keys.end());
        return result;
    }

    result.failure_kind = WorldLearningFailureKind::TemplateMissing;
    result.warning_keys.push_back("template_missing_for_experience:" + experience.experience_id);
    return result;
}

std::vector<pathfinder::content::KnowledgeTemplateContent>
WorldKnowledgeTemplateResolver::resolveFromReasonKeys(
    const std::vector<std::string>& reason_keys) const {
    std::vector<pathfinder::content::KnowledgeTemplateContent> result;
    for (const auto& key : reason_keys) {
        const auto* tmpl = content_registry_.findKnowledgeTemplate(key);
        if (tmpl != nullptr) {
            result.push_back(*tmpl);
        }
    }
    return result;
}

std::vector<pathfinder::content::KnowledgeTemplateContent>
WorldKnowledgeTemplateResolver::resolveFromActionEffectFallback(
    const world_command::WorldExperienceDto& experience) const {
    std::vector<pathfinder::content::KnowledgeTemplateContent> result;
    auto all_templates = content_registry_.allKnowledgeTemplates();
    for (const auto* tmpl : all_templates) {
        if (tmpl != nullptr && matchesExperience(*tmpl, experience)) {
            result.push_back(*tmpl);
        }
    }
    // Sort by key for stability
    std::sort(result.begin(), result.end(),
        [](const auto& a, const auto& b) { return a.key.value() < b.key.value(); });
    return result;
}

bool WorldKnowledgeTemplateResolver::matchesExperience(
    const pathfinder::content::KnowledgeTemplateContent& tmpl,
    const world_command::WorldExperienceDto& experience) const {
    // Match action_key
    if (!tmpl.action_key.empty() && tmpl.action_key != experience.command_key) {
        return false;
    }
    // Match effect_key
    if (!tmpl.effect_key.empty() && tmpl.effect_key != experience.effect_key) {
        return false;
    }
    // Match subject_object_key against subject_entity_key or target_entity_key
    if (!tmpl.subject_object_key.empty()) {
        if (tmpl.subject_object_key != experience.subject_entity_key &&
            tmpl.subject_object_key != experience.target_entity_key) {
            return false;
        }
    }
    // target_object_key is optional; if set, must match experience.target_entity_key
    if (!tmpl.target_object_key.empty() && tmpl.target_object_key != experience.target_entity_key) {
        return false;
    }
    return true;
}

} // namespace pathfinder::world_learning
