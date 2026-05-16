#include "pathfinder/cognition/types.h"

namespace pathfinder::cognition {

std::string toString(CognitionEffectKind kind) {
    switch (kind) {
        case CognitionEffectKind::Unknown: return "unknown";
        case CognitionEffectKind::Edible: return "edible";
        case CognitionEffectKind::Harmful: return "harmful";
        case CognitionEffectKind::HungerChanged: return "hunger_changed";
        case CognitionEffectKind::HealthChanged: return "health_changed";
    }
    return "unknown";
}

Result<CognitionEffectKind> cognitionEffectKindFromString(const std::string& str) {
    if (str == "unknown") return Result<CognitionEffectKind>::ok(CognitionEffectKind::Unknown);
    if (str == "edible") return Result<CognitionEffectKind>::ok(CognitionEffectKind::Edible);
    if (str == "harmful") return Result<CognitionEffectKind>::ok(CognitionEffectKind::Harmful);
    if (str == "hunger_changed") return Result<CognitionEffectKind>::ok(CognitionEffectKind::HungerChanged);
    if (str == "health_changed") return Result<CognitionEffectKind>::ok(CognitionEffectKind::HealthChanged);
    return Result<CognitionEffectKind>::fail(
        pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "invalid CognitionEffectKind: " + str));
}

Result<void> CognitionKey::validateBasic() const {
    if (subject_id.empty()) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "CognitionKey: subject_id is empty"));
    }
    if (!pathfinder::foundation::isValidIdString(subject_id.value())) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_invalid_format,
                "CognitionKey: subject_id has invalid format"));
    }
    if (object_definition_id.empty()) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "CognitionKey: object_definition_id is empty"));
    }
    if (!pathfinder::foundation::isValidIdString(object_definition_id.value())) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_invalid_format,
                "CognitionKey: object_definition_id has invalid format"));
    }
    if (action_id.empty()) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_failed,
                "CognitionKey: action_id is empty"));
    }
    if (!pathfinder::foundation::isValidIdString(action_id.value())) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_invalid_format,
                "CognitionKey: action_id has invalid format"));
    }
    return Result<void>::ok();
}

Result<void> CognitionEvidence::validateBasic() const {
    auto kr = key.validateBasic();
    if (kr.is_error()) return kr;
    if (confidence_delta < -1.0 || confidence_delta > 1.0) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::validation_value_out_of_range,
                "CognitionEvidence: confidence_delta must be -1.0 to 1.0"));
    }
    if (!source_event_id.empty() && !pathfinder::foundation::isValidIdString(source_event_id.value())) {
        return Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::id_invalid_format,
                "CognitionEvidence: source_event_id has invalid format"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::cognition
