# Agent P11 Context Pack: Agent Local Debug Report & Export Pre-Draft

## P11 Goal

P11 transforms P10 read-only projections into human-readable debug reports and in-memory export drafts:

```text
P8 Record: AgentTickRecord / AgentDecisionRecord / AgentTickLog
P9 Lock/Draft: AgentReplayLockSet / AgentTrainingSampleDraft
P10 Projection: AgentHistoryProjection / AgentReplayLockProjection / AgentTrainingSampleProjection / ProtocolEnvelope
P11 DebugReport: AgentDebugReport - structured debug report from P10 projections
P11 Diagnostics: AgentDiagnosticsSummary - failure/risk/boundary summary for AI review
P11 ExportDraft: AgentExportManifest + AgentExportChunk - in-memory export draft
```

P11 one-line goal:

```text
Convert P10 AgentHistoryProjection into readable local debug reports and in-memory export chunks for dev, AI review, and future CLI/H5 reuse.
```

## P11 Relationship to P10/P9/P8

- P8 produces `AgentTickRecord` / `AgentDecisionRecord` / `AgentTickLog`.
- P9 produces `AgentReplayLockSet` / `AgentTrainingSampleDraft`.
- P10 produces `AgentHistoryProjection` / `AgentReplayLockProjection` / `AgentTrainingSampleProjection` / `ProtocolEnvelope`.
- P11 reads P10 projections to build debug reports, diagnostics, and export drafts.
- P11 does NOT modify P10, P9, P8, or any upstream types.

## DebugReport / Diagnostics / ExportDraft Layer Boundaries

- **DebugReport layer**: Converts `AgentHistoryProjection` into `AgentDebugReport` with per-item severity, summary keys, and status counts.
- **Diagnostics layer**: Generates `AgentDiagnosticsSummary` from `AgentDebugReport` for AI review and failure localization.
- **ExportDraft layer**: Wraps report + diagnostics into `AgentExportManifest` + `AgentExportChunk` for future CLI/file export.

## P11 Does NOT Do

```text
Does NOT write local files.
Does NOT implement CLI.
Does NOT serialize JSON.
Does NOT write Markdown files.
Does NOT start HTTP server.
Does NOT start WebSocket.
Does NOT implement H5 pages.
Does NOT implement SaveManager.
Does NOT implement database.
Does NOT call AgentRuntime.
Does NOT call Policy.
Does NOT call RulePipeline.
Does NOT call ReplayRunner.
Does NOT compute Reward.
Does NOT judge Done.
Does NOT implement RL.
Does NOT implement permission system.
```

## File Scope

### Allowed New Files

```text
backend/include/pathfinder/agent/debug_report.h
backend/include/pathfinder/agent/debug_export.h
backend/src/agent/debug_report.cpp
backend/src/agent/debug_export.cpp
backend/tests/unit/agent/agent_debug_report_test.cpp
backend/tests/unit/agent/agent_debug_export_test.cpp
backend/tests/integration/p11/agent_debug_report_export_flow_test.cpp
backend/tests/integration/p11/agent_no_decision_debug_report_test.cpp
context_packs/agent_p11.md
```

### Allowed Modifications

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

### Prohibited Modifications

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/
backend/include/pathfinder/pipeline/
backend/src/replay/
backend/include/pathfinder/replay/
backend/src/protocol/ (unless P10 type naming blocks and must add include)
frontend/
Any HTTP / WebSocket / SaveManager directory
```

## Verification Commands

```bash
# Build
cmake -S backend -B build/backend
cmake --build build/backend

# P11 targeted tests
ctest --test-dir build/backend -R "agent_debug_report|agent_debug_export|p11" --output-on-failure

# P8/P9/P10 regression
ctest --test-dir build/backend -R "agent|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure

# Full regression
ctest --test-dir build/backend --output-on-failure

# Boundary: no runtime/pipeline/replay calls in P11 code
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|AgentTickRecord" \
  backend/include/pathfinder/agent/debug_report.h \
  backend/src/agent/debug_report.cpp \
  backend/include/pathfinder/agent/debug_export.h \
  backend/src/agent/debug_export.cpp && exit 1 || true

# Boundary: no GameState/ObjectDefinition/hidden truth
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent/debug_report.h \
  backend/src/agent/debug_report.cpp \
  backend/include/pathfinder/agent/debug_export.h \
  backend/src/agent/debug_export.cpp \
  backend/tests/unit/agent/agent_debug_report_test.cpp \
  backend/tests/unit/agent/agent_debug_export_test.cpp \
  backend/tests/integration/p11 && exit 1 || true

# Boundary: no file I/O or HTTP/WebSocket
rg -n "fstream|ofstream|ifstream|filesystem|file_path|absolute_path|write_result|nlohmann|json|socket|send\(|recv\(|HTTP|WebSocket|SaveManager" \
  backend/include/pathfinder/agent/debug_report.h \
  backend/src/agent/debug_report.cpp \
  backend/include/pathfinder/agent/debug_export.h \
  backend/src/agent/debug_export.cpp \
  backend/tests/integration/p11 && exit 1 || true

# Boundary: no reward/done/RL
rg -n "reward_value|done =|is_done|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/integration/p11 && exit 1 || true
```
