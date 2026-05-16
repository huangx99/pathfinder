#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/protocol/envelope.h"
#include "pathfinder/pipeline/result.h"
#include "pathfinder/command/envelope.h"
#include <string>

namespace pathfinder::protocol {

// Command result protocol summary
struct CommandResultProtocolSummary {
    pathfinder::foundation::CommandId command_id;
    pathfinder::pipeline::PipelineStatus pipeline_status = pathfinder::pipeline::PipelineStatus::NotStarted;
    size_t state_change_count = 0;
    size_t event_count = 0;
    size_t error_count = 0;
    pathfinder::foundation::StateVersion state_version_after;
};

// Options for building command response envelope
struct CommandResponseOptions {
    std::string protocol_version = "1.0";
    std::optional<std::string> request_id;
    std::optional<std::string> correlation_id;
    std::optional<std::string> session_id;
};

// Build a ProtocolEnvelope from a PipelineResult
ProtocolEnvelope buildCommandResponseEnvelope(
    const pathfinder::command::CommandEnvelope& command,
    const pathfinder::pipeline::PipelineResult& result,
    const CommandResponseOptions& options = {});

} // namespace pathfinder::protocol
