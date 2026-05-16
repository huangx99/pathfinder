#include "pathfinder/foundation/error.h"
#include <stdexcept>

namespace pathfinder::foundation {

std::string toString(ErrorCode code) {
    switch (code) {
        // Common
        case ErrorCode::common_unknown: return "common_unknown";
        case ErrorCode::common_not_implemented: return "common_not_implemented";
        case ErrorCode::common_unsupported: return "common_unsupported";
        case ErrorCode::common_internal_invariant_broken: return "common_internal_invariant_broken";

        // Command
        case ErrorCode::command_invalid_format: return "command_invalid_format";
        case ErrorCode::command_unknown_type: return "command_unknown_type";
        case ErrorCode::command_missing_required_field: return "command_missing_required_field";
        case ErrorCode::command_invalid_argument: return "command_invalid_argument";
        case ErrorCode::command_not_allowed_now: return "command_not_allowed_now";
        case ErrorCode::command_duplicate: return "command_duplicate";

        // Id
        case ErrorCode::id_missing: return "id_missing";
        case ErrorCode::id_invalid_format: return "id_invalid_format";
        case ErrorCode::id_not_found: return "id_not_found";
        case ErrorCode::id_type_mismatch: return "id_type_mismatch";
        case ErrorCode::id_definition_not_found: return "id_definition_not_found";

        // Validation
        case ErrorCode::validation_failed: return "validation_failed";
        case ErrorCode::validation_enum_unknown: return "validation_enum_unknown";
        case ErrorCode::validation_value_out_of_range: return "validation_value_out_of_range";
        case ErrorCode::validation_config_invalid: return "validation_config_invalid";
        case ErrorCode::validation_save_invalid: return "validation_save_invalid";

        // Preconditions
        case ErrorCode::precondition_target_too_far: return "precondition_target_too_far";
        case ErrorCode::precondition_target_unavailable: return "precondition_target_unavailable";
        case ErrorCode::precondition_actor_unavailable: return "precondition_actor_unavailable";
        case ErrorCode::precondition_insufficient_cost: return "precondition_insufficient_cost";
        case ErrorCode::precondition_missing_capability: return "precondition_missing_capability";
        case ErrorCode::precondition_missing_tool: return "precondition_missing_tool";
        case ErrorCode::precondition_environment_mismatch: return "precondition_environment_mismatch";
        case ErrorCode::precondition_risk_too_high: return "precondition_risk_too_high";

        // Rule
        case ErrorCode::rule_invariant_failed: return "rule_invariant_failed";
        case ErrorCode::rule_invalid_state_transition: return "rule_invalid_state_transition";
        case ErrorCode::rule_missing_resolver: return "rule_missing_resolver";
        case ErrorCode::rule_conflict: return "rule_conflict";
        case ErrorCode::rule_random_stream_missing: return "rule_random_stream_missing";
        case ErrorCode::rule_output_invalid: return "rule_output_invalid";

        // Knowledge
        case ErrorCode::knowledge_claim_incomplete: return "knowledge_claim_incomplete";
        case ErrorCode::knowledge_claim_conflict: return "knowledge_claim_conflict";
        case ErrorCode::knowledge_not_found: return "knowledge_not_found";
        case ErrorCode::knowledge_not_teachable: return "knowledge_not_teachable";
        case ErrorCode::knowledge_insufficient_confidence: return "knowledge_insufficient_confidence";
        case ErrorCode::memory_not_found: return "memory_not_found";

        // Propagation
        case ErrorCode::propagation_missing_receiver: return "propagation_missing_receiver";
        case ErrorCode::propagation_receiver_unavailable: return "propagation_receiver_unavailable";
        case ErrorCode::propagation_channel_unavailable: return "propagation_channel_unavailable";
        case ErrorCode::propagation_no_shared_context: return "propagation_no_shared_context";
        case ErrorCode::propagation_contradictory_knowledge: return "propagation_contradictory_knowledge";
        case ErrorCode::propagation_harmful_spread_detected: return "propagation_harmful_spread_detected";

        // State
        case ErrorCode::state_version_conflict: return "state_version_conflict";
        case ErrorCode::state_apply_failed: return "state_apply_failed";
        case ErrorCode::state_change_invalid: return "state_change_invalid";
        case ErrorCode::state_snapshot_missing: return "state_snapshot_missing";
        case ErrorCode::state_corrupted: return "state_corrupted";

        // Storage
        case ErrorCode::storage_save_failed: return "storage_save_failed";
        case ErrorCode::storage_load_failed: return "storage_load_failed";
        case ErrorCode::storage_write_failed: return "storage_write_failed";
        case ErrorCode::storage_read_failed: return "storage_read_failed";
        case ErrorCode::storage_checksum_mismatch: return "storage_checksum_mismatch";
    }
    return "unknown_error_code";
}

std::string toString(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Warning: return "Warning";
        case ErrorSeverity::UserError: return "UserError";
        case ErrorSeverity::Recoverable: return "Recoverable";
        case ErrorSeverity::DataError: return "DataError";
        case ErrorSeverity::RuleError: return "RuleError";
        case ErrorSeverity::Critical: return "Critical";
        case ErrorSeverity::Fatal: return "Fatal";
    }
    return "unknown_severity";
}

