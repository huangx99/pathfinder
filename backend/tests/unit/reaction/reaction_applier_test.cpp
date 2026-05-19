#include "pathfinder/reaction/reaction_applier.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::reaction;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ObjectId;
using pathfinder::object::ObjectCategory;

static ItemDefinitionContent makeItem(const std::string& id, const std::string& key, ObjectCategory category) {
    ItemDefinitionContent item;
    item.definition_id = ObjectDefinitionId(id);
    item.object_key = key;
    item.display_name = key;
    item.category = category;
    item.tag_keys = {key + "_tag"};
    item.safe_description = key;
    return item;
}

static ItemCatalog catalog() {
    ItemCatalog catalog;
    auto branch = makeItem("def_dry_branch", "dry_branch", ObjectCategory::Material);
    branch.tag_keys = {"combustible", "branch_like"};
    branch.default_state_keys = {"dry"};
    assert(catalog.addItem(branch).is_ok());
    auto torch = makeItem("def_torch", "torch", ObjectCategory::Tool);
    torch.tag_keys = {"tool", "light_source", "combustible"};
    torch.default_state_keys = {"burning", "usable"};
    torch.default_resource_values["fuel"] = 100.0;
    assert(catalog.addItem(torch).is_ok());
    auto ember = makeItem("def_ember", "ember", ObjectCategory::Material);
    ember.tag_keys = {"hot_material"};
    ember.default_state_keys = {"hot"};
    assert(catalog.addItem(ember).is_ok());
    return catalog;
}

static ReactionResult transformResult() {
    ReactionResult result;
    result.decision = ReactionDecision::Reacted;
    result.selected_rule_key = "fire_dry_branch_to_torch";
    result.trace.decision = ReactionDecision::Reacted;
    result.trace.input_key = "input_fire_branch";
    result.trace.selected_rule_key = result.selected_rule_key;
    ReactionObjectChangeDraft change;
    change.change_kind = ReactionOutputKind::TransformObject;
    change.object_id = ObjectId("obj_branch");
    change.role = ReactionObjectRole::Material;
    change.from_definition_id = ObjectDefinitionId("def_dry_branch");
    change.to_definition_id = ObjectDefinitionId("def_torch");
    change.reason_keys = {"fire_dry_branch_to_torch"};
    result.object_changes = {change};
    return result;
}

static void test_transform_applies_defaults_and_stays_usable() {
    auto cat = catalog();
    ReactionRuntimeWorld world;
    auto branch = cat.instantiate(ObjectId("obj_branch"), ObjectDefinitionId("def_dry_branch"), "inventory");
    assert(branch.is_ok());
    assert(world.addObject(branch.value()).is_ok());

    ReactionChangeApplier applier;
    auto applied = applier.apply(world, cat, transformResult());
    assert(applied.is_ok());
    const auto* torch = applied.value().world.findObject(ObjectId("obj_branch"));
    assert(torch != nullptr);
    assert(torch->definition_id.value() == "def_torch");
    assert(torch->object_key == "torch");
    assert(torch->category == ObjectCategory::Tool);
    assert(torch->resource_values.at("fuel") == 100.0);

    ReactionInputBuilder builder;
    auto ref = builder.buildObjectRef(*torch, ReactionObjectRole::Tool);
    assert(ref.is_ok());
    assert(ref.value().definition_id.value() == "def_torch");
    std::cout << "transform_applies_defaults_and_stays_usable passed" << std::endl;
}

static void test_produce_object_creates_runtime_instance() {
    auto cat = catalog();
    ReactionRuntimeWorld world;
    ReactionResult result;
    result.decision = ReactionDecision::Reacted;
    result.selected_rule_key = "produce_ember";
    result.trace.decision = ReactionDecision::Reacted;
    result.trace.input_key = "input_produce";
    result.trace.selected_rule_key = result.selected_rule_key;
    ReactionObjectChangeDraft change;
    change.change_kind = ReactionOutputKind::ProduceObject;
    change.role = ReactionObjectRole::Product;
    change.to_definition_id = ObjectDefinitionId("def_ember");
    change.reason_keys = {"produce_ember"};
    result.object_changes = {change};

    ReactionApplyOptions options;
    options.default_location_key = "ground";
    options.generated_object_prefix = "obj_ember";
    ReactionChangeApplier applier;
    auto applied = applier.apply(world, cat, result, options);
    assert(applied.is_ok());
    assert(applied.value().world.objects().size() == 1);
    assert(applied.value().world.objects().front().definition_id.value() == "def_ember");
    assert(applied.value().trace.generated_object_ids.front() == "obj_ember_1");
    std::cout << "produce_object_creates_runtime_instance passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "transform") test_transform_applies_defaults_and_stays_usable();
    else if (mode == "produce") test_produce_object_creates_runtime_instance();
    else {
        test_transform_applies_defaults_and_stays_usable();
        test_produce_object_creates_runtime_instance();
    }
    return 0;
}
