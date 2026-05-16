#include "pathfinder/cognition/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_cognition_types_tests() {
    // CognitionEffectKind toString
    assert(toString(CognitionEffectKind::Unknown) == "unknown");
    assert(toString(CognitionEffectKind::Edible) == "edible");
    assert(toString(CognitionEffectKind::Harmful) == "harmful");
    assert(toString(CognitionEffectKind::HungerChanged) == "hunger_changed");
    assert(toString(CognitionEffectKind::HealthChanged) == "health_changed");

    // CognitionEffectKind fromString
    {
        auto r = cognitionEffectKindFromString("edible");
        assert(r.is_ok());
        assert(r.value() == CognitionEffectKind::Edible);
    }
    {
        auto r = cognitionEffectKindFromString("invalid");
        assert(r.is_error());
    }

    // CognitionKey validateBasic
    {
        CognitionKey key;
        assert(key.validateBasic().is_error()); // empty subject_id

        key.subject_id = EntityId("actor_001");
        assert(key.validateBasic().is_error()); // empty object_definition_id

        key.object_definition_id = ObjectDefinitionId("unknown_fruit");
        assert(key.validateBasic().is_error()); // empty action_id

        key.action_id = ActionId("eat");
        assert(key.validateBasic().is_ok());

        key.effect_kind = CognitionEffectKind::Edible;
        assert(key.validateBasic().is_ok());
    }

    // CognitionKey comparison
    {
        CognitionKey a{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};
        CognitionKey b{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};
        CognitionKey c{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Harmful};
        assert(a == b);
        assert(!(a == c));
    }

    // CognitionEvidence validateBasic
    {
        CognitionKey key{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};
        CognitionEvidence evidence;
        evidence.key = key;
        evidence.confidence_delta = 0.25;
        assert(evidence.validateBasic().is_ok());

        evidence.confidence_delta = 1.5;
        assert(evidence.validateBasic().is_error()); // out of range

        evidence.confidence_delta = -1.5;
        assert(evidence.validateBasic().is_error()); // out of range
    }

    std::cout << "cognition_types tests passed" << std::endl;
}
