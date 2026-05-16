# Agent P6 Context Pack: Observation & ActionSpace Builders

## Goal

P6 builds two minimum builders on top of P5 Agent data contracts:

```text
ObservationBuilder: from GameState + VisibilityInput -> AgentObservation
ActionSpaceBuilder: from AgentObservation -> AgentActionSpace
```

P6 does NOT make Agent smart. P6 only removes hand-written observation/action_space from tests.

## Forbidden

```text
AgentRuntime, AgentPolicy, RuleBasedPolicy, PlayerPolicy, TestPolicy
Behavior trees, GOAP, utility AI, neural networks, LLM decisions
RL environment
Map, pathfinding, perception range system
Real combat system, real flee resolver, real call_pack resolver
Tribe split, war, group combat settlement
Use / Combine / Collect / ReactionRule gameplay extensions
Expression engine, ConfigRegistry full implementation
H5 frontend, HTTP, WebSocket, SaveManager
```

## Allowed

```text
ObservationBuilder (reads GameState, does NOT modify it)
ActionSpaceBuilder (reads AgentObservation, does NOT read GameState)
builder request/result data structures
Optional availability/hint fields on AgentObservedObject
P6 context pack, P6 integration tests
Reuse P3 UnknownFruitFixture
Integration tests calling RulePipeline for closed-loop verification
```

## Key Boundaries

```text
ObservationBuilder: reads GameState, does NOT modify it, does NOT call RulePipeline/Resolver
ObservationBuilder: does NOT expose real edible_profile hunger_delta/health_delta/effect_kind
ActionSpaceBuilder: reads AgentObservation only, does NOT read GameState
ActionSpaceBuilder: does NOT generate CommandEnvelope, does NOT select actions
RulePipeline still authoritative for settlement
```

## Task Checklist

```text
TASK-P6-000: Context pack (this file)
TASK-P6-001: Builder types & project skeleton
TASK-P6-002: ObservationBuilder basic validation
TASK-P6-003: Visible object observation projection
TASK-P6-004: Cognition claim -> confidence projection
TASK-P6-005: Threat seed -> observation projection
TASK-P6-006: ActionSpaceBuilder basic validation
TASK-P6-007: observed object -> Eat/Explore candidates
TASK-P6-008: threat -> Flee/CallGroup candidates
TASK-P6-009: unknown fruit builder integration closed-loop
TASK-P6-010: Spider/Wolf builder integration tests
TASK-P6-011: Boundary audit
```

## Verification Commands

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent|p6|p5|p3|p4" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "RulePipeline|EatObjectResolver|StateChange|EventRecord" backend/include/pathfinder/agent/observation_builder.h backend/src/agent/observation_builder.cpp backend/include/pathfinder/agent/action_space_builder.h backend/src/agent/action_space_builder.cpp && exit 1 || true
rg -n "GameState|state::" backend/include/pathfinder/agent/action_space_builder.h backend/src/agent/action_space_builder.cpp && exit 1 || true
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p6 && exit 1 || true
```
