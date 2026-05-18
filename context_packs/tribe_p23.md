# P23 Tribe State Context Pack

## Purpose

P23 adds the first backend tribe-state module for Pathfinder.

It turns safe summaries about members, morale, trust, roles, shared knowledge, and split pressure into a deterministic `TribeState` and safe `TribeProjection`.

P23 is not combat, war, civilization unlock, H5 UI, or post-1.0 expert NPC work.

## Required Reading

Read first:

```text
doc/50_P23族群状态任务卡设计.md
doc/15_族群状态设计.md
doc/39_项目完成总计划路线图.md
doc/13_知识系统设计.md
doc/14_传播系统设计.md
doc/16_文明能力设计.md
```

## Core Scope

Add:

```text
backend/include/pathfinder/tribe/tribe_types.h
backend/include/pathfinder/tribe/tribe_state.h
backend/src/tribe/tribe_types.cpp
backend/src/tribe/tribe_state.cpp
backend/tests/unit/tribe/tribe_types_test.cpp
backend/tests/unit/tribe/tribe_state_test.cpp
backend/tests/integration/p23/tribe_state_flow_test.cpp
backend/tests/integration/p23/tribe_state_security_test.cpp
```

Modify:

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

Implement:

```text
TribeId
TribeMemberRole
TribeCohesionState
TribeStateChangeKind
TribeMember
TribeMorale
TribeTrust
TribeSplitRisk
TribeState
TribeStateInput
TribeStateOptions
TribeStateChangeDraft
TribeProjection
TribeTrace
TribeStateResolveResult
TribeStateResolver
TribeProjectionBuilder
```

## Dependency Rules

Allowed:

```text
tribe -> foundation
tribe -> knowledge
```

Avoid in P23:

```text
tribe -> h5_dialog
tribe -> learning
tribe -> agent_runtime
tribe -> pipeline
tribe -> replay
```

## Forbidden

Do not implement:

```text
joint combat / assist / defend / pack tactics
tribe war / diplomacy resolution
civilization capability unlock
object reactions
new H5 UI
HTTP / WebSocket / JSON API
expert NPC teaching
NPC harm detection / murder inference
SaveManager
RL reward/done
```

Do not read or accept unsafe inputs:

```text
hidden_truth
real_effect
true_trait
object_truth
raw_state
GameState
SaveGame
AgentRuntime
Policy
random_split
probability_split
```

## Determinism Rule

Split risk must be deterministic.

Use a pure score such as:

```text
risk = clamp(resource_pressure * 0.30
           + trust_pressure * 0.30
           + knowledge_conflict_pressure * 0.25
           + casualty_pressure * 0.15)
```

Do not use random numbers to decide split state.

## Minimum Stories

### Tribe Creation

```text
Add pioneer and companion.
Pioneer role=Pioneer.
Companion role=Learner.
population_total=2.
active_population=2.
Projection shows role summary.
```

### Role Change

```text
Companion changes Learner -> Gatherer.
StateChange includes MemberRoleChanged.
Projection updates role summary.
```

### Morale And Trust

```text
recent_success increases morale.
leader trust decrease updates trust summary.
Trace explains both.
```

### Split Risk

```text
resource/trust/knowledge/casualty pressures produce deterministic risk.
cohesion_state follows threshold.
No random split.
```

### Knowledge Summary

```text
shared_knowledge_ids can link tribe-level knowledge.
Projection only exposes count/summary, not raw claims.
```

## Required Tests

Unit:

```text
tribe_role_enum_roundtrip
tribe_cohesion_enum_roundtrip
tribe_member_validate
tribe_state_validate
tribe_projection_safe_summary
tribe_resolver_member_join
tribe_resolver_role_change
tribe_resolver_morale_trust
tribe_resolver_split_risk
```

Integration:

```text
p23_tribe_creation_flow
p23_tribe_role_and_projection_flow
p23_split_risk_deterministic_flow
p23_security_forbidden_keys
p23_security_rejects_random_split
```

Commands:

```bash
cmake --build build/backend
ctest --test-dir build/backend -R "tribe|p23" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## Review Red Flags

Reject implementation if it:

```text
uses random split
uses tribe_level as the main model
reads raw GameState or SaveGame
adds combat/war/civilization unlock logic
adds H5 or HTTP changes
stores hidden truth in TribeProjection
omits validateBasic on ok DTOs
lacks trace for risk/morale/trust changes
```
