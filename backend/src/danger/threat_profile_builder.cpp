#include "pathfinder/danger/threat_profile_builder.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <utility>

namespace pathfinder::danger {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static bool hasKey(const std::vector<std::string>& values, const std::string& key) {
    return std::find(values.begin(), values.end(), key) != values.end();
}

Result<ThreatProfile> ThreatProfileBuilder::fromRuntimeObject(const pathfinder::reaction::ReactionRuntimeObject& object) const {
    auto valid = object.validateBasic();
    if (valid.is_error()) return Result<ThreatProfile>::fail(valid.errors());

    ThreatProfile profile;
    profile.threat_key = object.object_key.empty() ? object.definition_id.value() : object.object_key;
    profile.source_kind = DangerSourceKind::ObjectReaction;
    profile.source_object_id = object.object_id;
    profile.source_definition_id = object.definition_id;
    profile.public_name_key = object.object_key;
    profile.visibility = 1.0;
    profile.predictability = 0.45;
    profile.safe_tag_keys = object.tag_keys;
    profile.state_keys = object.state_keys;
    profile.reason_keys = {"threat_from_runtime_object"};

    if (hasKey(object.tag_keys, "fire_source") || hasKey(object.state_keys, "burning")) {
        profile.base_pressure = 0.75;
        profile.proximity_pressure = 0.60;
        profile.base_severity = DangerSeverity::Harm;
        profile.reason_keys.push_back("burning_object_hazard");
    } else if (hasKey(object.state_keys, "rotten") || object.object_key.find("rotten") != std::string::npos) {
        profile.base_pressure = 0.65;
        profile.proximity_pressure = 0.35;
        profile.base_severity = DangerSeverity::Harm;
        profile.reason_keys.push_back("rotten_food_hazard");
    } else if (hasKey(object.tag_keys, "sharp") || hasKey(object.tag_keys, "weapon")) {
        profile.base_pressure = 0.45;
        profile.proximity_pressure = 0.45;
        profile.base_severity = DangerSeverity::Strain;
        profile.reason_keys.push_back("sharp_object_hazard");
    } else {
        profile.base_pressure = 0.20;
        profile.proximity_pressure = 0.20;
        profile.base_severity = DangerSeverity::Notice;
        profile.reason_keys.push_back("unknown_object_caution");
    }

    auto profile_valid = profile.validateBasic();
    if (profile_valid.is_error()) return Result<ThreatProfile>::fail(profile_valid.errors());
    return Result<ThreatProfile>::ok(std::move(profile));
}

Result<ThreatProfile> ThreatProfileBuilder::fromCreatureProjection(
    pathfinder::foundation::EntityId creature_id,
    std::string creature_key,
    DangerSeverity severity,
    double base_pressure) const {

    if (creature_id.empty() || !pathfinder::foundation::isValidIdString(creature_id.value())) {
        return Result<ThreatProfile>::fail(makeError(ErrorCode::id_invalid_format, "creature_id invalid"));
    }
    if (creature_key.empty() || containsDangerForbiddenKey(creature_key)) {
        return Result<ThreatProfile>::fail(makeError(ErrorCode::validation_failed, "creature_key invalid"));
    }
    if (!validDangerRatio(base_pressure)) {
        return Result<ThreatProfile>::fail(makeError(ErrorCode::validation_value_out_of_range, "base_pressure must be 0..1"));
    }

    ThreatProfile profile;
    profile.threat_key = std::move(creature_key);
    profile.source_kind = DangerSourceKind::Creature;
    profile.source_entity_id = std::move(creature_id);
    profile.public_name_key = profile.threat_key;
    profile.base_pressure = base_pressure;
    profile.proximity_pressure = 0.65;
    profile.visibility = 0.85;
    profile.predictability = 0.35;
    profile.base_severity = severity;
    profile.safe_tag_keys = {"creature"};
    profile.state_keys = {"alert"};
    profile.reason_keys = {"threat_from_creature_projection"};

    auto valid = profile.validateBasic();
    if (valid.is_error()) return Result<ThreatProfile>::fail(valid.errors());
    return Result<ThreatProfile>::ok(std::move(profile));
}

} // namespace pathfinder::danger
