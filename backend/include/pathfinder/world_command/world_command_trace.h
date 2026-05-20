#pragma once

#include "pathfinder/world_command/world_command_types.h"
#include <vector>
#include <string>

namespace pathfinder::world_command {

class CommandTraceRecorder {
public:
    CommandTraceRecorder() = default;

    void recordCommandReceived(const WorldCommandDto& command);
    void recordStage(const std::string& stage_name);
    void recordNormalized(const WorldCommandDto& normalized);
    void recordValidated(bool passed, const std::vector<std::string>& reason_keys);
    void recordResult(const WorldCommandResultDto& result);
    void recordProjectionVersion(uint64_t version);

    std::vector<std::string> getTrace() const;
    void clear();

private:
    std::vector<std::string> trace_entries_;
};

} // namespace pathfinder::world_command
