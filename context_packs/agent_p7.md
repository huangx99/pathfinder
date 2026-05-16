# Agent P7 Context Pack: AgentRuntime Minimum Closed Loop

## Goal

P7 builds the minimum AgentRuntime that orchestrates the full agent tick:

```text
GameState + VisibilityInput
  -> ObservationBuilder
  -> AgentObservation
  -> ActionSpaceBuilder
  -> AgentActionSpace
  -> FirstSupportedPolicy
  -> AgentDecision
  -> AgentCommandAdapter
  -> CommandEnvelope
  -> RulePipeline
  -> PipelineResult
```

P7 does NOT make Agent smart. P7 replaces hand-written test flows with a reusable, testable, replayable runtime.

## Three-Layer Separation

```text
Builder layer:  ObservationBuilder / ActionSpaceBuilder - project world state into subjective observation
Policy layer:   FirstSupportedPolicy - only sees observation/action_space, selects one candidate
Runtime layer:  AgentRuntime - orchestrates builder, policy, adapter, pipeline in fixed order
```

## Forbidden

```text
AgentService, AgentStore, AgentGoal
BehaviorTree, GOAP, UtilityPolicy, NeuralPolicy, LLMPolicy, RLPolicy
PerceptionSystem, FleeResolver, GroupCombat, WarResolver, tribe_split
H5, HTTP, WebSocket, SaveManager
Policy reading GameState/ObjectStore/ActorStore
Runtime directly modifying GameState
Runtime bypassing AgentCommandAdapter
Runtime treating PipelineResult::emptySuccess as real action success
```

## Allowed

```text
AgentPolicyRequest, AgentPolicy interface, FirstSupportedPolicy
AgentRuntimeStatus, AgentRuntimeOptions, AgentRuntimeTrace
AgentTickRequest, AgentTickResult
AgentRuntime with tickOne()
command_action_id and suggested_targets on AgentActionCandidate
submit_action_allowlist for controlled pipeline submission
P7 integration tests reusing P3 UnknownFruitFixture
```

## Key Boundaries

```text
Policy: no GameState, no RulePipeline, no edible_profile, no effect_kind
Runtime: only submits commands whose action_id is in submit_action_allowlist
Flee: can generate CommandEnvelope but NOT submitted to pipeline (not in allowlist)
CallGroup: command_supported=false, no command generated
RulePipeline: remains the sole rule settlement entry point
AgentCommandAdapter: remains the sole CommandEnvelope generator
```

## P7 Action Candidate Semantic Fix

P6 had a gap: `AgentActionCandidate` only had `action_id` (candidate instance ID) and `required_target_types`, but no concrete targets or semantic command action ID.

P7 adds:

```text
command_action_id: semantic action id for CommandEnvelope (e.g. "eat")
suggested_targets: pre-built targets for Policy to construct AgentIntent
```

Rules:

```text
action_id = candidate instance id (e.g. eat_obj_unknown_fruit_001) - for trace, sorting, debugging
command_action_id = executable command semantic id (e.g. eat) - for CommandEnvelope
If command_action_id is empty, Policy falls back to action_id
suggested_targets must satisfy required_target_types
```

## P7 Submit Allowlist Principle

```text
submit_action_allowlist controls which commands reach RulePipeline
P7 unknown fruit test: allowlist = [eat]
Flee/CallGroup not in allowlist -> SubmitSkipped, no state change
Future ActionRegistry may replace allowlist, but Runtime flow stays the same
```

## P7 Relationship to P5/P6

```text
P5: Agent data contracts (AgentBinding, AgentIntent, AgentDecision, AgentCommandAdapter)
P6: ObservationBuilder, ActionSpaceBuilder, builder_types
P7: AgentRuntime, AgentPolicy, FirstSupportedPolicy, runtime_types
P7 depends on P0-P6, does not modify rules/object/pipeline internals
```

## Task Checklist

```text
TASK-P7-000: Context pack (this file)
TASK-P7-001: Fix ActionCandidate semantics (command_action_id, suggested_targets)
TASK-P7-002: Policy interface and FirstSupportedPolicy
TASK-P7-003: Runtime types (AgentRuntimeStatus, AgentTickRequest/Result, etc.)
TASK-P7-004: AgentRuntime build phase (observation + action_space)
TASK-P7-005: Runtime decision and command adaptation
TASK-P7-006: Runtime controlled pipeline submission (allowlist)
TASK-P7-007: unknown fruit runtime integration test
TASK-P7-008: threat runtime no-submit integration test
TASK-P7-009: Boundary audit and dev notes
```

## Verification Commands

```bash
# Build
cmake -S backend -B build/backend
cmake --build build/backend

# Full test suite
ctest --test-dir build/backend --output-on-failure

# P7 targeted tests
ctest --test-dir build/backend -R "agent_policy|agent_runtime_types|agent_runtime|p7" --output-on-failure

# P5/P6 regression
ctest --test-dir build/backend -R "agent|p5|p6|p3|p4" --output-on-failure

# Boundary scan: Policy no GameState
rg -n "GameState|object_store|actor_store|RulePipeline|EatObjectResolver|edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent/policy.h \
  backend/src/agent/policy.cpp \
  backend/tests/unit/agent/agent_policy_test.cpp && exit 1 || true

# Boundary scan: no effect data in agent code
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/unit/agent \
  backend/tests/integration/p7 && exit 1 || true

# Boundary scan: no forbidden systems
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat|rl_environment|BehaviorTree|GOAP|UtilityPolicy|NeuralPolicy|LLMPolicy|WebSocket|HTTP|SaveManager" \
  backend/include/pathfinder/agent \
  backend/src/agent \
  backend/tests/integration/p7 && exit 1 || true
```
