#include "pathfinder/agent/builder_types.h"
#include <set>

namespace pathfinder::agent {

// ObservedThreatSeed
foundation::Result<void> ObservedThreatSeed::validateBasic() const {
    if (source_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "threat_seed_source_id_missing", "source_id must not be empty"));
    }
    if (!foundation::isValidIdString(source_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "threat_seed_source_id_invalid", "source_id has invalid format: " + source_id.value()));
    }
    if (threat_type == AgentThreatType::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "threat_seed_type_unknown", "threat_type must not be Unknown"));
    }
    if (severity < 0.0 || severity > 1.0) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "threat_seed_severity_out_of_range",
                "severity must be in [0.0, 1.0], got " + std::to_string(severity)));
    }
    if (confidence < 0.0 || confidence > 1.0) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "threat_seed_confidence_out_of_range",
                "confidence must be in [0.0, 1.0], got " + std::to_string(confidence)));
    }
    return foundation::Result<void>::ok();
}

// VisibilityInput
foundation::Result<void> VisibilityInput::validateBasic() const {
    // Check for duplicate visible_object_ids
    std::set<std::string> seen;
    for (const auto& id : visible_object_ids) {
        if (!foundation::isValidIdString(id.value())) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::id_invalid_format, "visibility_object_id_invalid", "visible_object_ids contains invalid format: " + id.value()));
        }
        if (!seen.insert(id.value()).second) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::command_duplicate, "visibility_duplicate_object", "visible_object_ids contains duplicate: " + id.value()));
        }
    }
    // Validate threat seeds
    for (const auto& seed : threat_seeds) {
        auto r = seed.validateBasic();
        if (r.is_error()) return r;
    }
    return foundation::Result<void>::ok();
}

// ObservationBuildRequest
foundation::Result<void> ObservationBuildRequest::validateBasic() const {
    auto binding_result = binding.validateBasic();
    if (binding_result.is_error()) return binding_result;
    if (!state) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::command_missing_required_field, "observation_build_null_state", "state must not be nullptr"));
    }
    auto vis_result = visibility.validateBasic();
    if (vis_result.is_error()) return vis_result;
    return foundation::Result<void>::ok();
}

// ObservationBuildResult
foundation::Result<void> ObservationBuildResult::validateBasic() const {
    return observation.validateBasic();
}

// ActionSpaceBuildRequest
foundation::Result<void> ActionSpaceBuildRequest::validateBasic() const {
    return observation.validateBasic();
}

// ActionSpaceBuildResult
foundation::Result<void> ActionSpaceBuildResult::validateBasic() const {
    return action_space.validateBasic();
}

} // namespace pathfinder::agent
