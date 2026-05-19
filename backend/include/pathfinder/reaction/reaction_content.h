#pragma once

#include "pathfinder/reaction/reaction_rule.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::reaction {

enum class ReactionKnowledgeSubjectPolicy {
    Unknown,
    SourceDefinition,
    TargetDefinition,
    MaterialDefinition,
    ExplicitSubject,
    TestOnly
};

std::string toString(ReactionKnowledgeSubjectPolicy policy);
pathfinder::foundation::Result<ReactionKnowledgeSubjectPolicy> reactionKnowledgeSubjectPolicyFromString(const std::string& value);

struct ItemDefinitionContent {
    pathfinder::foundation::ObjectDefinitionId definition_id;
    std::string object_key;
    std::string display_name;
    pathfinder::object::ObjectCategory category{pathfinder::object::ObjectCategory::Unknown};
    std::vector<std::string> tag_keys;
    std::vector<std::string> default_state_keys;
    std::map<std::string, double> default_resource_values;
    std::string safe_description;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionRuntimeObject {
    pathfinder::foundation::ObjectId object_id;
    pathfinder::foundation::ObjectDefinitionId definition_id;
    std::string object_key;
    pathfinder::object::ObjectCategory category{pathfinder::object::ObjectCategory::Unknown};
    int quantity{1};
    std::vector<std::string> state_keys;
    std::vector<std::string> tag_keys;
    std::map<std::string, double> resource_values;
    std::string location_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class ItemCatalog {
public:
    pathfinder::foundation::Result<void> addItem(ItemDefinitionContent item);
    const ItemDefinitionContent* find(const pathfinder::foundation::ObjectDefinitionId& definition_id) const;
    const std::vector<ItemDefinitionContent>& items() const { return items_; }
    pathfinder::foundation::Result<ReactionRuntimeObject> instantiate(
        pathfinder::foundation::ObjectId object_id,
        const pathfinder::foundation::ObjectDefinitionId& definition_id,
        std::string location_key,
        int quantity = 1) const;
    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<ItemDefinitionContent> items_;
};

struct ReactionKnowledgeTemplate {
    std::string rule_key;
    ReactionKnowledgeSubjectPolicy subject_policy{ReactionKnowledgeSubjectPolicy::TargetDefinition};
    std::string subject_id;
    std::string action_key;
    std::string effect_key;
    bool shareable{true};
    bool teachable{true};
    bool npc_learnable{true};
    bool observation_learnable{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReactionContentPack {
    ItemCatalog catalog;
    std::vector<ObjectReactionRule> rules;
    std::vector<ReactionKnowledgeTemplate> knowledge_templates;

    const ReactionKnowledgeTemplate* findKnowledgeTemplate(const std::string& rule_key) const;
};

class ReactionRuntimeWorld {
public:
    pathfinder::foundation::Result<void> addObject(ReactionRuntimeObject object);
    ReactionRuntimeObject* findObject(const pathfinder::foundation::ObjectId& object_id);
    const ReactionRuntimeObject* findObject(const pathfinder::foundation::ObjectId& object_id) const;
    const std::vector<ReactionRuntimeObject>& objects() const { return objects_; }
    std::vector<ReactionRuntimeObject>& objects() { return objects_; }
    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<ReactionRuntimeObject> objects_;
};

class ReactionInputBuilder {
public:
    pathfinder::foundation::Result<ReactionObjectRef> buildObjectRef(
        const ReactionRuntimeObject& object,
        ReactionObjectRole role) const;

    pathfinder::foundation::Result<ReactionInputSet> buildInput(
        const ReactionRuntimeWorld& world,
        ReactionActionKind action_kind,
        pathfinder::foundation::EntityId actor_id,
        std::vector<std::pair<pathfinder::foundation::ObjectId, ReactionObjectRole>> object_roles,
        pathfinder::foundation::Tick tick,
        std::string input_key,
        std::vector<std::string> safe_context_keys = {}) const;
};

class ReactionContentValidator {
public:
    pathfinder::foundation::Result<void> validate(const ReactionContentPack& pack) const;
};

} // namespace pathfinder::reaction
