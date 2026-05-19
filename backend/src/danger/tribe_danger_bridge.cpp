#include "pathfinder/danger/tribe_danger_bridge.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <utility>

namespace pathfinder::danger {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static void appendUnique(std::vector<std::string>& values, const std::string& value) {
    if (!value.empty() && std::find(values.begin(), values.end(), value) == values.end()) values.push_back(value);
}

Result<pathfinder::tribe::TribeStateInput> TribeDangerBridge::toTribeStateInput(
    const std::vector<TribeDangerPressureDraft>& drafts,
    pathfinder::foundation::TribeId tribe_id,
    pathfinder::foundation::Tick tick) const {

    if (tribe_id.empty() || !pathfinder::foundation::isValidIdString(tribe_id.value())) {
        return Result<pathfinder::tribe::TribeStateInput>::fail(makeError(ErrorCode::id_invalid_format, "tribe_id invalid"));
    }

    pathfinder::tribe::TribeStateInput input;
    input.tribe_id = std::move(tribe_id);
    input.input_tick = tick;

    for (const auto& draft : drafts) {
        auto valid = draft.validateBasic();
        if (valid.is_error()) return Result<pathfinder::tribe::TribeStateInput>::fail(valid.errors());
        if (!draft.tribe_id.empty() && draft.tribe_id != input.tribe_id) {
            return Result<pathfinder::tribe::TribeStateInput>::fail(makeError(ErrorCode::validation_failed, "danger pressure tribe mismatch"));
        }
        input.morale_delta = std::clamp(input.morale_delta + draft.morale_delta, -1.0, 1.0);
        input.trust_delta = std::clamp(input.trust_delta + draft.trust_delta, -1.0, 1.0);
        input.safety_pressure = clampDangerRatio(input.safety_pressure + draft.safety_pressure);
        input.resource_pressure = clampDangerRatio(input.resource_pressure + draft.resource_pressure);
        input.knowledge_conflict_pressure = clampDangerRatio(input.knowledge_conflict_pressure + draft.knowledge_conflict_pressure);
        input.casualty_pressure = clampDangerRatio(input.casualty_pressure + draft.casualty_pressure + draft.split_risk_hint * 0.25);
        for (const auto& reason : draft.reason_keys) appendUnique(input.reason_keys, reason);
    }
    appendUnique(input.reason_keys, "p29_danger_pressure_bridge");

    auto input_valid = input.validateBasic();
    if (input_valid.is_error()) return Result<pathfinder::tribe::TribeStateInput>::fail(input_valid.errors());
    return Result<pathfinder::tribe::TribeStateInput>::ok(std::move(input));
}

} // namespace pathfinder::danger
