#include "pathfinder/reaction/reaction_content.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace pathfinder::reaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static Result<void> validateSafeKey(const std::string& key, const std::string& field, bool required = true) {
    if (key.empty()) {
        if (required) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " empty"));
        return Result<void>::ok();
    }
    if (containsReactionForbiddenKey(key)) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " forbidden"));
    return Result<void>::ok();
}

static Result<void> validateSafeText(const std::string& text, const std::string& field, bool required = true) {
    if (text.empty()) {
        if (required) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " empty"));
        return Result<void>::ok();
    }
    if (containsReactionForbiddenKey(text)) return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " forbidden"));
    return Result<void>::ok();
}

static Result<void> validateSafeKeys(const std::vector<std::string>& keys, const std::string& field, bool reject_dynamic_state) {
    for (const auto& key : keys) {
        auto valid = validateSafeKey(key, field);
        if (valid.is_error()) return valid;
        if (reject_dynamic_state && isDynamicStateLikeKey(key)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, field + " cannot contain dynamic state semantics"));
        }
    }
    return Result<void>::ok();
}

static Result<void> validateResources(const std::map<std::string, double>& values, const std::string& field) {
    for (const auto& [key, value] : values) {
        auto key_valid = validateSafeKey(key, field + " key");
        if (key_valid.is_error()) return key_valid;
        if (!std::isfinite(value)) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, field + " value must be finite"));
    }
    return Result<void>::ok();
}

static bool hasRule(const std::vector<ObjectReactionRule>& rules, const std::string& rule_key) {
    return std::any_of(rules.begin(), rules.end(), [&](const auto& rule) { return rule.rule_key == rule_key; });
}

