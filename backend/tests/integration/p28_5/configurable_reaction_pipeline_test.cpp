#include "pathfinder/reaction/reaction_applier.h"
#include "pathfinder/reaction/reaction_learning_bridge.h"
#include "pathfinder/reaction/reaction_resolver.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::reaction;
using pathfinder::condition::ConditionExpressionRef;
using pathfinder::foundation::EntityId;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ObjectId;
using pathfinder::foundation::Tick;
using pathfinder::object::ObjectCategory;

static ConditionExpressionRef condition(const std::string& key) {
    ConditionExpressionRef ref;
    ref.inline_canonical_key = key;
    return ref;
}

static ItemDefinitionContent item(const std::string& id, const std::string& key, ObjectCategory category) {
    ItemDefinitionContent value;
    value.definition_id = ObjectDefinitionId(id);
    value.object_key = key;
    value.display_name = key;
    value.category = category;
    value.safe_description = key;
    return value;
}

static ObjectReactionRule fireTorchRule() {
    ObjectReactionRule rule;
    rule.rule_key = "fire_dry_branch_to_torch";
    rule.action_kind = ReactionActionKind::Combine;

    ReactionObjectPattern source;
    source.role = ReactionObjectRole::Source;
    source.definition_id = ObjectDefinitionId("def_fire_source");
    source.category = ObjectCategory::Hazard;
    source.required_tag_keys = {"fire_source"};

    ReactionObjectPattern material;
    material.role = ReactionObjectRole::Material;
    material.definition_id = ObjectDefinitionId("def_dry_branch");
    material.category = ObjectCategory::Material;
    material.required_tag_keys = {"combustible", "branch_like"};

    rule.object_patterns = {source, material};
    rule.condition_refs = {condition("condition:source_state:eq:burning"), condition("condition:target_state:eq:dry")};

    ReactionOutputTemplate transform;
    transform.output_kind = ReactionOutputKind::TransformObject;
    transform.target_role = ReactionObjectRole::Material;
    transform.product_definition_id = ObjectDefinitionId("def_torch");

    ReactionOutputTemplate fuel;
    fuel.output_kind = ReactionOutputKind::ResourceDelta;
    fuel.target_role = ReactionObjectRole::Source;
    fuel.resource_key = "fuel";
    fuel.resource_delta = -5.0;

    rule.output_templates = {transform, fuel};
    rule.feedback_key = "reaction.fire_branch_torch";
    rule.feedback_text = "木棍被点燃，变成了可以携带的火把。";
    rule.knowledge_effect_key = "transform_to_torch";
    rule.priority = 100;
    return rule;
}

static ObjectReactionRule drinkRule() {
    ObjectReactionRule rule;
    rule.rule_key = "drink_clean_water";
    rule.action_kind = ReactionActionKind::Use;
    ReactionObjectPattern target;
    target.role = ReactionObjectRole::Target;
    target.definition_id = ObjectDefinitionId("def_clean_water");
    target.category = ObjectCategory::Food;
    target.required_tag_keys = {"drinkable"};
    rule.object_patterns = {target};
    rule.condition_refs = {condition("condition:target_state:eq:clean"), condition("condition:target_state:eq:available")};
    ReactionOutputTemplate consume;
    consume.output_kind = ReactionOutputKind::ConsumeObject;
    consume.target_role = ReactionObjectRole::Target;
    consume.quantity_delta = -1;
    rule.output_templates = {consume};
    rule.feedback_key = "reaction.drink_clean_water";
    rule.feedback_text = "你喝下清水，身体舒服了一些。";
    rule.knowledge_effect_key = "safe_to_drink";
    rule.priority = 10;
    return rule;
}

static ReactionContentPack contentPack() {
    ReactionContentPack pack;
    auto fire = item("def_fire_source", "fire_source", ObjectCategory::Hazard);
    fire.tag_keys = {"fire_source"};
    fire.default_state_keys = {"burning"};
    fire.default_resource_values["fuel"] = 10.0;
    assert(pack.catalog.addItem(fire).is_ok());

    auto branch = item("def_dry_branch", "dry_branch", ObjectCategory::Material);
    branch.tag_keys = {"combustible", "branch_like"};
    branch.default_state_keys = {"dry"};
    assert(pack.catalog.addItem(branch).is_ok());

    auto torch = item("def_torch", "torch", ObjectCategory::Tool);
    torch.tag_keys = {"tool", "light_source", "combustible"};
    torch.default_state_keys = {"burning", "usable"};
    torch.default_resource_values["fuel"] = 100.0;
    assert(pack.catalog.addItem(torch).is_ok());

    auto water = item("def_clean_water", "clean_water", ObjectCategory::Food);
    water.tag_keys = {"drinkable", "water_source"};
    water.default_state_keys = {"clean", "available"};
    assert(pack.catalog.addItem(water).is_ok());

    pack.rules = {fireTorchRule(), drinkRule()};

    ReactionKnowledgeTemplate torch_knowledge;
    torch_knowledge.rule_key = "fire_dry_branch_to_torch";
    torch_knowledge.subject_policy = ReactionKnowledgeSubjectPolicy::ExplicitSubject;
    torch_knowledge.subject_id = "def_dry_branch";
    torch_knowledge.action_key = "combine_with_fire";
    torch_knowledge.effect_key = "transform_to_torch";

    ReactionKnowledgeTemplate drink_knowledge;
    drink_knowledge.rule_key = "drink_clean_water";
    drink_knowledge.subject_policy = ReactionKnowledgeSubjectPolicy::ExplicitSubject;
    drink_knowledge.subject_id = "def_clean_water";
    drink_knowledge.action_key = "drink";
    drink_knowledge.effect_key = "safe_to_drink";

    pack.knowledge_templates = {torch_knowledge, drink_knowledge};
    return pack;
}

