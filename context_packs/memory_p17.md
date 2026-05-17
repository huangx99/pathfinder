# P17 Memory Compression And Recall Context Pack

## Goal

Implement the memory compression and recall layer after P16 memory foundation and before P18 knowledge.

P17 turns many safe `MemoryRecord` objects into traceable `MemorySummary` projections and provides deterministic recall over raw records and summaries. It must reduce context pressure without deleting original memories or creating knowledge claims.

## Must Read

- `doc/43_P17记忆压缩与检索任务卡设计.md`
- `doc/42_P16记忆系统基础任务卡设计.md`
- `doc/12_记忆系统设计.md`
- `doc/41_P15认知系统正式化任务卡设计.md`
- `context_packs/memory_p16.md`
- `backend/include/pathfinder/memory/types.h`
- `backend/include/pathfinder/memory/memory_record.h`
- `backend/include/pathfinder/memory/memory_store.h`
- `backend/include/pathfinder/foundation/id.h`
- `backend/include/pathfinder/foundation/result.h`
- `backend/include/pathfinder/foundation/time.h`

## Scope

Add:

```text
backend/include/pathfinder/memory/memory_summary.h
backend/include/pathfinder/memory/memory_compression.h
backend/include/pathfinder/memory/memory_recall.h
backend/src/memory/memory_summary.cpp
backend/src/memory/memory_compression.cpp
backend/src/memory/memory_recall.cpp
backend/tests/unit/memory/memory_summary_test.cpp
backend/tests/unit/memory/memory_compression_test.cpp
backend/tests/unit/memory/memory_recall_test.cpp
backend/tests/integration/p17/memory_compression_flow_test.cpp
backend/tests/integration/p17/memory_recall_flow_test.cpp
backend/tests/integration/p17/memory_boundary_security_test.cpp
```

Modify:

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

## Namespace

```cpp
namespace pathfinder::memory
```

## Dependencies Allowed

```text
pathfinder::foundation
pathfinder::event
pathfinder::cognition
pathfinder::memory P16 types
```

Do not depend on AgentRuntime, Policy, RulePipeline, raw GameState, Knowledge, SaveManager, HTTP/WebSocket/JSON.

## Required ID

Define inside memory layer, not foundation:

```cpp
struct MemorySummaryIdTag {};
using MemorySummaryId = pathfinder::foundation::StrongId<MemorySummaryIdTag>;
```

## Required Enums

- `MemorySummaryKind`
- `MemoryCompressionLevel`
- `MemoryCompressionDecision`
- `MemoryRecallMode`
- `MemoryRecallSort`
- `MemoryRecallItemKind`

Each enum requires:

```cpp
std::string toString(Enum value);
Result<Enum> enumFromString(const std::string& value);
```

Unknown strings must return `Result::fail`.

## Required DTOs

- `MemorySummaryKey`
- `MemorySummary`
- `MemoryCompressionOptions`
- `MemoryCompressionPlan`
- `MemoryCompressionResult`
- `MemoryRecallQuery`
- `MemoryRecallItem`
- `MemoryRecallResult`

Every DTO needs `validateBasic()`.

## Required Services

- `MemorySummaryIdFactory`
- `MemorySummaryStore`
- `MemoryCompressionPlanner`
- `MemoryCompressionService`
- `MemoryRecallService`

## Core Rules

- P17 compression is a safe projection, not destructive compaction.
- Never delete original `MemoryRecord` in P17.
- `MemorySummary` must keep non-empty `source_memory_ids`.
- `MemorySummary` must keep non-empty `representative_memory_ids`.
- `MemorySummary` must keep evidence refs or source traceability.
- Default compression uses owner + subject + summary kind grouping.
- Default compression skips `ShortTerm` and `Forgotten` records.
- `Protected` records can participate in summaries, but must not be deleted.
- Recall can return records, summaries, or both.
- Recall sorting must be deterministic with ID tie-break.
- P17 can prepare evidence candidates for P18, but must not create knowledge.

## Required Recall Modes

- `ExactSubject`
- `OwnerRecent`
- `ByMemoryKind`
- `ImportantOrCritical`
- `TeachingCandidates`
- `RiskRelated`
- `ContradictionRelated`
- `LongTermOnly`

## Forbidden In P17

Do not implement:

```text
KnowledgeClaim
KnowledgeRepository
KnowledgeEvidence
KnowledgeConfidence
KnowledgeConflict
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

## Hidden Truth Guard

Reuse P16:

```cpp
memoryForbiddenKeys()
containsMemoryForbiddenKey(...)
```

Must scan summary keys, summaries, compression plans, recall queries, recall items, and recall results.

## Tests

Run:

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "memory|p17" --output-on-failure
ctest --test-dir build/backend -R "memory|p17|p16|p15|cognition|p14|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol|pipeline|state" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scan:

```bash
rg -n "KnowledgeClaim|KnowledgeRepository|Propagation|HTTP|WebSocket|nlohmann|json|SaveManager|reward_value|done\s*=|is_done|AgentRuntime|Policy|RulePipeline|GameState" backend/include/pathfinder/memory backend/src/memory backend/tests/integration/p17
```

Expected: no substantive hits except hidden-truth blacklist or security tests.
