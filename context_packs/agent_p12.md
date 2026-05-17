# Agent P12 Context Pack: Local File Export & Verification

## P12 Goal

P12 turns P11 in-memory `AgentExportDraft` into safe local `.txt` / `.md` files:

```text
P10 Projection: AgentHistoryProjection / ProtocolEnvelope
P11 Debug Export: AgentDebugReport / AgentDiagnosticsSummary / AgentExportDraft
P12 Local Export: AgentExportWritePlan / AgentExportWriteResult / AgentExportVerificationReport
```

P12 one-line goal:

```text
Write P11 AgentExportDraft into a controlled local directory and verify file count, size, path safety, and forbidden content boundaries.
```

## P12 Relationship to P11/P10

- P10 creates safe protocol projections.
- P11 creates readable debug reports and in-memory export chunks.
- P12 consumes P11 `AgentExportDraft` only.
- P12 must not read `AgentTickRecord`, `AgentHistoryProjection`, `GameState`, or `ObjectDefinition` directly.
- P12 must not change P10/P11 DTO semantics.

## P12 Layer Boundaries

- **PathPolicy layer**: validates `root_dir`, relative paths, file count, and byte limits.
- **WritePlan layer**: converts `AgentExportDraft` into deterministic file plans.
- **LocalExport layer**: uses local filesystem write and verification.

## Important Difference From P11

P11 explicitly forbids file I/O.

P12 explicitly allows only these local file operations:

```text
std::filesystem for controlled directory creation and existence checks.
std::ofstream for writing planned files.
std::ifstream may be used by verifier to read back exported files.
```

The file I/O boundary is strict:

```text
Only root_dir + safe relative_path.
No absolute target path in file plans.
No path traversal.
No arbitrary file delete.
No non-P12 directory cleanup.
```

## P12 Does NOT Do

```text
Does NOT implement CLI.
Does NOT parse argv/argc.
Does NOT serialize JSON.
Does NOT start HTTP server.
Does NOT start WebSocket.
Does NOT implement H5 pages.
Does NOT implement SaveManager.
Does NOT implement database.
Does NOT call AgentRuntime.
Does NOT call Policy.
Does NOT call RulePipeline.
Does NOT call ReplayRunner.
Does NOT read GameState.
Does NOT read ObjectDefinition.
Does NOT compute Reward.
Does NOT judge Done.
Does NOT implement RL.
Does NOT implement LLM.
```

## File Scope

### Allowed New Files

```text
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
backend/tests/integration/p12/agent_local_export_flow_test.cpp
backend/tests/integration/p12/agent_local_export_security_test.cpp
context_packs/agent_p12.md
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
backend/src/protocol/
backend/include/pathfinder/protocol/
frontend/
Any HTTP / WebSocket / SaveManager directory
```

## Core Types To Implement

```text
AgentExportFileId
AgentExportWriteId
AgentExportVerifyId
AgentExportFileKind
AgentExportFileExtension
AgentExportWriteStatus
AgentExportVerificationStatus
AgentExportPathPolicy
AgentExportFilePlan
AgentExportWritePlan
AgentExportFileWriteResult
AgentExportWriteResult
AgentExportVerificationReport
AgentExportWritePlanRequest
AgentExportWritePlanner
AgentLocalExportService
AgentExportVerifyRequest
AgentExportVerifier
```

## Deterministic Naming

```text
<safe_base_name>_manifest.<ext>
<safe_base_name>_chunk_<index>.<ext>
```

Extension mapping:

```text
PlainText -> .txt
ProtocolText -> .txt
MarkdownLike -> .md
```

P12 must not introduce `.json`.

## Path Safety Rules

- `root_dir` must be non-empty.
- `relative_path` must be relative.
- `relative_path` must not start with `/`.
- `relative_path` must not contain `..`.
- `relative_path` must not contain backslash.
- `base_name` allows only letters, digits, `_`, and `-`.
- File plans must not contain duplicate `relative_path`.
- Tests may write only under `build/backend` P12 temp dirs or `/tmp`.

## Forbidden Content Terms

Verifier must be able to scan exported content for at least:

```text
GameState
ObjectDefinition
edible_profile
hunger_delta
health_delta
effect_kind
reward_value
is_done
done =
```

## Verification Commands

```bash
cmake -S backend -B build/backend
cmake --build build/backend

ctest --test-dir build/backend -R "agent_local_export|p12" --output-on-failure

ctest --test-dir build/backend -R "agent|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure

ctest --test-dir build/backend --output-on-failure
```

## Boundary Scans

```bash
# No runtime/pipeline/replay/raw history reads
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|AgentTickRecord|AgentHistoryProjection" \
  backend/include/pathfinder/agent/local_export.h \
  backend/src/agent/local_export.cpp \
  backend/tests/integration/p12 && exit 1 || true

# No hidden truth
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent/local_export.h \
  backend/src/agent/local_export.cpp \
  backend/tests/unit/agent/agent_local_export_test.cpp \
  backend/tests/integration/p12 && exit 1 || true

# No network/json/CLI/SaveManager
rg -n "httplib|asio|WebSocket|HTTP server|SaveManager|nlohmann|json|socket|send\(|recv\(|argc|argv" \
  backend/include/pathfinder/agent/local_export.h \
  backend/src/agent/local_export.cpp \
  backend/tests/integration/p12 && exit 1 || true

# No reward/done/RL/LLM
rg -n "reward_value|done =|is_done|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" \
  backend/include/pathfinder/agent/local_export.h \
  backend/src/agent/local_export.cpp \
  backend/tests/integration/p12 && exit 1 || true
```

## Completion Criteria

P12 is complete only when:

```text
AgentExportDraft can be planned into stable local files.
MarkdownLike exports write `.md` files.
PlainText / ProtocolText exports write `.txt` files.
Overwrite policy is enforced.
Path traversal is rejected.
Verifier catches missing files, size mismatch, and forbidden content.
P12 targeted tests pass.
P8-P11 regression tests pass.
Full backend tests pass.
```
