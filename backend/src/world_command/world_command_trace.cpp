#include "pathfinder/world_command/world_command_trace.h"

namespace pathfinder::world_command {

void CommandTraceRecorder::recordCommandReceived(const WorldCommandDto& command) {
    trace_entries_.push_back("receive: kind=" + toString(command.command_kind)
        + " key=" + command.command_key
        + " source=" + toString(command.source));
}

void CommandTraceRecorder::recordStage(const std::string& stage_name) {
    trace_entries_.push_back("stage: " + stage_name);
}

void CommandTraceRecorder::recordNormalized(const WorldCommandDto& normalized) {
    trace_entries_.push_back("normalize: kind=" + toString(normalized.command_kind)
        + " key=" + normalized.command_key);
}

void CommandTraceRecorder::recordValidated(bool passed, const std::vector<std::string>& reason_keys) {
    if (passed) {
        trace_entries_.push_back("validate: passed");
    } else {
        std::string entry = "validate: failed";
        for (const auto& reason : reason_keys) {
            entry += " " + reason;
        }
        trace_entries_.push_back(entry);
    }
}

void CommandTraceRecorder::recordResult(const WorldCommandResultDto& result) {
    trace_entries_.push_back("result: " + toString(result.result_kind));
}

void CommandTraceRecorder::recordProjectionVersion(uint64_t version) {
    trace_entries_.push_back("projection_version: " + std::to_string(version));
}

std::vector<std::string> CommandTraceRecorder::getTrace() const {
    return trace_entries_;
}

void CommandTraceRecorder::clear() {
    trace_entries_.clear();
}

} // namespace pathfinder::world_command
