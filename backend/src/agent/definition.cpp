#include "pathfinder/agent/definition.h"
#include "pathfinder/foundation/id.h"

namespace pathfinder::agent {

foundation::Result<void> AgentProfile::validateBasic() const {
    auto check_range = [](double val, const char* field) -> foundation::Result<void> {
        if (val < 0.0 || val > 1.0) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "profile_value_out_of_range",
                    std::string(field) + " must be in [0.0, 1.0], got " + std::to_string(val)));
        }
        return foundation::Result<void>::ok();
    };

    auto r = check_range(curiosity, "curiosity");
    if (r.is_error()) return r;
    r = check_range(caution, "caution");
    if (r.is_error()) return r;
    r = check_range(aggression, "aggression");
    if (r.is_error()) return r;
    r = check_range(sociality, "sociality");
    if (r.is_error()) return r;
    r = check_range(cooperation, "cooperation");
    if (r.is_error()) return r;
    r = check_range(learning_rate_hint, "learning_rate_hint");
    if (r.is_error()) return r;

    return foundation::Result<void>::ok();
}

foundation::Result<void> AgentDefinition::validateBasic() const {
    if (definition_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "agent_definition_id_missing", "definition_id must not be empty"));
    }
    if (!foundation::isValidIdString(definition_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "agent_definition_id_invalid", "definition_id has invalid format: " + definition_id.value()));
    }
    if (display_name_key.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::command_missing_required_field, "display_name_key_missing", "display_name_key must not be empty"));
    }
    if (scale == AgentScale::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_scale_unknown", "scale must not be Unknown"));
    }
    if (cognition_band == AgentCognitionBand::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_cognition_band_unknown", "cognition_band must not be Unknown"));
    }
    if (embodiment == AgentEmbodiment::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_embodiment_unknown", "embodiment must not be Unknown"));
    }
    if (default_controller == AgentControllerType::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "agent_default_controller_unknown", "default_controller must not be Unknown"));
    }
    auto profile_result = profile.validateBasic();
    if (profile_result.is_error()) return profile_result;

    return foundation::Result<void>::ok();
}

} // namespace pathfinder::agent
