#include "pathfinder/world_learning/world_knowledge_projection_bridge.h"
#include <algorithm>
#include <sstream>

namespace pathfinder::world_learning {

namespace {

std::string objectName(const pathfinder::content::ContentRegistry& registry, const std::string& object_key) {
    const auto translated = registry.translate("zh_cn", "object." + object_key + ".name");
    return translated == "object." + object_key + ".name" ? object_key : translated;
}

int inputQuantity(const pathfinder::content::ReactionInputDto& input) {
    return input.min > 0 ? input.min : (input.quantity > 0 ? input.quantity : 1);
}

std::string recipeText(const pathfinder::content::ContentRegistry& registry,
                       const pathfinder::content::ReactionDefinitionContent& reaction) {
    std::ostringstream oss;
    for (size_t i = 0; i < reaction.inputs.size(); ++i) {
        if (i > 0) oss << " + ";
        const auto& input = reaction.inputs[i];
        oss << objectName(registry, input.object_key) << "×" << inputQuantity(input);
    }
    return oss.str();
}

const pathfinder::content::KnowledgeTemplateContent* findTemplateForClaim(
    const pathfinder::content::ContentRegistry& registry,
    const pathfinder::knowledge::KnowledgeClaim& claim) {
    for (const auto& reason_key : claim.reason_keys) {
        if (const auto* tmpl = registry.findKnowledgeTemplate(reason_key)) return tmpl;
    }
    for (const auto* tmpl : registry.allKnowledgeTemplates()) {
        if (!tmpl) continue;
        if (tmpl->subject_object_key == claim.subject.subject_id &&
            tmpl->action_key == claim.predicate.action_key &&
            tmpl->effect_key == claim.predicate.effect_key) {
            return tmpl;
        }
    }
    return nullptr;
}

const pathfinder::content::ReactionDefinitionContent* findReactionForClaim(
    const pathfinder::content::ContentRegistry& registry,
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const pathfinder::content::KnowledgeTemplateContent* tmpl) {
    for (const auto* reaction : registry.allReactions()) {
        if (!reaction) continue;
        if (tmpl) {
            const auto& keys = reaction->knowledge_templates;
            if (std::find(keys.begin(), keys.end(), tmpl->key.value()) != keys.end()) return reaction;
        }
        if (reaction->effect_key == claim.predicate.effect_key &&
            (reaction->action_key == claim.predicate.action_key || claim.predicate.action_key == "craft")) {
            return reaction;
        }
    }
    return nullptr;
}

std::string claimSummaryText(const pathfinder::content::ContentRegistry* registry,
                             const pathfinder::knowledge::KnowledgeClaim& claim) {
    if (!registry) return {};
    const auto* tmpl = findTemplateForClaim(*registry, claim);
    if (tmpl && !tmpl->summary_key.empty()) {
        const auto translated = registry->translate("zh_cn", tmpl->summary_key);
        if (translated != tmpl->summary_key) return translated;
        return tmpl->summary_key;
    }
    const auto subject = objectName(*registry, claim.subject.subject_id);
    const auto action = registry->translate("zh_cn", "action." + claim.predicate.action_key + ".name");
    const auto effect = registry->translate("zh_cn", "effect." + claim.predicate.effect_key + ".name");
    return subject + "：" + (action.rfind("action.", 0) == 0 ? claim.predicate.action_key : action) +
        " → " + (effect.rfind("effect.", 0) == 0 ? claim.predicate.effect_key : effect);
}

std::string claimRecipeText(const pathfinder::content::ContentRegistry* registry,
                            const pathfinder::knowledge::KnowledgeClaim& claim) {
    if (!registry) return {};
    const auto* tmpl = findTemplateForClaim(*registry, claim);
    const auto* reaction = findReactionForClaim(*registry, claim, tmpl);
    return reaction ? recipeText(*registry, *reaction) : std::string{};
}

} // namespace

WorldKnowledgeProjectionBridge::WorldKnowledgeProjectionBridge(
    const pathfinder::content::ContentRegistry* content_registry)
    : content_registry_(content_registry) {}

WorldKnowledgeProjectionBridge::ProjectionResult
WorldKnowledgeProjectionBridge::project(
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& learned_claims,
    const std::vector<WorldKnowledgeDelta>& knowledge_deltas,
    const std::string& actor_key,
    uint64_t tick) const {
    ProjectionResult result;

    for (const auto& claim : learned_claims) {
        auto patch = claimToPatch(claim, actor_key);
        result.patch.changed_knowledge.push_back(patch);

        auto evt = claimToEvent(claim, actor_key, tick);
        result.events.push_back(evt);
    }

    for (const auto& delta : knowledge_deltas) {
        auto sd = claimToDelta(delta);
        result.state_deltas.push_back(sd);
    }

    result.patch.new_projection_version = tick;
    return result;
}

world_command::KnowledgePatchDto WorldKnowledgeProjectionBridge::claimToPatch(
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const std::string& actor_key) const {
    world_command::KnowledgePatchDto patch;
    patch.actor_key = actor_key;
    patch.op = world_command::PatchOp::Update;
    patch.fields["knowledge_id"] = claim.knowledge_id.value();
    patch.fields["subject_id"] = claim.subject.subject_id;
    patch.fields["action_key"] = claim.predicate.action_key;
    patch.fields["effect_key"] = claim.predicate.effect_key;
    patch.fields["status"] = pathfinder::knowledge::toString(claim.status);
    patch.fields["teachable"] = claim.teaching_profile.teachable ? "true" : "false";
    patch.fields["usable_by_ai"] = claim.projection_flags.usable_by_ai ? "true" : "false";
    patch.fields["usable_for_action"] = claim.projection_flags.usable_for_action ? "true" : "false";
    patch.fields["confidence"] = std::to_string(claim.confidence.confidence);
    const auto summary = claimSummaryText(content_registry_, claim);
    if (!summary.empty()) patch.fields["summary_text"] = summary;
    const auto recipe = claimRecipeText(content_registry_, claim);
    if (!recipe.empty()) patch.fields["recipe_text"] = recipe;
    return patch;
}

world_command::WorldEventDto WorldKnowledgeProjectionBridge::claimToEvent(
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const std::string& actor_key,
    uint64_t tick) const {
    world_command::WorldEventDto evt;
    evt.event_id = claim.knowledge_id.value() + ":event";
    evt.event_kind = "knowledge_formed";
    evt.tick = tick;
    evt.actor_key = actor_key;
    evt.reason_keys = claim.reason_keys;
    return evt;
}

world_command::WorldStateDeltaDto WorldKnowledgeProjectionBridge::claimToDelta(
    const WorldKnowledgeDelta& delta) const {
    world_command::WorldStateDeltaDto sd;
    sd.delta_id = delta.delta_id;
    sd.delta_kind = "knowledge_delta";
    sd.target_ref = delta.owner_key;
    sd.op = world_command::PatchOp::Update;
    sd.fields["knowledge_id"] = delta.knowledge_id.value();
    sd.fields["subject"] = delta.subject_object_key;
    sd.fields["action"] = delta.action_key;
    sd.fields["effect"] = delta.effect_key;
    sd.fields["status"] = delta.status;
    sd.fields["decision"] = delta.decision;
    sd.reason_keys = delta.reason_keys;
    return sd;
}

} // namespace pathfinder::world_learning
