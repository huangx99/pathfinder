#include "pathfinder/object/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::object;
using namespace pathfinder::foundation;

void run_object_types_tests() {
    // ObjectCategory toString
    assert(toString(ObjectCategory::Unknown) == "unknown");
    assert(toString(ObjectCategory::Food) == "food");
    assert(toString(ObjectCategory::Material) == "material");
    assert(toString(ObjectCategory::Tool) == "tool");
    assert(toString(ObjectCategory::Hazard) == "hazard");

    // ObjectCategory fromString
    {
        auto r = objectCategoryFromString("food");
        assert(r.is_ok());
        assert(r.value() == ObjectCategory::Food);
    }
    {
        auto r = objectCategoryFromString("tool");
        assert(r.is_ok());
        assert(r.value() == ObjectCategory::Tool);
    }
    {
        auto r = objectCategoryFromString("invalid");
        assert(r.is_error());
    }

    // ObjectRuntimeFlag toString
    assert(toString(ObjectRuntimeFlag::None) == "none");
    assert(toString(ObjectRuntimeFlag::Consumed) == "consumed");
    assert(toString(ObjectRuntimeFlag::Depleted) == "depleted");

    // ObjectRuntimeFlag fromString
    {
        auto r = objectRuntimeFlagFromString("consumed");
        assert(r.is_ok());
        assert(r.value() == ObjectRuntimeFlag::Consumed);
    }
    {
        auto r = objectRuntimeFlagFromString("invalid");
        assert(r.is_error());
    }

    // EdibleEffectKind toString
    assert(toString(EdibleEffectKind::Unknown) == "unknown");
    assert(toString(EdibleEffectKind::Edible) == "edible");
    assert(toString(EdibleEffectKind::Harmful) == "harmful");
    assert(toString(EdibleEffectKind::Neutral) == "neutral");

    // EdibleEffectKind fromString
    {
        auto r = edibleEffectKindFromString("edible");
        assert(r.is_ok());
        assert(r.value() == EdibleEffectKind::Edible);
    }
    {
        auto r = edibleEffectKindFromString("harmful");
        assert(r.is_ok());
        assert(r.value() == EdibleEffectKind::Harmful);
    }
    {
        auto r = edibleEffectKindFromString("invalid");
        assert(r.is_error());
    }

    // ObjectEdibleProfile default values
    {
        ObjectEdibleProfile profile;
        assert(profile.effect_kind == EdibleEffectKind::Unknown);
        assert(profile.hunger_delta == 0);
        assert(profile.health_delta == 0);
        assert(profile.risk_level == 0);
    }

    // ObjectEdibleProfile comparison
    {
        ObjectEdibleProfile a{EdibleEffectKind::Edible, -20, 0, 0};
        ObjectEdibleProfile b{EdibleEffectKind::Edible, -20, 0, 0};
        ObjectEdibleProfile c{EdibleEffectKind::Harmful, 0, -10, 2};
        assert(a == b);
        assert(!(a == c));
    }

    std::cout << "object_types tests passed" << std::endl;
}
