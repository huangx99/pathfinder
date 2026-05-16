#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include <string>

namespace pathfinder::cognition {

using pathfinder::foundation::EntityId;
using pathfinder::foundation::ObjectDefinitionId;
using pathfinder::foundation::ActionId;
using pathfinder::foundation::EventId;
using pathfinder::foundation::Result;

enum class CognitionEffectKind {
    Unknown,
    Edible,
    Harmful,
    HungerChanged,
    HealthChanged
};

std::string toString(CognitionEffectKind kind);
Result<CognitionEffectKind> cognitionEffectKindFromString(const std::string& str);

// CognitionKey: subject_id + object_definition_id + action_id + effect_kind
struct CognitionKey {
    EntityId subject_id;
    ObjectDefinitionId object_definition_id;
    ActionId action_id;
    CognitionEffectKind effect_kind = CognitionEffectKind::Unknown;

    bool operator==(const CognitionKey& other) const = default;

    Result<void> validateBasic() const;
};

// CognitionEvidence: a single piece of feedback evidence
struct CognitionEvidence {
    CognitionKey key;
    EventId source_event_id;
    double confidence_delta = 0.0;
    int observed_hunger_delta = 0;
    int observed_health_delta = 0;

    Result<void> validateBasic() const;
};

} // namespace pathfinder::cognition
