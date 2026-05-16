#pragma once

#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::object {

enum class ObjectCategory {
    Unknown,
    Food,
    Material,
    Tool,
    Hazard
};

std::string toString(ObjectCategory category);
pathfinder::foundation::Result<ObjectCategory> objectCategoryFromString(const std::string& str);

enum class ObjectRuntimeFlag {
    None,
    Consumed,
    Depleted
};

std::string toString(ObjectRuntimeFlag flag);
pathfinder::foundation::Result<ObjectRuntimeFlag> objectRuntimeFlagFromString(const std::string& str);

enum class EdibleEffectKind {
    Unknown,
    Edible,
    Harmful,
    Neutral
};

std::string toString(EdibleEffectKind kind);
pathfinder::foundation::Result<EdibleEffectKind> edibleEffectKindFromString(const std::string& str);

struct ObjectEdibleProfile {
    EdibleEffectKind effect_kind = EdibleEffectKind::Unknown;
    int hunger_delta = 0;
    int health_delta = 0;
    int risk_level = 0;

    bool operator==(const ObjectEdibleProfile& other) const = default;
};

} // namespace pathfinder::object
