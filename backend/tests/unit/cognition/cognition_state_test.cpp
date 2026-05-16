#include "pathfinder/cognition/cognition_state.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::cognition;
using namespace pathfinder::foundation;

void run_cognition_state_tests() {
    // Empty state
    {
        CognitionState state;
        assert(state.claims().empty());

        CognitionKey key{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};
        assert(state.findClaim(key) == nullptr);
    }

    // First evidence creates claim
    {
        CognitionState state;
        CognitionKey key{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};

        CognitionEvidence evidence;
        evidence.key = key;
        evidence.source_event_id = EventId("evt_001");
        evidence.confidence_delta = 0.25;

        auto result = CognitionUpdater::applyEvidence(state, evidence);
        assert(result.is_ok());
        assert(result.value().confidence == 0.25);
        assert(result.value().evidence_count == 1);
        assert(result.value().last_event_id == EventId("evt_001"));

        const auto* found = state.findClaim(key);
        assert(found != nullptr);
        assert(found->confidence == 0.25);
    }

    // Second evidence enhances confidence
    {
        CognitionState state;
        CognitionKey key{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};

        CognitionEvidence e1;
        e1.key = key;
        e1.source_event_id = EventId("evt_001");
        e1.confidence_delta = 0.25;
        CognitionUpdater::applyEvidence(state, e1);

        CognitionEvidence e2;
        e2.key = key;
        e2.source_event_id = EventId("evt_002");
        e2.confidence_delta = 0.30;

        auto result = CognitionUpdater::applyEvidence(state, e2);
        assert(result.is_ok());
        assert(result.value().confidence == 0.55);
        assert(result.value().evidence_count == 2);
        assert(result.value().last_event_id == EventId("evt_002"));
    }

    // Confidence clamps to 1.0
    {
        CognitionState state;
        CognitionKey key{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};

        CognitionEvidence e1;
        e1.key = key;
        e1.source_event_id = EventId("evt_001");
        e1.confidence_delta = 0.80;
        CognitionUpdater::applyEvidence(state, e1);

        CognitionEvidence e2;
        e2.key = key;
        e2.source_event_id = EventId("evt_002");
        e2.confidence_delta = 0.50;

        auto result = CognitionUpdater::applyEvidence(state, e2);
        assert(result.is_ok());
        assert(result.value().confidence == 1.0);
    }

    // Confidence clamps to 0.0
    {
        CognitionState state;
        CognitionKey key{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};

        CognitionEvidence e1;
        e1.key = key;
        e1.source_event_id = EventId("evt_001");
        e1.confidence_delta = 0.10;
        CognitionUpdater::applyEvidence(state, e1);

        CognitionEvidence e2;
        e2.key = key;
        e2.source_event_id = EventId("evt_002");
        e2.confidence_delta = -0.50;

        auto result = CognitionUpdater::applyEvidence(state, e2);
        assert(result.is_ok());
        assert(result.value().confidence == 0.0);
    }

    // Different keys create separate claims
    {
        CognitionState state;
        CognitionKey key1{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};
        CognitionKey key2{EntityId("actor_001"), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Harmful};

        CognitionEvidence e1;
        e1.key = key1;
        e1.source_event_id = EventId("evt_001");
        e1.confidence_delta = 0.30;
        CognitionUpdater::applyEvidence(state, e1);

        CognitionEvidence e2;
        e2.key = key2;
        e2.source_event_id = EventId("evt_002");
        e2.confidence_delta = 0.40;
        CognitionUpdater::applyEvidence(state, e2);

        assert(state.claims().size() == 2);
    }

    // Invalid evidence fails
    {
        CognitionState state;
        CognitionKey key{EntityId(""), ObjectDefinitionId("unknown_fruit"), ActionId("eat"), CognitionEffectKind::Edible};

        CognitionEvidence evidence;
        evidence.key = key;
        evidence.source_event_id = EventId("evt_001");
        evidence.confidence_delta = 0.25;

        auto result = CognitionUpdater::applyEvidence(state, evidence);
        assert(result.is_error());
        assert(state.claims().empty());
    }

    std::cout << "cognition_state tests passed" << std::endl;
}