std::string toString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::Common: return "Common";
        case ErrorCategory::Command: return "Command";
        case ErrorCategory::Id: return "Id";
        case ErrorCategory::Validation: return "Validation";
        case ErrorCategory::Preconditions: return "Preconditions";
        case ErrorCategory::Rule: return "Rule";
        case ErrorCategory::Knowledge: return "Knowledge";
        case ErrorCategory::Propagation: return "Propagation";
        case ErrorCategory::State: return "State";
        case ErrorCategory::Storage: return "Storage";
    }
    return "unknown_category";
}

std::string toString(const ErrorDetail& detail) {
    std::string result = "ErrorDetail{";
    result += "code=" + toString(detail.code);
    result += ", severity=" + toString(detail.severity);
    result += ", category=" + toString(detail.category);
    result += ", message_key=" + detail.message_key;
    if (detail.debug_message) {
        result += ", debug_message=" + *detail.debug_message;
    }
    result += ", recoverable=" + std::string(detail.recoverable ? "true" : "false");
    result += ", state_mutated=" + std::string(detail.state_mutated ? "true" : "false");
    result += "}";
    return result;
}

ErrorCategory getCategory(ErrorCode code) {
    switch (code) {
        // Common
        case ErrorCode::common_unknown:
        case ErrorCode::common_not_implemented:
        case ErrorCode::common_unsupported:
        case ErrorCode::common_internal_invariant_broken:
            return ErrorCategory::Common;

        // Command
        case ErrorCode::command_invalid_format:
        case ErrorCode::command_unknown_type:
        case ErrorCode::command_missing_required_field:
        case ErrorCode::command_invalid_argument:
        case ErrorCode::command_not_allowed_now:
        case ErrorCode::command_duplicate:
            return ErrorCategory::Command;

        // Id
        case ErrorCode::id_missing:
        case ErrorCode::id_invalid_format:
        case ErrorCode::id_not_found:
        case ErrorCode::id_type_mismatch:
        case ErrorCode::id_definition_not_found:
            return ErrorCategory::Id;

        // Validation
        case ErrorCode::validation_failed:
        case ErrorCode::validation_enum_unknown:
        case ErrorCode::validation_value_out_of_range:
        case ErrorCode::validation_config_invalid:
        case ErrorCode::validation_save_invalid:
            return ErrorCategory::Validation;

        // Preconditions
        case ErrorCode::precondition_target_too_far:
        case ErrorCode::precondition_target_unavailable:
        case ErrorCode::precondition_actor_unavailable:
        case ErrorCode::precondition_insufficient_cost:
        case ErrorCode::precondition_missing_capability:
        case ErrorCode::precondition_missing_tool:
        case ErrorCode::precondition_environment_mismatch:
        case ErrorCode::precondition_risk_too_high:
            return ErrorCategory::Preconditions;

        // Rule
        case ErrorCode::rule_invariant_failed:
        case ErrorCode::rule_invalid_state_transition:
        case ErrorCode::rule_missing_resolver:
        case ErrorCode::rule_conflict:
        case ErrorCode::rule_random_stream_missing:
        case ErrorCode::rule_output_invalid:
            return ErrorCategory::Rule;

        // Knowledge
        case ErrorCode::knowledge_claim_incomplete:
        case ErrorCode::knowledge_claim_conflict:
        case ErrorCode::knowledge_not_found:
        case ErrorCode::knowledge_not_teachable:
        case ErrorCode::knowledge_insufficient_confidence:
        case ErrorCode::memory_not_found:
            return ErrorCategory::Knowledge;

        // Propagation
        case ErrorCode::propagation_missing_receiver:
        case ErrorCode::propagation_receiver_unavailable:
        case ErrorCode::propagation_channel_unavailable:
        case ErrorCode::propagation_no_shared_context:
        case ErrorCode::propagation_contradictory_knowledge:
        case ErrorCode::propagation_harmful_spread_detected:
            return ErrorCategory::Propagation;

        // State
        case ErrorCode::state_version_conflict:
        case ErrorCode::state_apply_failed:
        case ErrorCode::state_change_invalid:
        case ErrorCode::state_snapshot_missing:
        case ErrorCode::state_corrupted:
            return ErrorCategory::State;

        // Storage
        case ErrorCode::storage_save_failed:
        case ErrorCode::storage_load_failed:
        case ErrorCode::storage_write_failed:
        case ErrorCode::storage_read_failed:
        case ErrorCode::storage_checksum_mismatch:
            return ErrorCategory::Storage;
    }
    return ErrorCategory::Common;
}

