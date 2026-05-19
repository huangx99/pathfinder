#include "pathfinder/reaction/reaction_content.h"
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

static ObjectReactionRule drinkRule() {
    ObjectReactionRule rule;
    rule.rule_key = "drink_clean_water";
    rule.action_kind = ReactionActionKind::Use;
    ReactionObjectPattern pattern;
    pattern.role = ReactionObjectRole::Target;
    pattern.definition_id = ObjectDefinitionId("def_clean_water");
    pattern.category = ObjectCategory::Food;
    pattern.required_tag_keys = {"drinkable"};
    rule.object_patterns = {pattern};
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

static ReactionContentPack validPack() {
    ReactionContentPack pack;
    auto water = item("def_clean_water", "clean_water", ObjectCategory::Food);
    water.tag_keys = {"drinkable", "water_source"};
    water.default_state_keys = {"clean", "available"};
    assert(pack.catalog.addItem(water).is_ok());
    pack.rules = {drinkRule()};
    ReactionKnowledgeTemplate tmpl;
    tmpl.rule_key = "drink_clean_water";
    tmpl.subject_policy = ReactionKnowledgeSubjectPolicy::ExplicitSubject;
    tmpl.subject_id = "def_clean_water";
    tmpl.action_key = "drink";
    tmpl.effect_key = "safe_to_drink";
    pack.knowledge_templates = {tmpl};
    return pack;
}

static void test_content_validator_accepts_drink_pack() {
    ReactionContentPack pack = validPack();
    ReactionContentValidator validator;
    assert(validator.validate(pack).is_ok());
    std::cout << "content_validator_accepts_drink_pack passed" << std::endl;
}

static void test_content_validator_rejects_dynamic_tag() {
    ReactionContentPack pack = validPack();
    auto bad = item("def_bad_branch", "bad_branch", ObjectCategory::Material);
    bad.tag_keys = {"dry"};
    bad.default_state_keys = {"dry"};
    assert(pack.catalog.addItem(bad).is_error());
    std::cout << "content_validator_rejects_dynamic_tag passed" << std::endl;
}

static void test_content_validator_rejects_missing_product() {
    ReactionContentPack pack = validPack();
    auto rule = drinkRule();
    rule.rule_key = "bad_missing_product";
    ReactionOutputTemplate output;
    output.output_kind = ReactionOutputKind::ProduceObject;
    output.target_role = ReactionObjectRole::Product;
    output.product_definition_id = ObjectDefinitionId("def_missing_product");
    rule.output_templates = {output};
    pack.rules.push_back(rule);
    ReactionContentValidator validator;
    assert(validator.validate(pack).is_error());
    std::cout << "content_validator_rejects_missing_product passed" << std::endl;
}

static void test_single_object_drink_rule_resolves() {
    ReactionContentPack pack = validPack();
    ReactionRuntimeWorld world;
    auto water = pack.catalog.instantiate(ObjectId("obj_clean_water"), ObjectDefinitionId("def_clean_water"), "inventory");
    assert(water.is_ok());
    assert(world.addObject(water.value()).is_ok());

    ReactionInputBuilder builder;
    auto input = builder.buildInput(
        world,
        ReactionActionKind::Use,
        EntityId("actor_pioneer"),
        {{ObjectId("obj_clean_water"), ReactionObjectRole::Target}},
        Tick(1),
        "input_drink_water");
    assert(input.is_ok());

    ObjectReactionResolver resolver;
    auto result = resolver.resolve(input.value(), pack.rules);
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::Reacted);
    assert(result.value().selected_rule_key == "drink_clean_water");
    assert(result.value().object_changes.front().change_kind == ReactionOutputKind::ConsumeObject);
    assert(result.value().knowledge_drafts.front().effect_key == "safe_to_drink");
    std::cout << "single_object_drink_rule_resolves passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "valid_pack") test_content_validator_accepts_drink_pack();
    else if (mode == "dynamic_tag") test_content_validator_rejects_dynamic_tag();
    else if (mode == "missing_product") test_content_validator_rejects_missing_product();
    else if (mode == "single_object_drink") test_single_object_drink_rule_resolves();
    else {
        test_content_validator_accepts_drink_pack();
        test_content_validator_rejects_dynamic_tag();
        test_content_validator_rejects_missing_product();
        test_single_object_drink_rule_resolves();
    }
    return 0;
}
