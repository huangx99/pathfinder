# Agent P8 Context Pack: Agent Runtime Record & Replay Bridge

## Goal

P8 adds the recording layer and replay bridge layer on top of P7's AgentRuntime:

```text
P7 Runtime: execute one Agent tick -> AgentTickResult
P8 Record:  build AgentTickRecord / AgentDecisionRecord from request + result
P8 Bridge:  convert AgentTickRecord -> CommandReplayRecord for P4 ReplayRunner
P8 Check:   verify replay can proceed without calling Policy
```

## P8 Core Principles

```text
History must be explainable.
Historical commands must be replayable.
Replay must not re-invoke Policy.
Training samples can be derived from records, but P8 does NOT implement training.
```

## Allowed Implementation

```text
AgentTickRecordId / AgentDecisionRecordId (StrongId tags)
AgentDecisionRecordStatus / AgentTickRecordStatus (enums)
AgentDecisionRecord / AgentTickRecord (data contracts)
AgentTickLog (in-memory log)
AgentRecordBuildRequest / AgentRecordBuilder (request+result -> record)
AgentReplayBridge (AgentTickRecord -> CommandReplayRecord)
AgentReplayLockChecker / AgentReplayLockCheckResult (minimal lock check)
```

## Forbidden Implementation

```text
SaveManager, file format, database
HTTP / WebSocket / H5
Complete AgentService / AgentStore / AgentGoal
Replay UI / ReplayService
RL Policy / NeuralPolicy / LLMPolicy / Reward / Training environment
Multi-Agent scheduling
New rule resolvers (FleeResolver, GroupCombat, WarResolver, tribe_split)
```

## Forbidden Actions

```text
Replay must NOT call AgentRuntime / Policy
AgentReplayBridge must NOT call RulePipeline / ReplayRunner
AgentRecordBuilder must NOT call AgentRuntime / Policy / RulePipeline
AgentRecordBuilder must NOT modify GameState
AgentRecordBuilder must NOT read edible_profile / hidden truth
SubmitSkipped / NoDecision must NOT generate CommandReplayRecord
```

## File Scope

### Allowed New Files

```text
backend/include/pathfinder/agent/record_types.h
backend/include/pathfinder/agent/record_builder.h
backend/include/pathfinder/agent/replay_bridge.h
backend/src/agent/record_types.cpp
backend/src/agent/record_builder.cpp
backend/src/agent/replay_bridge.cpp
backend/tests/unit/agent/agent_record_types_test.cpp
backend/tests/unit/agent/agent_record_builder_test.cpp
backend/tests/unit/agent/agent_replay_bridge_test.cpp
backend/tests/integration/p8/agent_runtime_record_unknown_fruit_replay_test.cpp
backend/tests/integration/p8/agent_runtime_record_no_submit_test.cpp
context_packs/agent_p8.md
```

### Allowed Modifications

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

### Forbidden Modifications

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/rule_pipeline.cpp
backend/src/replay/replay_runner.cpp (unless P4/P7 blocking bug found)
frontend/
```

## P8 Relationship to P4/P7

```text
P7 AgentRuntime produces AgentTickResult (one tick execution).
P8 AgentRecordBuilder converts request+result into AgentTickRecord (memory record).
P8 AgentReplayBridge converts AgentTickRecord into CommandReplayRecord (P4 format).
P4 ReplayRunner replays CommandReplayRecord through RulePipeline.
P8 ReplayLockChecker verifies the record chain is complete for replay.
```

## Replay Locked Principle

```text
If AgentTickRecord has command_id, CommandReplayLog must find matching record.
If found -> can_replay_without_policy = true.
If not found -> cannot claim replay locked.
NoDecision / SubmitSkipped have no command_id, but agent record explains why.
```

## Training Sample Prelude

P8 records contain fields that can later derive Observation/Action/Result/Trace:
- AgentDecisionRecord: selected_candidate_id, selected_command_action_id, intent_type, targets, confidence
- AgentTickRecord: observation, action_space, decision, command, pipeline_result, state_change_ids, event_ids

P8 does NOT implement: Reward, Done, batch dataset, training environment, RL algorithm.

## Verification Commands

```bash
# Build
cmake -S backend -B build/backend
cmake --build build/backend

# P8 targeted tests
ctest --test-dir build/backend -R "agent_record|agent_replay_bridge|p8" --output-on-failure

# P7 regression
ctest --test-dir build/backend -R "agent_policy|agent_runtime_types|agent_runtime|p7" --output-on-failure

# Full regression
ctest --test-dir build/backend --output-on-failure

# Boundary: record_builder/replay_bridge no runtime/pipeline calls
rg -n "AgentRuntime\(|FirstSupportedPolicy|RulePipeline|execute\(|edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent/record_builder.h \
  backend/src/agent/record_builder.cpp \
  backend/include/pathfinder/agent/replay_bridge.h \
  backend/src/agent/replay_bridge.cpp && exit 1 || true

# Boundary: no forbidden systems
rg -n "SaveManager|WebSocket|HTTP|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy|BehaviorTree|GOAP|FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/integration/p8 && exit 1 || true
```
