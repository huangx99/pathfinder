# Agent P10 Context Pack: Agent History Query & Protocol Projection

## P10 Goal

P10 adds read-only query and safe projection layers on top of P8/P9 recorded data:

```text
P8 Record: AgentTickRecord / AgentDecisionRecord / AgentTickLog
P9 Lock/Draft: AgentReplayLockSet / AgentTrainingSampleDraft
P10 Query: AgentHistoryQueryService - query history by agent_id / tick / status
P10 Project: AgentHistoryProjector - project records into safe DTOs
P10 Protocol: AgentProtocolProjectionAdapter - wrap projections into ProtocolEnvelope
```

P10 one-line goal:

```text
Convert AgentTickRecord / AgentReplayLockSet / AgentTrainingSampleDraft into read-only query results and protocol projection DTOs.
```

## P10 Relationship to P8/P9/P4

- P8 produces `AgentTickRecord` / `AgentDecisionRecord` / `AgentTickLog` and bridges to `CommandReplayRecord`.
- P9 produces `AgentReplayLockSet` and `AgentTrainingSampleDraft`.
- P4 provides `ProtocolEnvelope` / `ProtocolPayloadType` base structures.
- P10 reads P8/P9 data and P4 protocol types to build query results and safe projections.
- P10 does NOT modify P8, P9, or P4 types.

## Read-Only Query Principles

- AgentHistoryQueryService only reads AgentTickLog.
- Does NOT modify AgentTickLog.
- Does NOT call AgentRuntime / Policy / RulePipeline / ReplayRunner.
- Does NOT read GameState / ObjectDefinition / hidden truth.

## Safe Projection Principles

- Public mode: only safe summary, no phase_keys / reason_keys / warning_keys.
- Debug mode: allows reason_keys / phase_keys / warning_keys for explainability.
- Training mode: allows training_sample_status summary.
- ALL modes: no hidden truth, no complete GameState, no complete CommandEnvelope, no ObjectDefinition.
- ALL modes: no reward_value, no done / is_done bool.

## P10 Does NOT Do

- No H5 pages
- No HTTP server
- No WebSocket
- No SaveManager
- No JSON serialization
- No database
- No permission system
- No ReplayRunner invocation
- No AgentRuntime invocation
- No RL algorithm
- No Reward / Done computation
- No multi-Agent scheduling
- No new gameplay rules

## File Scope

### Allowed New Files

```text
backend/include/pathfinder/agent/history_query.h
backend/src/agent/history_query.cpp
backend/include/pathfinder/protocol/agent_projection.h
backend/src/protocol/agent_projection.cpp
backend/tests/unit/agent/agent_history_query_test.cpp
backend/tests/unit/protocol/agent_projection_test.cpp
backend/tests/integration/p10/agent_history_projection_flow_test.cpp
backend/tests/integration/p10/agent_no_decision_projection_flow_test.cpp
context_packs/agent_p10.md
```

### Allowed Modifications

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
backend/include/pathfinder/protocol/types.h
backend/src/protocol/types.cpp
backend/tests/unit/protocol/protocol_smoke_test.cpp
```

### Prohibited Modifications

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/
backend/include/pathfinder/pipeline/
backend/src/replay/replay_runner.cpp
frontend/
Any HTTP / WebSocket / SaveManager directory
```

## Verification Commands

```bash
# Build
cmake -S backend -B build/backend
cmake --build build/backend

# P10 targeted tests
ctest --test-dir build/backend -R "agent_history_query|agent_projection|p10|protocol" --output-on-failure

# P8/P9 regression
ctest --test-dir build/backend -R "agent|p9|p8|p7|p6|p5|p4|replay|protocol" --output-on-failure

# Full regression
ctest --test-dir build/backend --output-on-failure

# Boundary: QueryService/Projector/Adapter no runtime/pipeline calls
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner" \
  backend/include/pathfinder/agent/history_query.h \
  backend/src/agent/history_query.cpp \
  backend/include/pathfinder/protocol/agent_projection.h \
  backend/src/protocol/agent_projection.cpp && exit 1 || true

# Boundary: no GameState/ObjectDefinition/hidden truth
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent/history_query.h \
  backend/src/agent/history_query.cpp \
  backend/include/pathfinder/protocol/agent_projection.h \
  backend/src/protocol/agent_projection.cpp \
  backend/tests/unit/agent/agent_history_query_test.cpp \
  backend/tests/unit/protocol/agent_projection_test.cpp \
  backend/tests/integration/p10 && exit 1 || true

# Boundary: no reward_value / done bool
rg -n "reward_value|done =|is_done|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" \
  backend/include/pathfinder/agent \
  backend/include/pathfinder/protocol/agent_projection.h \
  backend/src/agent \
  backend/src/protocol/agent_projection.cpp \
  backend/tests/integration/p10 && exit 1 || true

# Boundary: no HTTP/WebSocket/JSON
rg -n "httplib|asio|WebSocket|HTTP server|SaveManager|nlohmann|json|socket|send\(|recv\(" \
  backend/include/pathfinder/agent/history_query.h \
  backend/src/agent/history_query.cpp \
  backend/include/pathfinder/protocol/agent_projection.h \
  backend/src/protocol/agent_projection.cpp \
  backend/tests/integration/p10 && exit 1 || true
```
