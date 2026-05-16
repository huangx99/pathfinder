#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/random.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/pipeline/types.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::replay {

// Tag for CommandReplayRecord ID
struct ReplayRecordIdTag {};
using ReplayRecordId = pathfinder::foundation::StrongId<ReplayRecordIdTag>;

// Replay record status
enum class ReplayRecordStatus {
    Unknown,
    Recorded,
    Replayed,
    Mismatch
};

std::string toString(ReplayRecordStatus status);
pathfinder::foundation::Result<ReplayRecordStatus> replayRecordStatusFromString(const std::string& str);

// CommandReplayRecord - records one command execution for replay
struct CommandReplayRecord {
    ReplayRecordId record_id;
    pathfinder::foundation::CommandId command_id;
    pathfinder::command::CommandEnvelope command;
    pathfinder::foundation::StateVersion state_version_before;
    pathfinder::foundation::StateVersion state_version_after;
    pathfinder::foundation::RandomSeed random_seed;
    pathfinder::foundation::HashValue input_hash;
    pathfinder::foundation::HashValue output_hash;
    std::vector<pathfinder::foundation::StateChangeId> state_change_ids;
    std::vector<pathfinder::foundation::EventId> event_ids;
    pathfinder::pipeline::PipelineStatus pipeline_status;
    size_t error_count = 0;
    ReplayRecordStatus status = ReplayRecordStatus::Unknown;

    // Validate basic structure
    pathfinder::foundation::Result<void> validateBasic() const;
};

// CommandReplayLog - in-memory ordered log of command replay records
class CommandReplayLog {
public:
    // Append a record (rejects duplicate record_id)
    pathfinder::foundation::Result<void> append(CommandReplayRecord record);

    // Get number of records
    size_t size() const { return records_.size(); }

    // Get all records
    const std::vector<CommandReplayRecord>& records() const { return records_; }

    // Find a record by command_id
    std::optional<CommandReplayRecord> findByCommandId(
        const pathfinder::foundation::CommandId& command_id) const;

    // Validate all records
    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<CommandReplayRecord> records_;
};

} // namespace pathfinder::replay
