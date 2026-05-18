# P24 Tribe Coordination Context Pack

## Purpose

P24 adds deterministic intra-tribe coordination for Pathfinder.

It turns a validated `TribeState` plus safe threat summaries into group intent, assist actions, defend actions, a pack tactic, coordination projection, trace, and a safe state-impact draft.

P24 is not damage resolution, death, loot, tribe war, diplomacy, civilization unlock, H5 UI, or expert NPC teaching.

## Required Reading

Read first:

```text
doc/51_P24共同作战任务卡设计.md
doc/50_P23族群状态任务卡设计.md
doc/review/P23族群状态复审报告.md
doc/15_族群状态设计.md
doc/39_项目完成总计划路线图.md
```

## Core Scope

Add:

```text
backend/include/pathfinder/tribe/tribe_coordination.h
backend/src/tribe/tribe_coordination.cpp
backend/tests/unit/tribe/tribe_coordination_test.cpp
backend/tests/integration/p24/tribe_coordination_flow_test.cpp
backend/tests/integration/p24/tribe_coordination_security_test.cpp
```

Modify:

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

Implement at minimum:

```text
GroupIntentKind
AssistActionKind
DefendActionKind
PackTacticKind
CoordinationQuality
ThreatPressureSummary
CombatParticipant
GroupIntent
AssistAction
DefendAction
PackTactic
MemberCoordinationSummary
CombatCoordinationInput
CombatCoordinationOptions
CombatCoordinationStateDraft
CombatCoordinationProjection
CombatCoordinationTrace
CombatCoordinationResult
CombatCoordinationResolver
CombatCoordinationProjectionBuilder
```

Optional only if needed:

```text
TribeStateInputDraftBuilder
```

## Dependency Rules

Allowed:

```text
pathfinder_tribe -> pathfinder_foundation
pathfinder_tribe -> pathfinder_knowledge
```

Avoid in P24:

```text
pathfinder_h5_dialog
pathfinder_learning
pathfinder_agent_runtime
pathfinder_pipeline
pathfinder_replay
```

## Critical P23 Constraint

P23 rejects duplicate `member_id` inside one `TribeStateInput.member_events`.

Therefore P24 must obey:

```text
Aggregate multiple same-member coordination effects into MemberCoordinationSummary.
If building a TribeStateInput draft, emit at most one member event per member_id.
First version should prefer tribe-level morale_delta/trust_delta/safety_pressure/casualty_pressure and avoid member_events unless necessary.
```

Required test:

```text
p24_state_draft_unique_member_events_flow
```

## Forbidden

Do not implement:

```text
damage resolution
hit chance
random damage
HP loss
death
kill result
loot drop
weapons stats
tribe-vs-tribe war
hostility/diplomacy resolver
civilization unlock
object reactions
H5 or HTTP changes
expert NPC teaching
NPC harm detection
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
random_hit
random_damage
damage_roll
critical_hit
true_hp
actual_damage
kill_result
loot_drop
war_result
```

## Determinism Rule

Same input + same options must produce same result.

No random numbers.

Use deterministic scores based on:

```text
role contribution
member morale
member trust
fatigue_pressure
injury_pressure
tribe split_risk
threat_pressure
safety_pressure
```

## Minimum Stories

### Guardian Defends Learner

```text
Pioneer + Guardian + Learner.
Threat pressure is high.
Guardian produces a DefendAction.
PackTactic is Stable or Strong.
No damage/death/kill output.
```

### Low Morale Retreat

```text
Morale low, trust low or split risk high.
Threat pressure high.
Resolver chooses AvoidEngagement or ScreenRetreat.
Trace explains low morale / high threat / split risk.
```

### High Trust Focus Threat

```text
Pioneer + Guardian + Explorer.
Morale and trust high.
Threat pressure medium.
Resolver chooses FocusThreat / FocusPressure.
Two identical resolves produce identical result.
```

## Required Tests

Unit:

```text
tribe_coordination_intent_enum_roundtrip
tribe_coordination_action_enum_roundtrip
tribe_coordination_quality_threshold
tribe_coordination_input_validate
tribe_coordination_result_validate
tribe_coordination_projection_safe_summary
tribe_coordination_member_summary_unique
```

Integration:

```text
p24_guardian_defends_learner_flow
p24_explorer_warns_group_flow
p24_high_trust_focus_threat_flow
p24_low_morale_avoid_engagement_flow
p24_split_risk_weakens_coordination_flow
p24_state_draft_unique_member_events_flow
p24_deterministic_coordination_flow
p24_security_forbidden_keys
p24_security_rejects_damage_or_death
p24_security_rejects_duplicate_member_effects
```

Commands:

```bash
cmake --build build/backend
ctest --test-dir build/backend -R "tribe_coordination|p24" --output-on-failure
ctest --test-dir build/backend -R "tribe|p23|p24" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## Review Red Flags

Reject implementation if it:

```text
uses random hit/damage
changes HP or kills entities
reads raw GameState or SaveGame
stores hidden truth in projection
implements tribe war or diplomacy
implements civilization unlock
lets H5/frontend infer coordination results
emits duplicate member_id in a P23 TribeStateInput draft
omits validateBasic on ok DTOs
lacks trace for intent/tactic/quality decisions
```
