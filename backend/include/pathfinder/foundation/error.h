#pragma once

#include <string>
#include <vector>
#include <optional>

namespace pathfinder::foundation {

// Error severity levels
enum class ErrorSeverity {
    Warning,      // 可以继续，但需要关注
    UserError,    // 用户输入错误
    Recoverable,  // 可恢复错误
    DataError,    // 数据错误
    RuleError,    // 规则错误
    Critical,     // 严重错误
    Fatal         // 致命错误，状态可能不可信
};

// Error categories
enum class ErrorCategory {
    Common,
    Command,
    Id,
    Validation,
    Preconditions,
    Rule,
    Knowledge,
    Propagation,
    State,
    Storage
};

// Error codes
enum class ErrorCode {
    // Common
    common_unknown,
    common_not_implemented,
    common_unsupported,
    common_internal_invariant_broken,

    // Command
    command_invalid_format,
    command_unknown_type,
    command_missing_required_field,
    command_invalid_argument,
    command_not_allowed_now,
    command_duplicate,

    // Id
    id_missing,
    id_invalid_format,
    id_not_found,
    id_type_mismatch,
    id_definition_not_found,

    // Validation
    validation_failed,
    validation_enum_unknown,
    validation_value_out_of_range,
    validation_config_invalid,
    validation_save_invalid,

    // Preconditions
    precondition_target_too_far,
    precondition_target_unavailable,
    precondition_actor_unavailable,
    precondition_insufficient_cost,
    precondition_missing_capability,
    precondition_missing_tool,
    precondition_environment_mismatch,
    precondition_risk_too_high,

    // Rule
    rule_invariant_failed,
    rule_invalid_state_transition,
    rule_missing_resolver,
    rule_conflict,
    rule_random_stream_missing,
    rule_output_invalid,

    // Knowledge
    knowledge_claim_incomplete,
    knowledge_claim_conflict,
    knowledge_not_found,
    knowledge_not_teachable,
    knowledge_insufficient_confidence,
    memory_not_found,

    // Propagation
    propagation_missing_receiver,
    propagation_receiver_unavailable,
    propagation_channel_unavailable,
    propagation_no_shared_context,
    propagation_contradictory_knowledge,
    propagation_harmful_spread_detected,

    // State
    state_version_conflict,
    state_apply_failed,
    state_change_invalid,
    state_snapshot_missing,
    state_corrupted,

    // Storage
    storage_save_failed,
    storage_load_failed,
    storage_write_failed,
    storage_read_failed,
    storage_checksum_mismatch
};

// Error detail
struct ErrorDetail {
    ErrorCode code;
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message_key;
    std::optional<std::string> debug_message;
    std::vector<std::string> related_ids;
    std::optional<std::string> field_path;
    std::optional<std::string> expected;
    std::optional<std::string> actual;
    bool recoverable;
    bool state_mutated;
};

// Convert functions
std::string toString(ErrorCode code);
std::string toString(ErrorSeverity severity);
std::string toString(ErrorCategory category);
std::string toString(const ErrorDetail& detail);

// Helper to get category from code
ErrorCategory getCategory(ErrorCode code);

// Helper to get default severity from code
ErrorSeverity getDefaultSeverity(ErrorCode code);

// Helper to create ErrorDetail with defaults
ErrorDetail makeError(ErrorCode code, std::string message_key, std::optional<std::string> debug_message = std::nullopt);

} // namespace pathfinder::foundation
