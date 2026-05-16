#pragma once

#include "pathfinder/agent/binding.h"
#include "pathfinder/agent/builder_types.h"
#include "pathfinder/agent/observation.h"
#include "pathfinder/agent/action_space.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/random.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/pipeline/result.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::state {
class GameState;
}

namespace pathfinder::agent {

enum class AgentRuntimeStatus {
    Unknown,
    ObservationBuilt,
    ActionSpaceBuilt,
    DecisionMade,
    CommandCreated,
    SubmitSkipped,
    PipelineSucceeded,
    PipelineFailed,
    NoDecision,
    Failed
};

std::string toString(AgentRuntimeStatus status);
foundation::Result<AgentRuntimeStatus> agentRuntimeStatusFromString(const std::string& str);

struct AgentRuntimeOptions {
    bool include_cognition_claims = true;
    bool include_explore_candidates = true;
    bool include_wait_candidate = false;
    bool build_command = true;
    bool submit_to_pipeline = true;
    std::vector<foundation::ActionId> submit_action_allowlist;
};

struct AgentRuntimeTrace {
    std::vector<std::string> phase_keys;
    std::vector<std::string> reason_keys;
    int selected_candidate_index = -1;
    foundation::ActionId selected_candidate_id;
    foundation::ActionId selected_command_action_id;
    bool command_created = false;
    bool pipeline_submitted = false;
};

struct AgentTickRequest {
    AgentBinding binding;
    state::GameState* state = nullptr;
    VisibilityInput visibility;
    foundation::Tick issued_tick;
    foundation::RandomSeed random_seed;
    command::CommandSource command_source = command::CommandSource::Ai;
    AgentRuntimeOptions options;

    foundation::Result<void> validateBasic() const;
};

struct AgentTickResult {
    AgentRuntimeStatus status = AgentRuntimeStatus::Unknown;
    AgentObservation observation;
    AgentActionSpace action_space;
    AgentDecision decision;
    std::optional<command::CommandEnvelope> command;
    std::optional<pipeline::PipelineResult> pipeline_result;
    AgentRuntimeTrace trace;
    std::vector<foundation::ErrorDetail> warnings;

    foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