ErrorSeverity getDefaultSeverity(ErrorCode code) {
    switch (code) {
        // Common
        case ErrorCode::common_unknown: return ErrorSeverity::Critical;
        case ErrorCode::common_not_implemented: return ErrorSeverity::RuleError;
        case ErrorCode::common_unsupported: return ErrorSeverity::Recoverable;
        case ErrorCode::common_internal_invariant_broken: return ErrorSeverity::Fatal;

        // Command
        case ErrorCode::command_invalid_format:
        case ErrorCode::command_unknown_type:
        case ErrorCode::command_missing_required_field:
        case ErrorCode::command_invalid_argument:
            return ErrorSeverity::UserError;
        case ErrorCode::command_not_allowed_now: return ErrorSeverity::Recoverable;
        case ErrorCode::command_duplicate: return ErrorSeverity::Warning;

        // Id
        case ErrorCode::id_missing:
        case ErrorCode::id_invalid_format:
        case ErrorCode::id_not_found:
        case ErrorCode::id_type_mismatch:
            return ErrorSeverity::UserError;
        case ErrorCode::id_definition_not_found: return ErrorSeverity::DataError;

        // Validation
        case ErrorCode::validation_failed: return ErrorSeverity::Recoverable;
        case ErrorCode::validation_enum_unknown:
        case ErrorCode::validation_value_out_of_range:
            return ErrorSeverity::UserError;
        case ErrorCode::validation_config_invalid:
        case ErrorCode::validation_save_invalid:
            return ErrorSeverity::DataError;

        // Preconditions
        case ErrorCode::precondition_target_too_far:
        case ErrorCode::precondition_insufficient_cost:
        case ErrorCode::precondition_missing_capability:
        case ErrorCode::precondition_missing_tool:
            return ErrorSeverity::UserError;
        case ErrorCode::precondition_target_unavailable:
        case ErrorCode::precondition_actor_unavailable:
        case ErrorCode::precondition_environment_mismatch:
            return ErrorSeverity::Recoverable;
        case ErrorCode::precondition_risk_too_high: return ErrorSeverity::Warning;

        // Rule
        case ErrorCode::rule_invariant_failed:
        case ErrorCode::rule_invalid_state_transition:
        case ErrorCode::rule_missing_resolver:
        case ErrorCode::rule_random_stream_missing:
        case ErrorCode::rule_output_invalid:
            return ErrorSeverity::RuleError;
        case ErrorCode::rule_conflict: return ErrorSeverity::RuleError;

        // Knowledge
        case ErrorCode::knowledge_claim_incomplete:
            return ErrorSeverity::RuleError;
        case ErrorCode::knowledge_claim_conflict:
        case ErrorCode::knowledge_insufficient_confidence:
            return ErrorSeverity::Warning;
        case ErrorCode::knowledge_not_found:
        case ErrorCode::knowledge_not_teachable:
        case ErrorCode::memory_not_found:
            return ErrorSeverity::UserError;

        // Propagation
        case ErrorCode::propagation_missing_receiver:
        case ErrorCode::propagation_channel_unavailable:
            return ErrorSeverity::UserError;
        case ErrorCode::propagation_receiver_unavailable:
            return ErrorSeverity::Recoverable;
        case ErrorCode::propagation_no_shared_context:
        case ErrorCode::propagation_contradictory_knowledge:
            return ErrorSeverity::Warning;
        case ErrorCode::propagation_harmful_spread_detected:
            return ErrorSeverity::Warning;

        // State
        case ErrorCode::state_version_conflict:
        case ErrorCode::state_apply_failed:
            return ErrorSeverity::Critical;
        case ErrorCode::state_change_invalid:
            return ErrorSeverity::RuleError;
        case ErrorCode::state_snapshot_missing:
            return ErrorSeverity::Critical;
        case ErrorCode::state_corrupted:
            return ErrorSeverity::Fatal;

        // Storage
        case ErrorCode::storage_save_failed:
        case ErrorCode::storage_load_failed:
        case ErrorCode::storage_write_failed:
        case ErrorCode::storage_read_failed:
            return ErrorSeverity::Critical;
        case ErrorCode::storage_checksum_mismatch:
            return ErrorSeverity::Fatal;
    }
    return ErrorSeverity::Critical;
}

ErrorDetail makeError(ErrorCode code, std::string message_key, std::optional<std::string> debug_message) {
    return ErrorDetail{
        .code = code,
        .severity = getDefaultSeverity(code),
        .category = getCategory(code),
        .message_key = std::move(message_key),
        .debug_message = std::move(debug_message),
        .related_ids = {},
        .field_path = std::nullopt,
        .expected = std::nullopt,
        .actual = std::nullopt,
        .recoverable = (getDefaultSeverity(code) <= ErrorSeverity::Recoverable),
        .state_mutated = false
    };
}

} // namespace pathfinder::foundation
