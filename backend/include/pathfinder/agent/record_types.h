#pragma once

#include "pathfinder/agent/types.h"
#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/random.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/command/target.h"
#include "pathfinder/pipeline/types.h"
#include "pathfinder/replay/command_replay.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::agent {

// --- ID Tag Types ---

struct AgentTickRecordIdTag {};
struct AgentDecisionRecordIdTag {};

using AgentTickRecordId = pathfinder::foundation::StrongId<AgentTickRecordIdTag>;
using AgentDecisionRecordId = pathfinder::foundation::StrongId<AgentDecisionRecordIdTag>;

// --- ID Helpers ---

// Generate stable AgentTickRecordId: agent_tick_<agent_id>_<tick>_<state_version>
AgentTickRecordId makeAgentTickRecordId(
    const AgentId& agent_id,
    pathfinder::foundation::Tick tick,
    pathfinder::foundation::StateVersion state_version);

// Generate stable AgentDecisionRecordId: agent_decision_record_<decision_id>
AgentDecisionRecordId makeAgentDecisionRecordId(
    const pathfinder::foundation::DecisionId& decision_id);

// --- Enums ---

enum class AgentDecisionRecordStatus {
    Unknown,
    Selected,
    NoDecision,
    AdapterRejected,
    SubmitSkipped,
    PipelineSucceeded,
    PipelineFailed,
    Failed
};

std::string toString(AgentDecisionRecordStatus status);
pathfinder::foundation::Result<AgentDecisionRecordStatus> agentDecisionRecordStatusFromString(const std::string& str);

enum class AgentTickRecordStatus {
    Unknown,
    Recorded,
    ReplayLinked,
    ReplayMatched,
    ReplayMismatched,
    Invalid
};

std::string toString(AgentTickRecordStatus status);
pathfinder::foundation::Result<AgentTickRecordStatus> agentTickRecordStatusFromString(const std::string& str);

// --- Data Contracts ---

struct AgentDecisionRecord {
    AgentDecisionRecordId record_id;
    pathfinder::foundation::DecisionId decision_id;
    AgentId agent_id;
    pathfinder::foundation::EntityId actor_id;
    pathfinder::foundation::Tick decision_tick;
    pathfinder::foundation::StateVersion observation_state_version;

    pathfinder::foundation::ActionId selected_candidate_id;
    pathfinder::foundation::ActionId selected_command_action_id;
    AgentIntentType selected_intent_type = AgentIntentType::Unknown;
    pathfinder::foundation::ActionId selected_intent_action_id;
    std::vector<command::ActionTarget> selected_targets;

    AgentDecisionRecordStatus status = AgentDecisionRecordStatus::Unknown;
    std::string reason_key;
    double confidence = 0.0;
    std::vector<std::string> phase_keys;
    std::vector<std::string> rejected_reason_keys;

    std::optional<pathfinder::foundation::CommandId> command_id;
    std::optional<pathfinder::replay::ReplayRecordId> replay_record_id;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentTickRecord {
    AgentTickRecordId record_id;
    AgentId agent_id;
    pathfinder::foundation::EntityId actor_id;
    pathfinder::foundation::Tick tick;
    pathfinder::foundation::StateVersion state_version_before;
    pathfinder::foundation::StateVersion state_version_after;
    pathfinder::foundation::RandomSeed random_seed;

    AgentRuntimeStatus runtime_status = AgentRuntimeStatus::Unknown;
    AgentTickRecordStatus record_status = AgentTickRecordStatus::Unknown;

    AgentDecisionRecord decision_record;
    std::optional<command::CommandEnvelope> command;
    std::optional<pathfinder::pipeline::PipelineStatus> pipeline_status;
    std::vector<pathfinder::foundation::StateChangeId> state_change_ids;
    std::vector<pathfinder::foundation::EventId> event_ids;
    size_t error_count = 0;

    pathfinder::foundation::HashValue input_hash;
    pathfinder::foundation::HashValue output_hash;

    std::vector<std::string> phase_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- AgentTickLog ---

class AgentTickLog {
public:
    pathfinder::foundation::Result<void> append(AgentTickRecord record);
    size_t size() const { return records_.size(); }
    const std::vector<AgentTickRecord>& records() const { return records_; }
    std::optional<AgentTickRecord> findByRecordId(AgentTickRecordId id) const;
    std::vector<AgentTickRecord> findByAgentId(AgentId agent_id) const;
    std::optional<AgentTickRecord> findByDecisionId(pathfinder::foundation::DecisionId decision_id) const;
    pathfinder::foundation::Result<void> validateBasic() const;

private:
    std::vector<AgentTickRecord> records_;
};

// --- AgentRecordBuildRequest ---

struct AgentRecordBuildRequest {
    AgentTickRequest tick_request;
    AgentTickResult tick_result;
    pathfinder::foundation::HashValue input_hash;
    pathfinder::foundation::HashValue output_hash;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
