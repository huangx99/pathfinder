# Agent P9 Context Pack

## P9 Goal

P9 adds two lightweight contract layers on top of P8's recording and replay bridge:

1. **ReplayLock layer**: Judge whether a set of AgentTickRecord + CommandReplayLog is replay-locked (can safely replay without re-calling Policy).
2. **TrainingDraft layer**: Derive minimal training sample drafts from AgentTickRecord for future RL/imitation learning. P9 does NOT implement training algorithms.

## P9 Relationship to P8 and P4

- P8 produces `AgentTickRecord` / `AgentDecisionRecord` / `AgentTickLog` and bridges to `CommandReplayRecord` / `CommandReplayLog`.
- P4 provides `CommandReplayLog` / `ReplayRunner` for deterministic replay.
- P9 reads P8 records and P4 command logs to build `AgentReplayLockSet` and `AgentTrainingSampleDraft`.
- P9 does NOT modify P8 or P4 types.

## ReplayLocked Principles

- ReplayLock only checks P8 records and P4 CommandReplayLog.
- ReplayLock does NOT call AgentRuntime, Policy, ReplayRunner, or RulePipeline.
- Locked: has command_id and matching CommandReplayRecord -> can replay without Policy.
- ExplainedOnly: SubmitSkipped or NoDecision -> no command to replay but history is explainable.
- Broken: has command_id but CommandReplayLog missing or mismatched.
- Invalid: AgentTickRecord itself is invalid.

## TrainingSampleDraft Principles

- TrainingSampleDraft derives only from AgentTickRecord fields.
- Does NOT read GameState, ObjectDefinition, or hidden truth.
- Does NOT compute reward (reward_status = NotComputed).
- Does NOT judge done (done_status = NotComputed).
- Does NOT call AgentRuntime, Policy, RulePipeline, or ReplayRunner.
- observation_hash is passed in by request; P9 does not implement full Observation hash algorithm.
- NoDecision / SubmitSkipped samples must have phase_keys or reason_keys for explainability.

## P9 Does NOT Do

- No RL algorithm
- No Reward calculator
- No Done detector
- No Episode runner
- No batch dataset files
- No SaveManager
- No database
- No HTTP / WebSocket / H5
- No LLMPolicy / NeuralPolicy
- No multi-Agent scheduling
- No new rule resolvers

## File Scope

### Allowed New Files
- `backend/include/pathfinder/agent/replay_lock.h`
- `backend/include/pathfinder/agent/training_sample.h`
- `backend/src/agent/replay_lock.cpp`
- `backend/src/agent/training_sample.cpp`
- `backend/tests/unit/agent/agent_replay_lock_test.cpp`
- `backend/tests/unit/agent/agent_training_sample_test.cpp`
- `backend/tests/integration/p9/agent_replay_locked_training_draft_test.cpp`
- `backend/tests/integration/p9/agent_no_decision_training_draft_test.cpp`
- `context_packs/agent_p9.md`

### Allowed Modifications
- `backend/CMakeLists.txt`
- `backend/tests/CMakeLists.txt`
- `backend/dev_notes/agent.md`

### Prohibited Modifications
- `backend/src/rules/`
- `backend/include/pathfinder/rules/`
- `backend/src/object/`
- `backend/include/pathfinder/object/`
- `backend/src/pipeline/`
- `backend/include/pathfinder/pipeline/`
- `backend/src/replay/replay_runner.cpp`
- `frontend/`
- Any HTTP / WebSocket / SaveManager directory

## Verification Commands

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_replay_lock|agent_training_sample|p9" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```
