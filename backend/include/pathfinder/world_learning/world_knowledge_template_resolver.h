#pragma once

#include "pathfinder/world_learning/world_learning_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_learning {

// ---------------------------------------------------------------------------
// WorldKnowledgeTemplateResolver
// ---------------------------------------------------------------------------
// Resolves knowledge templates from ContentRegistry based on experience
// reason_keys, action_key, effect_key, and subject_entity_key.
// Never hardcodes content keys.
// ---------------------------------------------------------------------------

class WorldKnowledgeTemplateResolver {
public:
    struct ResolveResult {
        std::vector<pathfinder::content::KnowledgeTemplateContent> templates;
        std::vector<std::string> matched_keys;
        WorldLearningFailureKind failure_kind = WorldLearningFailureKind::None;
        std::vector<std::string> warning_keys;
    };

    explicit WorldKnowledgeTemplateResolver(const pathfinder::content::ContentRegistry& content_registry);

    ResolveResult resolve(const world_command::WorldExperienceDto& experience) const;

private:
    const pathfinder::content::ContentRegistry& content_registry_;

    // Resolution steps in priority order
    std::vector<pathfinder::content::KnowledgeTemplateContent> resolveFromReasonKeys(
        const std::vector<std::string>& reason_keys) const;

    std::vector<pathfinder::content::KnowledgeTemplateContent> resolveFromActionEffectFallback(
        const world_command::WorldExperienceDto& experience) const;

    bool matchesExperience(
        const pathfinder::content::KnowledgeTemplateContent& tmpl,
        const world_command::WorldExperienceDto& experience) const;
};

} // namespace pathfinder::world_learning