static void test_configurable_reaction_to_usable_item_and_npc_learning() {
    auto pack = contentPack();
    ReactionContentValidator validator;
    assert(validator.validate(pack).is_ok());

    ReactionRuntimeWorld world;
    auto fire = pack.catalog.instantiate(ObjectId("obj_fire"), ObjectDefinitionId("def_fire_source"), "camp");
    auto branch = pack.catalog.instantiate(ObjectId("obj_branch"), ObjectDefinitionId("def_dry_branch"), "inventory");
    assert(fire.is_ok());
    assert(branch.is_ok());
    assert(world.addObject(fire.value()).is_ok());
    assert(world.addObject(branch.value()).is_ok());

    ReactionInputBuilder builder;
    auto input = builder.buildInput(
        world,
        ReactionActionKind::Combine,
        EntityId("actor_pioneer"),
        {{ObjectId("obj_fire"), ReactionObjectRole::Source}, {ObjectId("obj_branch"), ReactionObjectRole::Material}},
        Tick(1),
        "input_make_torch");
    assert(input.is_ok());

    ObjectReactionResolver resolver;
    auto result = resolver.resolve(input.value(), pack.rules);
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::Reacted);
    assert(!result.value().knowledge_drafts.empty());

    ReactionChangeApplier applier;
    auto applied = applier.apply(world, pack.catalog, result.value());
    assert(applied.is_ok());
    const auto* torch = applied.value().world.findObject(ObjectId("obj_branch"));
    assert(torch != nullptr);
    assert(torch->definition_id.value() == "def_torch");
    assert(torch->object_key == "torch");

    auto torch_ref = builder.buildObjectRef(*torch, ReactionObjectRole::Tool);
    assert(torch_ref.is_ok());
    assert(torch_ref.value().definition_id.value() == "def_torch");
    assert(torch_ref.value().tag_keys.size() == 3);

    ReactionKnowledgePlannerInput learning;
    learning.actor_id = EntityId("actor_pioneer");
    learning.observer_ids = {EntityId("actor_companion")};
    learning.result = result.value();
    learning.knowledge_templates = pack.knowledge_templates;
    ReactionKnowledgePlanner planner;
    auto plan = planner.plan(learning);
    assert(plan.is_ok());
    assert(plan.value().deliveries.size() == 2);
    assert(plan.value().deliveries[0].source_kind == ReactionLearningSourceKind::DirectExperience);
    assert(plan.value().deliveries[1].source_kind == ReactionLearningSourceKind::Observation);
    assert(plan.value().deliveries[1].owner_id.value() == "actor_companion");
    assert(plan.value().deliveries[1].knowledge.effect_key == "transform_to_torch");
    std::cout << "configurable_reaction_to_usable_item_and_npc_learning passed" << std::endl;
}

static void test_configurable_drink_is_single_object_content() {
    auto pack = contentPack();
    ReactionRuntimeWorld world;
    auto water = pack.catalog.instantiate(ObjectId("obj_water"), ObjectDefinitionId("def_clean_water"), "inventory");
    assert(water.is_ok());
    assert(world.addObject(water.value()).is_ok());

    ReactionInputBuilder builder;
    auto input = builder.buildInput(
        world,
        ReactionActionKind::Use,
        EntityId("actor_pioneer"),
        {{ObjectId("obj_water"), ReactionObjectRole::Target}},
        Tick(2),
        "input_drink_water");
    assert(input.is_ok());

    ObjectReactionResolver resolver;
    auto result = resolver.resolve(input.value(), pack.rules);
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::Reacted);

    ReactionChangeApplier applier;
    auto applied = applier.apply(world, pack.catalog, result.value());
    assert(applied.is_ok());
    const auto* water_after = applied.value().world.findObject(ObjectId("obj_water"));
    assert(water_after != nullptr);
    assert(water_after->quantity == 0);
    std::cout << "configurable_drink_is_single_object_content passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "torch_learning") test_configurable_reaction_to_usable_item_and_npc_learning();
    else if (mode == "drink") test_configurable_drink_is_single_object_content();
    else {
        test_configurable_reaction_to_usable_item_and_npc_learning();
        test_configurable_drink_is_single_object_content();
    }
    return 0;
}
