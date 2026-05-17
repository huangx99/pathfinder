# P19 Knowledge Revision Context Pack

## Purpose

P19 implements knowledge conflict and revision after P18 knowledge formation.

It turns static subjective `KnowledgeClaim` objects into updateable knowledge that can be reinforced, weakened, specialized, deprecated, or disproven by new safe evidence from P17 `MemorySummary` and P16 `MemoryRecord` references.

## Required Reading

- `doc/45_P19知识冲突与修正任务卡设计.md`
- `doc/44_P18知识系统任务卡设计.md`
- `doc/13_知识系统设计.md`
- `context_packs/knowledge_p18.md`

## Core Scope

Implement:

```text
KnowledgeConflictType
KnowledgeRevisionDecision
KnowledgeResolutionStrategy
KnowledgeRevisionOptions
KnowledgeConflictSignal
KnowledgeRevisionInput
KnowledgeRevisionAssessment
KnowledgeClaimPatch
KnowledgeRevisionPlan
KnowledgeRevisionResult
KnowledgeRevisionPlanner
KnowledgeRevisionService
```

Update existing P18 components:

```text
KnowledgeClaim::validateBasic
KnowledgeRepository / KnowledgeQuery
KnowledgeProjectionBuilder
KnowledgeFormationPlan validation remains P18-safe
```

## Core Rules

- P19 can create `Deprecated` and `Disproven` KnowledgeClaim states.
- P18 formation must still never create `Deprecated` or `Disproven`.
- `KnowledgeClaim::validateBasic()` may allow Deprecated/Disproven only with strict evidence/reason requirements.
- Repository may store Deprecated/Disproven.
- Repository default query must exclude Deprecated/Disproven.
- Ordinary Projection must never output Deprecated/Disproven items.
- Revision service must not directly write Repository.
- No `Result::ok(dto)` may return a DTO that fails `validateBasic()`.

## Forbidden

Do not implement:

```text
Knowledge propagation
Tribe knowledge merge
Civilization unlock
H5/API/HTTP/JSON
SaveManager
RL reward/done
AgentRuntime/Policy direct writes
Debug projection history UI
```

Do not read:

```text
ObjectDefinition hidden truth
GameState raw state
AgentTickRecord raw data
SaveGame
```

## Revision Strategy

Priority:

```text
supporting evidence -> Reinforce
weak contradiction -> Weaken
condition mismatch -> Specialize or KeepBoth
overgeneralized old claim -> Deprecated
same-condition strong contradiction -> Disproven
```

Never disprove when condition mismatch can explain the conflict.

## Tests

Run:

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "knowledge|p18|p19" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scan:

```bash
rg -n "ObjectDefinition hidden|edible_profile|hunger_delta|health_delta|effect_kind|true_trait|real_effect|object_truth|raw_state|hidden_truth|GameState|AgentTickRecord|SaveGame|SaveManager" backend/include/pathfinder/knowledge backend/src/knowledge backend/tests/integration/p19 backend/tests/unit/knowledge
rg -n "Propagation|HTTP|WebSocket|nlohmann|json|SaveManager|reward_value|done\s*=|is_done|AgentRuntime|Policy|RulePipeline|GameState" backend/include/pathfinder/knowledge backend/src/knowledge backend/tests/integration/p19 backend/tests/unit/knowledge
```
