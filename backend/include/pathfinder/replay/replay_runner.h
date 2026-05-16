#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/replay/command_replay.h"
#include "pathfinder/replay/random_replay.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include <vector>
#include <string>

namespace pathfinder::replay {

// Replay compare status
enum class ReplayCompareStatus {
    Unknown,
    Match,
    Mismatch
};

std::string toString(ReplayCompareStatus status);

// Replay compare report
struct ReplayCompareReport {
    ReplayCompareStatus status = ReplayCompareStatus::Unknown;
    size_t expected_state_change_count = 0;
    size_t actual_state_change_count = 0;
    size_t expected_event_count = 0;
    size_t actual_event_count = 0;
    pathfinder::pipeline::PipelineStatus expected_pipeline_status = pathfinder::pipeline::PipelineStatus::NotStarted;
    pathfinder::pipeline::PipelineStatus actual_pipeline_status = pathfinder::pipeline::PipelineStatus::NotStarted;
    pathfinder::foundation::StateVersion expected_state_version_after;
    pathfinder::foundation::StateVersion actual_state_version_after;
    std::vector<std::string> differences;
};

// Replay input
struct ReplayInput {
    pathfinder::state::GameState initial_state;
    CommandReplayLog command_log;
    RandomReplayLog random_log;
};

// Replay result
struct ReplayResult {
    ReplayCompareReport report;
    pathfinder::pipeline::PipelineResult pipeline_result;
    pathfinder::state::GameState replay_state;
};

// ReplayRunner - replays commands through RulePipeline
class ReplayRunner {
public:
    // Replay a single command from the log
    // Returns error if log is empty or has more than 1 record
    pathfinder::foundation::Result<ReplayResult> replayOne(const ReplayInput& input);

private:
    pathfinder::pipeline::RulePipeline pipeline_;
};

} // namespace pathfinder::replay
