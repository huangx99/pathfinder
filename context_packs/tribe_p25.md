# P25 Tribe Conflict Context Pack

## Purpose

P25 adds deterministic inter-tribe conflict for Pathfinder.

It turns two validated tribe conflict participants plus P24 `CombatCoordinationResult` summaries into hostility changes, conflict outcomes, safe state drafts, events, projection, and trace.

P25 is not a full combat simulator. It does not resolve random damage, HP, death, kills, loot, weapons, diplomacy treaties, civilization unlocks, H5 UI, or expert NPC teaching.

## Required Reading

Read first:

```text
doc/52_P25族群冲突任务卡设计.md
doc/51_P24共同作战任务卡设计.md
doc/50_P23族群状态任务卡设计.md
doc/review/P24共同作战复审报告.md
doc/review/P23族群状态复审报告.md
doc/15_族群状态设计.md
doc/39_项目完成总计划路线图.md
```

## Core Scope

Add:

```text
backend/include/pathfinder/tribe/tribe_conflict.h
backend/src/tribe/tribe_conflict.cpp
backend/tests/unit/tribe/tribe_conflict_test.cpp
backend/tests/integration/p25/tribe_conflict_flow_test.cpp
backend/tests/integration/p25/tribe_conflict_security_test.cpp
```

Modify:

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

Implement at minimum:

```text
HostilityState
ConflictIntentKind
ConflictStanceKind
ConflictOutcomeKind
ConflictEventKind
TribeRelation
TribeRelationDraft
ConflictPressureSummary
ConflictParticipantTribe
ConflictIntent
ConflictStateDraft
ConflictEvent
ConflictProjection
ConflictTrace
ConflictResolutionInput
ConflictResolutionOptions
ConflictResolutionResult
GroupCombatResolver
ConflictProjectionBuilder
ConflictStateDraftBuilder
TribeRelationTransitionPolicy
```

## Dependency Rules

Allowed:

```text
pathfinder_tribe -> pathfinder_foundation
pathfinder_tribe -> pathfinder_knowledge
pathfinder_tribe -> tribe_coordination types already in tribe module
```

Avoid in P25:

```text
pathfinder_h5_dialog
pathfinder_learning
pathfinder_agent_runtime
pathfinder_pipeline
pathfinder_replay
pathfinder_save
```

## Critical P23 Constraint

P23 rejects duplicate `member_id` inside one `TribeStateInput.member_events`.

Therefore P25 must obey:

```text
Aggregate multiple same-member conflict effects before producing any TribeStateInput draft.
If building a TribeStateInput draft, emit at most one member event per member_id.
Prefer tribe-level morale_delta/trust_delta/safety_pressure_delta/loss_pressure_delta/split_risk_delta.
Do not emit per-member events unless necessary.
```

Required test:

```text
conflict_state_draft_unique_member_events
p25_security_rejects_duplicate_member_effects
```

## Critical P24 Constraint

P25 must build on P24 and must not bypass it.

Required behavior:

```text
Read CombatCoordinationResult.
Read CombatCoordinationStateDraft.
Use P24 coordination_score/quality in conflict strength.
Reject missing or invalid coordination result.
Do not recompute GroupIntent, AssistAction, DefendAction, or PackTactic in P25.
```

Required test:

```text
p25_uses_p24_coordination_result_flow
p25_security_rejects_missing_coordination_result
```

## Forbidden

Do not implement:

```text
full combat simulator
hit chance
random damage
HP loss
death
kill result
loot drop
weapon stats
large diplomacy system
alliance/vassal/trade treaty
civilization unlock
object reactions
H5 or HTTP changes
expert NPC teaching
NPC harm detection
SaveManager
RL reward/done
```

Do not read, accept, project, or trace unsafe keys:

```text
raw_state
GameState
SaveGame
hidden_truth
true_trait
real_effect
real_damage
true_hp
actual_hp
actual_damage
damage_roll
random_hit
critical_hit
death
kill
loot
corpse
hidden_weapon_stat
```

