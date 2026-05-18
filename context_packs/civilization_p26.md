# P26 Civilization Capability Context Pack

## Purpose

P26 adds the first deterministic civilization capability system for Pathfinder.

It turns stable knowledge, propagation coverage, tribe state summaries, P24 coordination summaries, and P25 conflict summaries into civilization capability candidates, maturity states, effect drafts, safe projection, events, and trace.

P26 is not a tech tree UI, production chain, object reaction system, dangerous exploration system, combat damage system, or frontend unlock system.

## Required Reading

Read first:

```text
doc/53_P26文明能力任务卡设计.md
doc/16_文明能力设计.md
doc/52_P25族群冲突任务卡设计.md
doc/review/P25族群冲突最终复审报告.md
doc/51_P24共同作战任务卡设计.md
doc/50_P23族群状态任务卡设计.md
doc/13_知识系统设计.md
doc/14_传播系统设计.md
doc/39_项目完成总计划路线图.md
```

## Core Scope

Add:

```text
backend/include/pathfinder/civilization/civilization_types.h
backend/include/pathfinder/civilization/civilization_state.h
backend/include/pathfinder/civilization/civilization_resolver.h
backend/src/civilization/civilization_types.cpp
backend/src/civilization/civilization_state.cpp
backend/src/civilization/civilization_resolver.cpp
backend/tests/unit/civilization/civilization_types_test.cpp
backend/tests/unit/civilization/civilization_state_test.cpp
backend/tests/unit/civilization/civilization_resolver_test.cpp
backend/tests/integration/p26/civilization_capability_flow_test.cpp
backend/tests/integration/p26/civilization_capability_security_test.cpp
```

Modify:

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

Create target:

```text
pathfinder_civilization
```

## Minimum Types

Implement at minimum:

```text
CivilizationStage
CapabilityType
CapabilityMaturityState
CapabilityUsabilityState
CapabilityChangeReason
CapabilityEffectTarget
EffectOperationType
CivilizationCapability
CivilizationCapabilityState
CivilizationProgress
CapabilityUnlockCondition
CapabilityEvidenceBundle
CapabilityEffectDraft
CivilizationState
CivilizationCapabilityCandidate
CapabilityRequirementSnapshot
CivilizationStateChangeDraft
CivilizationProjection
CivilizationTrace
CivilizationResolveInput
CivilizationResolveResult
CivilizationCandidateBuilder
CapabilityRequirementEvaluator
CapabilityStateResolver
CapabilityEffectResolver
CivilizationStageResolver
CivilizationResolver
```

## Minimum Capabilities

P26 first version must support:

```text
IdentifyEdible
IdentifyDanger
SafeForaging
ConflictDeescalation
```

Other capability enums may exist as placeholders, but do not implement complex content early.

## Dependency Rules

Allowed:

```text
pathfinder_civilization -> pathfinder_foundation
pathfinder_civilization -> pathfinder_knowledge
pathfinder_civilization -> pathfinder_tribe
```

Avoid in P26:

```text
pathfinder_h5_dialog
pathfinder_agent_runtime
pathfinder_replay
pathfinder_pipeline
pathfinder_protocol
pathfinder_save
```

## Critical Rules

Civilization capability is not:

```text
bool unlocked
civilization_level
single success result
frontend-derived unlock
stage-derived unlock
config unlocked=true
personal knowledge only
```

Civilization capability must use:

```text
CapabilityMaturityState
CapabilityUsabilityState
CivilizationProgress
CapabilityEvidenceBundle
CapabilityRequirementSnapshot
```

## P25 Boundary

P26 may read only safe P25 outputs:

```text
ConflictResolutionResult
ConflictProjection
ConflictTrace
ConflictStateDraft
TribeRelationDraft
```

P26 must not:

```text
recompute conflict winner/loser
read enemy hidden state
read hidden truth
use P25 outcome as unconditional unlock
```

P25 outcomes are only evidence:

```text
TruceOffered -> conflict_deescalation evidence
ForcedRetreat with low loss -> organized_retreat evidence
Intimidated with low loss -> group_intimidation evidence
Standoff controlled -> resource_defense evidence
```

## Forbidden Unsafe Inputs

Reject keys and fields containing:

```text
raw_state
GameState
SaveGame
hidden_truth
true_trait
real_effect
true_hp
actual_hp
hp_delta
death
kill
loot
corpse
raw_damage
actual_damage
frontend_unlock
unlocked=true
civilization_level
```

## Determinism Rule

Same input + same definitions must produce same result.

No random unlock.

Do not depend on:

```text
system clock
unordered map iteration
frontend state
SaveGame side effects
AgentRuntime policy
```

Use deterministic sorting by:

```text
CapabilityType string
candidate_id
effect_key
deterministic_key
```

## Required Flow Stories

### Identify Edible Candidate

```text
Known edible knowledge exists.
Propagation coverage is not yet enough.
Result: IdentifyEdible Candidate or Emerging, not Stable.
```

### Safe Foraging Stable

```text
At least two edible knowledge items.
At least one danger knowledge item.
Propagation coverage >= 0.55.
Success rate >= 0.70.
Misunderstanding rate <= 0.20.
Result: SafeForaging Stable, effect drafts generated.
```

### Conflict Deescalation Candidate

```text
P25 summary has repeated TruceOffered or low-loss ForcedRetreat.
Open conflict pressure is controlled.
Result: ConflictDeescalation Candidate/Emerging.
No combat damage/death/loot data is read.
```

### Blocked But Not Lost

```text
SafeForaging Stable.
Conflict pressure or cohesion block is high.
Result: maturity remains Stable, usability becomes BlockedByConflict or BlockedByCohesion.
No Lost unless evidence is broken.
```

## Required Tests

Unit:

```text
civilization_stage_enum_roundtrip
capability_type_enum_roundtrip
capability_maturity_enum_roundtrip
capability_usability_enum_roundtrip
civilization_progress_validate
capability_condition_validate
capability_state_validate
capability_effect_draft_validate
civilization_state_validate
civilization_projection_safe_summary
civilization_trace_safe_summary
```

Integration:

```text
p26_identify_edible_candidate_flow
p26_identify_danger_candidate_flow
p26_safe_foraging_stable_flow
p26_conflict_deescalation_candidate_from_p25_flow
p26_capability_requires_repeated_evidence_flow
p26_stable_capability_emits_unlock_once_flow
p26_stage_foraging_from_stable_capabilities_flow
p26_blocked_by_conflict_does_not_lose_maturity_flow
p26_effect_draft_from_stable_usable_capability_flow
p26_deterministic_resolution_flow
```

Security:

```text
p26_security_rejects_unlocked_bool_shortcut
p26_security_rejects_stage_only_unlock
p26_security_rejects_frontend_unlock_source
p26_security_rejects_hidden_truth_keys
p26_security_rejects_raw_game_state_keys
p26_security_rejects_hp_death_kill_loot_keys
p26_security_rejects_direct_state_mutation_effect
p26_security_rejects_duplicate_capability_state
p26_security_rejects_test_only_enum_in_production
p26_security_rejects_single_success_as_stable
```

Commands:

```bash
cmake --build build/backend
ctest --test-dir build/backend -R "civilization|p26" --output-on-failure
ctest --test-dir build/backend -R "knowledge|propagation|tribe|p23|p24|p25|p26|civilization" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## Review Red Flags

Reject implementation if it:

```text
uses bool unlocked as authority
uses civilization_level as authority
allows config unlocked=true
unlocks from stage only
unlocks from a single success
lets frontend infer unlocks
reads raw GameState or SaveGame
reads hidden truth
recomputes P25 conflict results
outputs HP/death/kill/loot
mutates TribeState or WorldState directly
emits effects from Lost capability
emits normal effects from blocked capability
omits evidence for Stable capability
omits trace for capability transitions
```
