#include "pathfinder/reaction/reaction_fixtures.h"
#include <optional>
#include <utility>

namespace pathfinder::reaction::fixtures {

using pathfinder::condition::ConditionExpressionRef;
using pathfinder::foundation::EntityId;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ObjectId;
using pathfinder::foundation::Tick;
using pathfinder::object::ObjectCategory;

static ConditionExpressionRef condition(std::string key) {
    ConditionExpressionRef ref;
    ref.inline_canonical_key = std::move(key);
    return ref;
}

ReactionObjectRef fireSource(bool burning) {
    ReactionObjectRef object;
    object.role = ReactionObjectRole::Source;
    object.object_id = ObjectId("obj_fire_source");
    object.definition_id = ObjectDefinitionId("def_fire_source");
    object.object_key = "fire_source";
    object.category = ObjectCategory::Hazard;
    object.quantity = 1;
    object.tag_keys = {"fire_source"};
    if (burning) object.state_keys = {"burning"};
    object.resource_values["fuel"] = 10.0;
    return object;
}

ReactionObjectRef dryBranch() {
    ReactionObjectRef object;
    object.role = ReactionObjectRole::Material;
    object.object_id = ObjectId("obj_dry_branch");
    object.definition_id = ObjectDefinitionId("def_dry_branch");
    object.object_key = "dry_branch";
    object.category = ObjectCategory::Material;
    object.quantity = 1;
    object.tag_keys = {"combustible", "branch_like"};
    object.state_keys = {"dry"};
    return object;
}

ReactionObjectRef wetBranch() {
    ReactionObjectRef object = dryBranch();
    object.object_id = ObjectId("obj_wet_branch");
    object.definition_id = ObjectDefinitionId("def_wet_branch");
    object.object_key = "wet_branch";
    object.state_keys = {"wet"};
    return object;
}

ReactionObjectRef waterPortion() {
    ReactionObjectRef object;
    object.role = ReactionObjectRole::Material;
    object.object_id = ObjectId("obj_water_portion");
    object.definition_id = ObjectDefinitionId("def_water_portion");
    object.object_key = "water_portion";
    object.category = ObjectCategory::Material;
    object.quantity = 1;
    object.tag_keys = {"water_source"};
    object.state_keys = {"available"};
    return object;
}

ObjectReactionRule fireDryBranchToTorchRule() {
    ObjectReactionRule rule;
    rule.rule_key = "fire_dry_branch_to_torch";
    rule.action_kind = ReactionActionKind::Combine;
    rule.object_patterns = {
        ReactionObjectPattern{ReactionObjectRole::Source, ObjectDefinitionId("def_fire_source"), ObjectCategory::Hazard, {"fire_source"}, {}, true},
        ReactionObjectPattern{ReactionObjectRole::Material, std::nullopt, ObjectCategory::Material, {"combustible"}, {}, true}
    };
    rule.condition_refs = {
        condition("condition:source_state:eq:burning"),
        condition("condition:target_state:eq:dry"),
        condition("condition:target_tag:has:combustible")
    };
    ReactionOutputTemplate transform;
    transform.output_kind = ReactionOutputKind::TransformObject;
    transform.target_role = ReactionObjectRole::Material;
    transform.product_definition_id = ObjectDefinitionId("def_torch");
    transform.knowledge_key = "transforms_into_torch";
    ReactionOutputTemplate fuel;
    fuel.output_kind = ReactionOutputKind::ResourceDelta;
    fuel.target_role = ReactionObjectRole::Source;
    fuel.resource_key = "fuel";
    fuel.resource_delta = -5.0;
    rule.output_templates = {transform, fuel};
    rule.priority = 100;
    rule.conflict_policy = ReactionConflictPolicy::HighestPriorityOnly;
    rule.feedback_key = "reaction.fire_branch_torch";
    rule.feedback_text = "木棍被点燃，变成了可以携带的火把。";
    rule.knowledge_effect_key = "transform_to_torch";
    rule.safe_tags = {"fire", "tool_creation"};
    return rule;
}

ObjectReactionRule waterExtinguishFireRule() {
    ObjectReactionRule rule;
    rule.rule_key = "water_extinguish_fire";
    rule.action_kind = ReactionActionKind::Combine;
    rule.object_patterns = {
        ReactionObjectPattern{ReactionObjectRole::Source, ObjectDefinitionId("def_fire_source"), ObjectCategory::Hazard, {"fire_source"}, {}, true},
        ReactionObjectPattern{ReactionObjectRole::Material, ObjectDefinitionId("def_water_portion"), ObjectCategory::Material, {"water_source"}, {}, true}
    };
    rule.condition_refs = {
        condition("condition:source_state:eq:burning"),
        condition("condition:target_tag:has:water_source")
    };
    ReactionOutputTemplate remove;
    remove.output_kind = ReactionOutputKind::RemoveState;
    remove.target_role = ReactionObjectRole::Source;
    remove.state_key = "burning";
    ReactionOutputTemplate consume;
    consume.output_kind = ReactionOutputKind::ConsumeObject;
    consume.target_role = ReactionObjectRole::Material;
    consume.quantity_delta = -1;
    rule.output_templates = {remove, consume};
    rule.priority = 90;
    rule.conflict_policy = ReactionConflictPolicy::HighestPriorityOnly;
    rule.feedback_key = "reaction.water_extinguish_fire";
    rule.feedback_text = "水浇灭了火源。";
    rule.knowledge_effect_key = "extinguish_fire";
    rule.safe_tags = {"fire", "water"};
    return rule;
}

ObjectReactionRule cutWoodWithAxeRule() {
    ObjectReactionRule rule;
    rule.rule_key = "cut_wood_with_axe";
    rule.action_kind = ReactionActionKind::Use;
    rule.object_patterns = {
        ReactionObjectPattern{ReactionObjectRole::Material, ObjectDefinitionId("def_raw_wood"), ObjectCategory::Material, {"wood_material"}, {}, true},
        ReactionObjectPattern{ReactionObjectRole::Tool, ObjectDefinitionId("def_axe"), ObjectCategory::Tool, {"cutting_tool"}, {}, true}
    };
    rule.condition_refs = {
        condition("condition:source_state:eq:sharp")
    };
    ReactionOutputTemplate product;
    product.output_kind = ReactionOutputKind::ProduceObject;
    product.target_role = ReactionObjectRole::Product;
    product.product_definition_id = ObjectDefinitionId("def_wood_processed");
    product.quantity_delta = 1;
    rule.output_templates = {product};
    rule.priority = 80;
    rule.conflict_policy = ReactionConflictPolicy::HighestPriorityOnly;
    rule.feedback_key = "reaction.cut_wood";
    rule.feedback_text = "斧头把木头处理成可用材料。";
    rule.knowledge_effect_key = "cut_wood";
    rule.safe_tags = {"material_processing", "tool_use"};
    return rule;
}

ObjectReactionRule sharpenAxeRule() {
    ObjectReactionRule rule;
    rule.rule_key = "sharpen_axe";
    rule.action_kind = ReactionActionKind::Use;
    rule.object_patterns = {
        ReactionObjectPattern{ReactionObjectRole::Material, ObjectDefinitionId("def_whetstone"), ObjectCategory::Tool, {"sharpening_tool"}, {}, true},
        ReactionObjectPattern{ReactionObjectRole::Tool, ObjectDefinitionId("def_axe"), ObjectCategory::Tool, {"cutting_tool"}, {}, true}
    };
    rule.condition_refs = {
        condition("condition:target_tag:has:cutting_tool")
    };
    ReactionOutputTemplate sharpen;
    sharpen.output_kind = ReactionOutputKind::ResourceDelta;
    sharpen.target_role = ReactionObjectRole::Tool;
    sharpen.resource_key = "sharpness";
    sharpen.resource_delta = 3.0;
    rule.output_templates = {sharpen};
    rule.priority = 70;
    rule.conflict_policy = ReactionConflictPolicy::HighestPriorityOnly;
    rule.feedback_key = "reaction.sharpen_axe";
    rule.feedback_text = "斧头被重新打磨锋利。";
    rule.knowledge_effect_key = "restore_sharpness";
    rule.safe_tags = {"tool_maintenance"};
    return rule;
}

ReactionInputSet fireBranchInput(const ReactionObjectRef& branch) {
    ReactionInputSet input;
    input.input_key = "input_fire_branch";
    input.action_kind = ReactionActionKind::Combine;
    input.actor_id = EntityId("actor_pioneer");
    input.objects = {fireSource(true), branch};
    input.tick = Tick(1);
    input.safe_context_keys = {"reaction_test"};
    return input;
}

ReactionInputSet waterFireInput() {
    ReactionInputSet input;
    input.input_key = "input_water_fire";
    input.action_kind = ReactionActionKind::Combine;
    input.actor_id = EntityId("actor_pioneer");
    input.objects = {fireSource(true), waterPortion()};
    input.tick = Tick(2);
    input.safe_context_keys = {"reaction_test"};
    return input;
}

std::vector<ObjectReactionRule> coreP28Rules() {
    return {fireDryBranchToTorchRule(), waterExtinguishFireRule(), cutWoodWithAxeRule(), sharpenAxeRule()};
}

} // namespace pathfinder::reaction::fixtures