## Determinism Rule

Same input + same options must produce same result.

No random numbers.

Use deterministic scores based on:

```text
P24 coordination_score
morale_summary
trust_summary
split_risk_summary
safety_pressure_summary
loss_pressure_summary
resource_pressure
territory_pressure
fear_pressure
survival_pressure
hostility_state
```

## Safe Loss Rule

Allowed:

```text
loss_pressure_delta
casualty_pressure_delta
retreat_pressure_delta
safety_pressure_delta
morale_pressure_delta
```

Forbidden:

```text
member_dead
enemy_dead
kill_count
hp_delta
body_count
loot_count
```

## Minimum Stories

### Resource Standoff

```text
Two wary tribes contest a visible resource summary.
Both have similar P24 coordination scores.
Outcome is Standoff.
Relation draft increases hostility/resource tension.
No death/damage/loot output.
```

### High Coordination Intimidates Low Morale

```text
Source tribe has strong P24 coordination and high morale/trust.
Target tribe has weak coordination and low morale.
Outcome is Intimidated or ForcedRetreat.
Target receives retreat/loss/safety pressure draft.
```

### Low Morale Retreat

```text
Source tribe has low morale or high split risk.
Threat/fear pressure is high.
Resolver chooses Retreat/Avoided/ForcedRetreat summary.
Trace explains low morale/high split risk/high fear pressure.
```

### Truce Offered

```text
Both tribes have high loss pressure and resource pressure has dropped.
One side attempts NegotiateTruce.
Outcome is TruceOffered.
Relation draft moves toward Truce without creating alliance/trade.
```

## Required Tests

Unit:

```text
tribe_conflict_hostility_enum_roundtrip
tribe_conflict_intent_enum_roundtrip
tribe_conflict_outcome_enum_roundtrip
tribe_relation_validate
conflict_pressure_summary_validate
conflict_participant_requires_coordination_result
conflict_resolution_input_validate
conflict_resolution_result_validate
conflict_projection_safe_summary
conflict_trace_safe_summary
conflict_state_draft_unique_member_events
```

Integration:

```text
p25_neutral_low_pressure_avoids_conflict_flow
p25_resource_contest_standoff_flow
p25_high_coordination_intimidates_low_morale_flow
p25_low_morale_forced_retreat_flow
p25_hostility_escalates_deterministically_flow
p25_loss_pressure_summary_without_death_flow
p25_truce_offered_when_pressure_drops_flow
p25_uses_p24_coordination_result_flow
p25_relation_draft_does_not_mutate_state_flow
p25_deterministic_resolution_flow
```

Security:

```text
p25_security_rejects_missing_coordination_result
p25_security_rejects_raw_game_state_keys
p25_security_rejects_hidden_truth_keys
p25_security_rejects_damage_death_kill_loot_keys
p25_security_projection_has_no_hidden_truth
p25_security_trace_has_no_hp_or_death
p25_security_rejects_duplicate_member_effects
p25_security_rejects_same_source_target_tribe
p25_security_rejects_test_only_enum_in_production
```

Commands:

```bash
cmake --build build/backend
ctest --test-dir build/backend -R "tribe_conflict|p25" --output-on-failure
ctest --test-dir build/backend -R "tribe|p23|p24|p25" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## Review Red Flags

Reject implementation if it:

```text
recomputes P24 coordination inside P25
reads raw GameState or SaveGame
stores hidden truth in projection or trace
uses random hit/damage
changes HP or kills entities
outputs loot or corpse data
mutates TribeState directly
emits duplicate member_id in a P23 TribeStateInput draft
implements civilization unlock
implements object reaction
implements dangerous exploration pressure
lets H5/frontend infer conflict results
omits validateBasic on ok DTOs
lacks trace for intent/outcome/relation decisions
```
