#include "pathfinder/object/types.h"
#include <stdexcept>

namespace pathfinder::object {

// ObjectCategory
std::string toString(ObjectCategory category) {
    switch (category) {
        case ObjectCategory::Unknown: return "unknown";
        case ObjectCategory::Food: return "food";
        case ObjectCategory::Material: return "material";
        case ObjectCategory::Tool: return "tool";
        case ObjectCategory::Hazard: return "hazard";
    }
    return "unknown";
}

pathfinder::foundation::Result<ObjectCategory> objectCategoryFromString(const std::string& str) {
    if (str == "unknown") return pathfinder::foundation::Result<ObjectCategory>::ok(ObjectCategory::Unknown);
    if (str == "food") return pathfinder::foundation::Result<ObjectCategory>::ok(ObjectCategory::Food);
    if (str == "material") return pathfinder::foundation::Result<ObjectCategory>::ok(ObjectCategory::Material);
    if (str == "tool") return pathfinder::foundation::Result<ObjectCategory>::ok(ObjectCategory::Tool);
    if (str == "hazard") return pathfinder::foundation::Result<ObjectCategory>::ok(ObjectCategory::Hazard);
    return pathfinder::foundation::Result<ObjectCategory>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "invalid ObjectCategory: " + str));
}

// ObjectRuntimeFlag
std::string toString(ObjectRuntimeFlag flag) {
    switch (flag) {
        case ObjectRuntimeFlag::None: return "none";
        case ObjectRuntimeFlag::Consumed: return "consumed";
        case ObjectRuntimeFlag::Depleted: return "depleted";
    }
    return "none";
}

pathfinder::foundation::Result<ObjectRuntimeFlag> objectRuntimeFlagFromString(const std::string& str) {
    if (str == "none") return pathfinder::foundation::Result<ObjectRuntimeFlag>::ok(ObjectRuntimeFlag::None);
    if (str == "consumed") return pathfinder::foundation::Result<ObjectRuntimeFlag>::ok(ObjectRuntimeFlag::Consumed);
    if (str == "depleted") return pathfinder::foundation::Result<ObjectRuntimeFlag>::ok(ObjectRuntimeFlag::Depleted);
    return pathfinder::foundation::Result<ObjectRuntimeFlag>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "invalid ObjectRuntimeFlag: " + str));
}

// EdibleEffectKind
std::string toString(EdibleEffectKind kind) {
    switch (kind) {
        case EdibleEffectKind::Unknown: return "unknown";
        case EdibleEffectKind::Edible: return "edible";
        case EdibleEffectKind::Harmful: return "harmful";
        case EdibleEffectKind::Neutral: return "neutral";
    }
    return "unknown";
}

pathfinder::foundation::Result<EdibleEffectKind> edibleEffectKindFromString(const std::string& str) {
    if (str == "unknown") return pathfinder::foundation::Result<EdibleEffectKind>::ok(EdibleEffectKind::Unknown);
    if (str == "edible") return pathfinder::foundation::Result<EdibleEffectKind>::ok(EdibleEffectKind::Edible);
    if (str == "harmful") return pathfinder::foundation::Result<EdibleEffectKind>::ok(EdibleEffectKind::Harmful);
    if (str == "neutral") return pathfinder::foundation::Result<EdibleEffectKind>::ok(EdibleEffectKind::Neutral);
    return pathfinder::foundation::Result<EdibleEffectKind>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "invalid EdibleEffectKind: " + str));
}

} // namespace pathfinder::object
