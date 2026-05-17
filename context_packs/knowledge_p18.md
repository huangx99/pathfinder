# P18 Knowledge System Context Pack

## Goal

Implement the first knowledge formation layer after P17 memory compression and before P19 knowledge conflict/revision.

P18 turns stable, traceable `MemorySummary` projections into subjective `KnowledgeClaim` objects. A claim is a structured, queryable, evidence-backed statement owned by an agent/actor/tribe. It is not hidden truth and must not read ObjectDefinition hidden fields or raw GameState.

## Must Read

- `doc/44_P18知识系统任务卡设计.md`
- `doc/13_知识系统设计.md`
- `doc/43_P17记忆压缩与检索任务卡设计.md`
- `doc/42_P16记忆系统基础任务卡设计.md`
- `context_packs/memory_p17.md`
- `backend/include/pathfinder/memory/memory_summary.h`
- `backend/include/pathfinder/memory/memory_record.h`
- `backend/include/pathfinder/foundation/id.h`
- `backend/include/pathfinder/foundation/result.h`
- `backend/include/pathfinder/foundation/error.h`

## Scope

Add:

```text
backend/include/pathfinder/knowledge/knowledge_types.h
backend/include/pathfinder/knowledge/knowledge_claim.h
backend/include/pathfinder/knowledge/knowledge_repository.h
backend/include/pathfinder/knowledge/knowledge_formation.h
backend/include/pathfinder/knowledge/knowledge_projection.h
backend/src/knowledge/knowledge_types.cpp
backend/src/knowledge/knowledge_claim.cpp
backend/src/knowledge/knowledge_repository.cpp
backend/src/knowledge/knowledge_formation.cpp
backend/src/knowledge/knowledge_projection.cpp
backend/tests/unit/knowledge/knowledge_types_test.cpp
backend/tests/unit/knowledge/knowledge_claim_test.cpp
backend/tests/unit/knowledge/knowledge_repository_test.cpp
backend/tests/unit/knowledge/knowledge_formation_test.cpp
backend/tests/unit/knowledge/knowledge_projection_test.cpp
backend/tests/integration/p18/knowledge_from_memory_summary_flow_test.cpp
backend/tests/integration/p18/knowledge_boundary_security_test.cpp
backend/tests/integration/p18/knowledge_projection_flow_test.cpp
```

Modify:

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

## Namespace

```cpp
namespace pathfinder::knowledge
```

## Dependencies Allowed

```text
pathfinder::foundation
pathfinder::event
pathfinder::cognition
pathfinder::memory
```

Do not depend on AgentRuntime, Policy, RulePipeline execution, raw GameState, SaveManager, HTTP/WebSocket/JSON, propagation, conflict revision.

## Required Core Types

- `KnowledgeOwner`
- `KnowledgeSubject`
- `KnowledgePredicate`
- `KnowledgeCondition`
- `KnowledgeEvidence`
- `KnowledgeConfidence`
- `KnowledgeTeachingProfile`
- `KnowledgeProjectionFlags`
- `KnowledgeClaim`
- `KnowledgeFormationOptions`
- `KnowledgeFormationInput`
- `KnowledgeFormationPlan`
- `KnowledgeFormationResult`
- `KnowledgeQuery`
- `KnowledgeProjectionItem`
- `KnowledgeProjection`
- `KnowledgeEventDraft`
- `KnowledgeStateChangeDraft`

Every DTO needs `validateBasic()`.

## Required Services

- `KnowledgeIdFactory`
- `KnowledgeRepository`
- `KnowledgeFormationPlanner`
- `KnowledgeFormationService`
- `KnowledgeProjectionBuilder`

## Required Enums

- `KnowledgeOwnerKind`
- `KnowledgeSubjectKind`
- `KnowledgeRelationType`
- `KnowledgeConfidenceBand`
- `KnowledgeStatus`
- `KnowledgeEvidenceKind`
- `KnowledgeFormationDecision`
- `KnowledgeQueryMode`

Each enum requires:

```cpp
std::string toString(Enum value);
Result<Enum> enumFromString(const std::string& value);
```

Unknown strings must return `Result::fail`.

## Core Rules

- P18 knowledge is subjective, not ObjectDefinition truth.
- P18 forms claims only from safe evidence, primarily P17 `MemorySummary`.
- `KnowledgeClaim` must keep source summary IDs, source memory IDs, and memory evidence refs.
- `KnowledgeFormationPlan` is only an executable plan; insufficient evidence must return fail, not ok with invalid DTO.
- `KnowledgeFormationService` may convert insufficient evidence into a valid `Skipped` result.
- Repository query order must be deterministic.
- Projection must not expose full evidence internals by default.

## Forbidden In P18

Do not implement:

```text
KnowledgeConflictResolver
KnowledgeRevision
KnowledgeMergePolicy
Propagation
H5/API/HTTP/JSON
SaveManager
RL reward/done
AgentRuntime/Policy direct writes
```

Do not read:

```text
ObjectDefinition hidden truth
GameState raw state
AgentTickRecord raw data
SaveGame
```

## Tests

Run:

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "knowledge|p18" --output-on-failure
ctest --test-dir build/backend -R "knowledge|p18|memory|p17|p16|p15|cognition|p14|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol|pipeline|state" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scan:

```bash
rg -n "Propagation|HTTP|WebSocket|nlohmann|json|SaveManager|reward_value|done\s*=|is_done|AgentRuntime|Policy|RulePipeline|GameState" backend/include/pathfinder/knowledge backend/src/knowledge backend/tests/integration/p18 backend/tests/unit/knowledge
```

Expected: no substantive hits except hidden-truth blacklist or security tests.
