#pragma once

#include "pathfinder/agent/record_types.h"
#include "pathfinder/agent/replay_lock.h"
#include "pathfinder/agent/types.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/command/target.h"
#include "pathfinder/pipeline/types.h"
#include <vector>
#include <string>
#include <optional>

namespace pathfinder::agent {

// --- ID Tag Types ---

struct AgentTrainingSampleIdTag {};
using AgentTrainingSampleId = pathfinder::foundation::StrongId<AgentTrainingSampleIdTag>;

// --- Enums ---

enum class AgentTrainingSampleStatus {
    Unknown,
    Draft,
    ReplayLocked,
    Skipped,
    NoDecision,
    Invalid
};

std::string toString(AgentTrainingSampleStatus status);
pathfinder::foundation::Result<AgentTrainingSampleStatus> agentTrainingSampleStatusFromString(const std::string& str);

enum class AgentRewardDraftStatus {
    Unknown,
    NotComputed,
    PlaceholderOnly
};

std::string toString(AgentRewardDraftStatus status);
pathfinder::foundation::Result<AgentRewardDraftStatus> agentRewardDraftStatusFromString(const std::string& str);

enum class AgentDoneDraftStatus {
    Unknown,
    NotComputed,
    PlaceholderOnly
};

std::string toString(AgentDoneDraftStatus status);
pathfinder::foundation::Result<AgentDoneDraftStatus> agentDoneDraftStatusFromString(const std::string& str);

// --- Data Contracts ---

struct AgentObservationSampleDraft {
    AgentId agent_id;
    pathfinder::foundation::EntityId actor_id;
    pathfinder::foundation::Tick tick;
    pathfinder::foundation::StateVersion state_version;
    size_t observed_object_count = 0;
    size_t observed_threat_count = 0;
    size_t knowledge_claim_count = 0;
    pathfinder::foundation::HashValue observation_hash;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentActionSampleDraft {
    std::optional<pathfinder::foundation::DecisionId> decision_id;
    std::optional<pathfinder::foundation::ActionId> selected_candidate_id;
    std::optional<pathfinder::foundation::ActionId> selected_command_action_id;
    AgentIntentType intent_type = AgentIntentType::Unknown;
    std::vector<command::ActionTarget> selected_targets;
    std::optional<pathfinder::foundation::CommandId> command_id;
    AgentDecisionRecordStatus decision_status = AgentDecisionRecordStatus::Unknown;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentResultSampleDraft {
    AgentRuntimeStatus runtime_status = AgentRuntimeStatus::Unknown;
    std::optional<pipeline::PipelineStatus> pipeline_status;
    pathfinder::foundation::StateVersion state_version_before;
    pathfinder::foundation::StateVersion state_version_after;
    size_t state_change_count = 0;
    size_t event_count = 0;
    size_t error_count = 0;
    pathfinder::foundation::HashValue input_hash;
    pathfinder::foundation::HashValue output_hash;

    AgentRewardDraftStatus reward_status = AgentRewardDraftStatus::NotComputed;
    AgentDoneDraftStatus done_status = AgentDoneDraftStatus::NotComputed;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentTraceSampleDraft {
    std::vector<std::string> phase_keys;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;
    bool replay_locked = false;
    AgentReplayLockStatus replay_lock_status = AgentReplayLockStatus::Unknown;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentTrainingSampleDraft {
    AgentTrainingSampleId sample_id;
    AgentTickRecordId source_record_id;
    AgentTrainingSampleStatus status = AgentTrainingSampleStatus::Unknown;

    AgentObservationSampleDraft observation;
    AgentActionSampleDraft action;
    AgentResultSampleDraft result;
    AgentTraceSampleDraft trace;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// --- ID Helper ---

AgentTrainingSampleId makeAgentTrainingSampleId(
    const AgentId& agent_id,
    pathfinder::foundation::Tick tick,
    pathfinder::foundation::StateVersion state_version_before);

// --- Builder ---

struct AgentTrainingSampleBuildRequest {
    AgentTickRecord record;
    std::optional<AgentReplayLockEntry> replay_lock_entry;
    pathfinder::foundation::HashValue observation_hash;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class AgentTrainingSampleDraftBuilder {
public:
    pathfinder::foundation::Result<AgentTrainingSampleDraft> build(
        const AgentTrainingSampleBuildRequest& request) const;
};

} // namespace pathfinder::agent
