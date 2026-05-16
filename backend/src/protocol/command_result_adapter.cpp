#include "pathfinder/protocol/command_result_adapter.h"

namespace pathfinder::protocol {

ProtocolEnvelope buildCommandResponseEnvelope(
    const pathfinder::command::CommandEnvelope& command,
    const pathfinder::pipeline::PipelineResult& result,
    const CommandResponseOptions& options) {

    ProtocolEnvelope env;
    env.protocol_version = options.protocol_version;
    env.message_id = "resp_" + command.command_id.value();
    env.message_type = ProtocolMessageType::Response;
    env.domain = ProtocolDomain::Command;
    env.request_id = options.request_id;
    env.correlation_id = options.correlation_id;
    env.session_id = options.session_id;

    // Set payload
    env.payload.payload_type = ProtocolPayloadType::CommandResult;
    if (result.status == pathfinder::pipeline::PipelineStatus::Succeeded) {
        env.payload.message_key = "command.succeeded";
    } else {
        env.payload.message_key = "command.failed";
    }

    // Set state_version from the last event (post-change version)
    if (!result.events.empty()) {
        const auto& last_event = result.events.events().back();
        env.state_version = last_event.state_version;
    }

    // Build summary in debug_text
    CommandResultProtocolSummary summary;
    summary.command_id = command.command_id;
    summary.pipeline_status = result.status;
    summary.state_change_count = result.state_changes.size();
    summary.event_count = result.events.size();
    summary.error_count = result.errors.size();
    env.payload.debug_text = "changes=" + std::to_string(summary.state_change_count) +
                             " events=" + std::to_string(summary.event_count) +
                             " errors=" + std::to_string(summary.error_count);

    // Copy errors as ProtocolError summaries (no internal stack)
    for (const auto& err : result.errors) {
        ProtocolError perr;
        perr.code = pathfinder::foundation::toString(err.code);
        perr.message_key = err.message_key;
        if (err.debug_message) {
            perr.debug_text = *err.debug_message;
        }
        env.errors.push_back(perr);
    }

    return env;
}

} // namespace pathfinder::protocol
