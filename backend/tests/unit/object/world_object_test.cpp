#include "pathfinder/object/world_object.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::object;
using namespace pathfinder::foundation;

void run_world_object_tests() {
    // ObjectDefinition validateBasic
    {
        ObjectDefinition def;
        assert(def.validateBasic().is_error()); // empty definition_id

        def.definition_id = ObjectDefinitionId("unknown_fruit");
        def.category = ObjectCategory::Food;
        assert(def.validateBasic().is_ok());
    }

    // WorldObject validateBasic
    {
        WorldObject obj;
        assert(obj.validateBasic().is_error()); // empty object_id

        obj.object_id = ObjectId("obj_001");
        assert(obj.validateBasic().is_error()); // empty definition_id

        obj.definition_id = ObjectDefinitionId("unknown_fruit");
        assert(obj.validateBasic().is_ok());

        obj.quantity = -1;
        assert(obj.validateBasic().is_error()); // negative quantity
        obj.quantity = 1;
    }

    // ObjectStore add and find
    {
        ObjectStore store;

        ObjectDefinition def;
        def.definition_id = ObjectDefinitionId("unknown_fruit");
        def.category = ObjectCategory::Food;
        def.edible_profile.effect_kind = EdibleEffectKind::Edible;
        def.edible_profile.hunger_delta = -20;
        assert(store.addDefinition(def).is_ok());

        WorldObject obj;
        obj.object_id = ObjectId("obj_001");
        obj.definition_id = ObjectDefinitionId("unknown_fruit");
        obj.quantity = 1;
        assert(store.addObject(obj).is_ok());

        // findDefinition
        auto* found_def = store.findDefinition(ObjectDefinitionId("unknown_fruit"));
        assert(found_def != nullptr);
        assert(found_def->category == ObjectCategory::Food);

        // findObject
        auto* found_obj = store.findObject(ObjectId("obj_001"));
        assert(found_obj != nullptr);
        assert(found_obj->quantity == 1);

        // not found
        assert(store.findObject(ObjectId("nonexistent")) == nullptr);
        assert(store.findDefinition(ObjectDefinitionId("nonexistent")) == nullptr);
    }

    // Duplicate definition_id fails
    {
        ObjectStore store;
        ObjectDefinition def;
        def.definition_id = ObjectDefinitionId("unknown_fruit");
        assert(store.addDefinition(def).is_ok());
        assert(store.addDefinition(def).is_error());
    }

    // Duplicate object_id fails
    {
        ObjectStore store;
        WorldObject obj;
        obj.object_id = ObjectId("obj_001");
        obj.definition_id = ObjectDefinitionId("unknown_fruit");
        assert(store.addObject(obj).is_ok());
        assert(store.addObject(obj).is_error());
    }

    // markConsumed
    {
        ObjectStore store;
        ObjectDefinition def;
        def.definition_id = ObjectDefinitionId("unknown_fruit");
        store.addDefinition(def);

        WorldObject obj;
        obj.object_id = ObjectId("obj_001");
        obj.definition_id = ObjectDefinitionId("unknown_fruit");
        store.addObject(obj);

        assert(store.markConsumed(ObjectId("obj_001")).is_ok());
        auto* found = store.findObject(ObjectId("obj_001"));
        assert(found->flag == ObjectRuntimeFlag::Consumed);

        // markConsumed on non-existent fails
        assert(store.markConsumed(ObjectId("nonexistent")).is_error());
    }

    // validateBasic - object referencing non-existent definition
    {
        ObjectStore store;
        WorldObject obj;
        obj.object_id = ObjectId("obj_001");
        obj.definition_id = ObjectDefinitionId("nonexistent_def");
        store.addObject(obj); // bypass validation for this test

        assert(store.validateBasic().is_error());
    }

    std::cout << "world_object tests passed" << std::endl;
}