std::string toString(ReactionKnowledgeSubjectPolicy policy) {
    switch (policy) {
        case ReactionKnowledgeSubjectPolicy::Unknown: return "unknown";
        case ReactionKnowledgeSubjectPolicy::SourceDefinition: return "source_definition";
        case ReactionKnowledgeSubjectPolicy::TargetDefinition: return "target_definition";
        case ReactionKnowledgeSubjectPolicy::MaterialDefinition: return "material_definition";
        case ReactionKnowledgeSubjectPolicy::ExplicitSubject: return "explicit_subject";
        case ReactionKnowledgeSubjectPolicy::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<ReactionKnowledgeSubjectPolicy> reactionKnowledgeSubjectPolicyFromString(const std::string& value) {
    if (value == "unknown") return Result<ReactionKnowledgeSubjectPolicy>::ok(ReactionKnowledgeSubjectPolicy::Unknown);
    if (value == "source_definition") return Result<ReactionKnowledgeSubjectPolicy>::ok(ReactionKnowledgeSubjectPolicy::SourceDefinition);
    if (value == "target_definition") return Result<ReactionKnowledgeSubjectPolicy>::ok(ReactionKnowledgeSubjectPolicy::TargetDefinition);
    if (value == "material_definition") return Result<ReactionKnowledgeSubjectPolicy>::ok(ReactionKnowledgeSubjectPolicy::MaterialDefinition);
    if (value == "explicit_subject") return Result<ReactionKnowledgeSubjectPolicy>::ok(ReactionKnowledgeSubjectPolicy::ExplicitSubject);
    if (value == "test_only") return Result<ReactionKnowledgeSubjectPolicy>::ok(ReactionKnowledgeSubjectPolicy::TestOnly);
    return Result<ReactionKnowledgeSubjectPolicy>::fail(makeError(ErrorCode::validation_enum_unknown, "invalid ReactionKnowledgeSubjectPolicy: " + value));
}

Result<void> ItemDefinitionContent::validateBasic() const {
    if (definition_id.empty() || !isValidIdString(definition_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ItemDefinitionContent definition_id invalid"));
    }
    auto object_key_valid = validateSafeKey(object_key, "ItemDefinitionContent object_key");
    if (object_key_valid.is_error()) return object_key_valid;
    auto display_valid = validateSafeText(display_name, "ItemDefinitionContent display_name");
    if (display_valid.is_error()) return display_valid;
    if (category == pathfinder::object::ObjectCategory::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ItemDefinitionContent category invalid"));
    }
    auto tags_valid = validateSafeKeys(tag_keys, "ItemDefinitionContent tag_keys", true);
    if (tags_valid.is_error()) return tags_valid;
    auto states_valid = validateSafeKeys(default_state_keys, "ItemDefinitionContent default_state_keys", false);
    if (states_valid.is_error()) return states_valid;
    auto resources_valid = validateResources(default_resource_values, "ItemDefinitionContent default_resource_values");
    if (resources_valid.is_error()) return resources_valid;
    return validateSafeText(safe_description, "ItemDefinitionContent safe_description", false);
}

Result<void> ReactionRuntimeObject::validateBasic() const {
    if (object_id.empty() || !isValidIdString(object_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionRuntimeObject object_id invalid"));
    }
    if (definition_id.empty() || !isValidIdString(definition_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ReactionRuntimeObject definition_id invalid"));
    }
    auto object_key_valid = validateSafeKey(object_key, "ReactionRuntimeObject object_key");
    if (object_key_valid.is_error()) return object_key_valid;
    if (category == pathfinder::object::ObjectCategory::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionRuntimeObject category invalid"));
    }
    if (quantity < 0) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ReactionRuntimeObject quantity must be >= 0"));
    auto states_valid = validateSafeKeys(state_keys, "ReactionRuntimeObject state_keys", false);
    if (states_valid.is_error()) return states_valid;
    auto tags_valid = validateSafeKeys(tag_keys, "ReactionRuntimeObject tag_keys", true);
    if (tags_valid.is_error()) return tags_valid;
    auto resources_valid = validateResources(resource_values, "ReactionRuntimeObject resource_values");
    if (resources_valid.is_error()) return resources_valid;
    return validateSafeKey(location_key, "ReactionRuntimeObject location_key", false);
}

Result<void> ItemCatalog::addItem(ItemDefinitionContent item) {
    auto valid = item.validateBasic();
    if (valid.is_error()) return valid;
    if (find(item.definition_id)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ItemCatalog duplicate definition_id"));
    }
    items_.push_back(std::move(item));
    return Result<void>::ok();
}

const ItemDefinitionContent* ItemCatalog::find(const pathfinder::foundation::ObjectDefinitionId& definition_id) const {
    for (const auto& item : items_) {
        if (item.definition_id == definition_id) return &item;
    }
    return nullptr;
}

Result<ReactionRuntimeObject> ItemCatalog::instantiate(
    pathfinder::foundation::ObjectId object_id,
    const pathfinder::foundation::ObjectDefinitionId& definition_id,
    std::string location_key,
    int quantity) const {

    const auto* item = find(definition_id);
    if (!item) return Result<ReactionRuntimeObject>::fail(makeError(ErrorCode::id_definition_not_found, "ItemCatalog instantiate definition missing"));

    ReactionRuntimeObject object;
    object.object_id = std::move(object_id);
    object.definition_id = item->definition_id;
    object.object_key = item->object_key;
    object.category = item->category;
    object.quantity = quantity;
    object.state_keys = item->default_state_keys;
    object.tag_keys = item->tag_keys;
    object.resource_values = item->default_resource_values;
    object.location_key = std::move(location_key);

    auto valid = object.validateBasic();
    if (valid.is_error()) return Result<ReactionRuntimeObject>::fail(valid.errors());
    return Result<ReactionRuntimeObject>::ok(std::move(object));
}

Result<void> ItemCatalog::validateBasic() const {
    std::set<pathfinder::foundation::ObjectDefinitionId> ids;
    for (const auto& item : items_) {
        auto valid = item.validateBasic();
        if (valid.is_error()) return valid;
        if (!ids.insert(item.definition_id).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ItemCatalog duplicate definition_id"));
        }
    }
    return Result<void>::ok();
}

Result<void> ReactionKnowledgeTemplate::validateBasic() const {
    auto rule_valid = validateSafeKey(rule_key, "ReactionKnowledgeTemplate rule_key");
    if (rule_valid.is_error()) return rule_valid;
    if (subject_policy == ReactionKnowledgeSubjectPolicy::Unknown || subject_policy == ReactionKnowledgeSubjectPolicy::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ReactionKnowledgeTemplate subject_policy invalid"));
    }
    if (subject_policy == ReactionKnowledgeSubjectPolicy::ExplicitSubject) {
        auto subject_valid = validateSafeKey(subject_id, "ReactionKnowledgeTemplate subject_id");
        if (subject_valid.is_error()) return subject_valid;
    } else {
        auto subject_valid = validateSafeKey(subject_id, "ReactionKnowledgeTemplate subject_id", false);
        if (subject_valid.is_error()) return subject_valid;
    }
    auto action_valid = validateSafeKey(action_key, "ReactionKnowledgeTemplate action_key");
    if (action_valid.is_error()) return action_valid;
    return validateSafeKey(effect_key, "ReactionKnowledgeTemplate effect_key");
}

const ReactionKnowledgeTemplate* ReactionContentPack::findKnowledgeTemplate(const std::string& rule_key) const {
    for (const auto& tmpl : knowledge_templates) {
        if (tmpl.rule_key == rule_key) return &tmpl;
    }
    return nullptr;
}

Result<void> ReactionRuntimeWorld::addObject(ReactionRuntimeObject object) {
    auto valid = object.validateBasic();
    if (valid.is_error()) return valid;
    if (findObject(object.object_id)) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionRuntimeWorld duplicate object_id"));
    objects_.push_back(std::move(object));
    return Result<void>::ok();
}

ReactionRuntimeObject* ReactionRuntimeWorld::findObject(const pathfinder::foundation::ObjectId& object_id) {
    for (auto& object : objects_) {
        if (object.object_id == object_id) return &object;
    }
    return nullptr;
}

const ReactionRuntimeObject* ReactionRuntimeWorld::findObject(const pathfinder::foundation::ObjectId& object_id) const {
    for (const auto& object : objects_) {
        if (object.object_id == object_id) return &object;
    }
    return nullptr;
}

Result<void> ReactionRuntimeWorld::validateBasic() const {
    std::set<pathfinder::foundation::ObjectId> ids;
    for (const auto& object : objects_) {
        auto valid = object.validateBasic();
        if (valid.is_error()) return valid;
        if (!ids.insert(object.object_id).second) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionRuntimeWorld duplicate object_id"));
    }
    return Result<void>::ok();
}

Result<ReactionObjectRef> ReactionInputBuilder::buildObjectRef(const ReactionRuntimeObject& object, ReactionObjectRole role) const {
    auto object_valid = object.validateBasic();
    if (object_valid.is_error()) return Result<ReactionObjectRef>::fail(object_valid.errors());
    ReactionObjectRef ref;
    ref.role = role;
    ref.object_id = object.object_id;
    ref.definition_id = object.definition_id;
    ref.object_key = object.object_key;
    ref.category = object.category;
    ref.quantity = object.quantity;
    ref.state_keys = object.state_keys;
    ref.tag_keys = object.tag_keys;
    ref.resource_values = object.resource_values;
    auto valid = ref.validateBasic();
    if (valid.is_error()) return Result<ReactionObjectRef>::fail(valid.errors());
    return Result<ReactionObjectRef>::ok(std::move(ref));
}

Result<ReactionInputSet> ReactionInputBuilder::buildInput(
    const ReactionRuntimeWorld& world,
    ReactionActionKind action_kind,
    pathfinder::foundation::EntityId actor_id,
    std::vector<std::pair<pathfinder::foundation::ObjectId, ReactionObjectRole>> object_roles,
    pathfinder::foundation::Tick tick,
    std::string input_key,
    std::vector<std::string> safe_context_keys) const {

    auto world_valid = world.validateBasic();
    if (world_valid.is_error()) return Result<ReactionInputSet>::fail(world_valid.errors());

    ReactionInputSet input;
    input.input_key = std::move(input_key);
    input.action_kind = action_kind;
    input.actor_id = std::move(actor_id);
    input.tick = tick;
    input.safe_context_keys = std::move(safe_context_keys);

    for (const auto& [object_id, role] : object_roles) {
        const auto* object = world.findObject(object_id);
        if (!object) return Result<ReactionInputSet>::fail(makeError(ErrorCode::id_not_found, "ReactionInputBuilder object not found"));
        auto ref = buildObjectRef(*object, role);
        if (ref.is_error()) return Result<ReactionInputSet>::fail(ref.errors());
        input.objects.push_back(std::move(ref.value()));
    }

    auto valid = input.validateBasic();
    if (valid.is_error()) return Result<ReactionInputSet>::fail(valid.errors());
    return Result<ReactionInputSet>::ok(std::move(input));
}

Result<void> ReactionContentValidator::validate(const ReactionContentPack& pack) const {
    auto catalog_valid = pack.catalog.validateBasic();
    if (catalog_valid.is_error()) return catalog_valid;

    std::set<std::string> rule_keys;
    for (const auto& rule : pack.rules) {
        auto rule_valid = rule.validateBasic();
        if (rule_valid.is_error()) return rule_valid;
        if (!rule_keys.insert(rule.rule_key).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionContentPack duplicate rule_key"));
        }
        for (const auto& pattern : rule.object_patterns) {
            if (pattern.definition_id && !pack.catalog.find(*pattern.definition_id)) {
                return Result<void>::fail(makeError(ErrorCode::id_definition_not_found, "ReactionContentPack pattern definition missing"));
            }
        }
        for (const auto& output : rule.output_templates) {
            if ((output.output_kind == ReactionOutputKind::TransformObject || output.output_kind == ReactionOutputKind::ProduceObject) &&
                !pack.catalog.find(output.product_definition_id)) {
                return Result<void>::fail(makeError(ErrorCode::id_definition_not_found, "ReactionContentPack output product definition missing"));
            }
        }
    }

    std::set<std::string> template_keys;
    for (const auto& tmpl : pack.knowledge_templates) {
        auto valid = tmpl.validateBasic();
        if (valid.is_error()) return valid;
        if (!hasRule(pack.rules, tmpl.rule_key)) {
            return Result<void>::fail(makeError(ErrorCode::id_not_found, "ReactionContentPack knowledge template rule missing"));
        }
        if (!template_keys.insert(tmpl.rule_key).second) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "ReactionContentPack duplicate knowledge template"));
        }
    }

    return Result<void>::ok();
}

} // namespace pathfinder::reaction
