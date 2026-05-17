# Agent P13 Context Pack: Local Debug CLI Pre-Stage

## P13 Goal

P13 adds a small controlled local CLI wrapper around P11/P12:

```text
P11: AgentDebugReport / AgentDiagnosticsSummary / AgentExportDraft
P12: AgentExportWritePlan / AgentLocalExportService / AgentExportVerifier
P13: AgentDebugCliParser / FixtureFactory / Runner / pathfinder_agent_debug_cli
```

P13 one-line goal:

```text
Provide a minimal local command-line tool that uses built-in fixtures to run the P11/P12 debug export pipeline and produce safe local .txt/.md files.
```

## Relationship To P11/P12

- P13 must call P11 `AgentExportDraftBuilder` to build export drafts.
- P13 must call P12 `AgentExportWritePlanner`, `AgentLocalExportService`, and `AgentExportVerifier`.
- P13 must not change P11/P12 DTO semantics.
- P13 is only a CLI shell and orchestration layer.

## P13 Does NOT Do

```text
Does NOT read real SaveGame.
Does NOT read real AgentTickRecord.
Does NOT read real AgentHistoryProjection.
Does NOT read GameState.
Does NOT read ObjectDefinition.
Does NOT parse JSON.
Does NOT output JSON.
Does NOT start HTTP server.
Does NOT start WebSocket.
Does NOT implement H5 pages.
Does NOT implement SaveManager.
Does NOT call AgentRuntime.
Does NOT call Policy.
Does NOT call RulePipeline.
Does NOT call ReplayRunner.
Does NOT compute Reward.
Does NOT judge Done.
Does NOT implement RL.
Does NOT implement LLM.
```

## Allowed CLI Scope

P13 supports only:

```text
--help
export
--fixture unknown_fruit|no_decision|public_safe
--format text|markdown|protocol_text
--output-dir <dir>
--base-name <safe_name>
--dry-run
--overwrite
--max-items-per-chunk <n>
```

P13 must reject:

```text
--json
--http
--websocket
--save
--load-save
--agent-log
--game-state
--rl
--llm
Any unknown argument
```

## Core Types To Implement

```text
AgentDebugCliCommand
AgentDebugCliFixture
AgentDebugCliFormat
AgentDebugCliExitCode
AgentDebugCliOptions
AgentDebugCliResult
AgentDebugCliParseResult
AgentDebugCliParser
AgentDebugFixtureBundle
AgentDebugFixtureFactory
AgentDebugCliRunner
```

## File Scope

### Allowed New Files

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tools/agent_debug_cli_main.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
context_packs/agent_p13.md
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

## argc/argv Boundary

P13 is the first phase that allows a real production CLI entry point.

Allowed `argc` / `argv` locations:

```text
backend/tools/agent_debug_cli_main.cpp
backend/src/agent/debug_cli.cpp, only inside AgentDebugCliParser::parse
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/
```

Not allowed:

```text
Do not parse argc/argv in P11/P12.
Do not put business logic in main.
Do not let main call P11/P12 directly.
```

## Fixture Boundary

P13 fixtures are built-in C++ test/dev data.

Required fixtures:

```text
unknown_fruit: successful decision + replay locked style report.
no_decision: no-decision report with explanation keys or diagnostic summary.
public_safe: public report without reason_keys / phase_keys / warning_keys.
```

Forbidden in fixtures:

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

## Exit Code Mapping

```text
Success = 0
InvalidArguments = 2
BuildFailed = 3
WriteFailed = 4
VerificationFailed = 5
InternalError = 10
```

Exit codes must be stable and covered by tests.

## Minimal Commands

```bash
./build/backend/pathfinder_agent_debug_cli --help

./build/backend/pathfinder_agent_debug_cli export \
  --fixture unknown_fruit \
  --format markdown \
  --output-dir /tmp/pathfinder_p13_cli \
  --base-name unknown_fruit \
  --overwrite

./build/backend/pathfinder_agent_debug_cli export \
  --fixture no_decision \
  --format text \
  --output-dir /tmp/pathfinder_p13_cli \
  --dry-run
```

## Verification Commands

```bash
cmake -S backend -B build/backend
cmake --build build/backend

ctest --test-dir build/backend -R "agent_debug_cli|p13" --output-on-failure

ctest --test-dir build/backend -R "agent|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure

ctest --test-dir build/backend --output-on-failure
```

## Boundary Scans

```bash
# No runtime/pipeline/replay/raw state/history reads
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|AgentTickRecord|AgentHistoryProjection|GameState|ObjectDefinition" \
  backend/include/pathfinder/agent/debug_cli.h \
  backend/src/agent/debug_cli.cpp \
  backend/tools/agent_debug_cli_main.cpp \
  backend/tests/integration/p13 && exit 1 || true

# No network/json/save/RL/LLM
rg -n "httplib|asio|WebSocket|HTTP server|SaveManager|nlohmann|json|socket|send\(|recv\(|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" \
  backend/include/pathfinder/agent/debug_cli.h \
  backend/src/agent/debug_cli.cpp \
  backend/tools/agent_debug_cli_main.cpp \
  backend/tests/integration/p13 && exit 1 || true

# argc/argv must only appear in allowed CLI/parser/test files
rg -n "argc|argv" \
  backend/include/pathfinder/agent/debug_cli.h \
  backend/src/agent/debug_cli.cpp \
  backend/tools/agent_debug_cli_main.cpp \
  backend/tests/unit/agent/agent_debug_cli_test.cpp \
  backend/tests/integration/p13
```

## Completion Criteria

P13 is complete only when:

```text
--help returns success and prints usage.
export unknown_fruit markdown writes files and verifies them.
export no_decision text dry-run does not create files.
invalid base_name fails without writing files.
--json and --load-save are rejected.
CLI result exit codes are stable.
P13 targeted tests pass.
P10-P12 regression tests pass.
Full backend tests pass.
```
