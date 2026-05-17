# P16 Memory System Context Pack

## Goal

Implement the foundation memory layer after P15 cognition and before P17 compression/recall.

P16 converts safe P15 cognition outputs into short/mid/long term memory records. It supports deterministic writing, reinforcement, promotion, decay, forgetting, protection, basic in-memory storage, basic query, event drafts, and state change drafts.

## Must Read

- `doc/42_P16记忆系统基础任务卡设计.md`
- `doc/12_记忆系统设计.md`
- `doc/41_P15认知系统正式化任务卡设计.md`
- `context_packs/cognition_p15.md`
- `backend/include/pathfinder/cognition/cognition_v2_types.h`
- `backend/include/pathfinder/foundation/id.h`
- `backend/include/pathfinder/foundation/result.h`
- `backend/include/pathfinder/foundation/time.h`

## Scope

Add:

```text
backend/include/pathfinder/memory/types.h
backend/include/pathfinder/memory/memory_record.h
backend/include/pathfinder/memory/memory_store.h
backend/include/pathfinder/memory/memory_writer.h
backend/include/pathfinder/memory/memory_decay.h
backend/include/pathfinder/memory/memory_events.h
backend/src/memory/types.cpp
backend/src/memory/memory_record.cpp
backend/src/memory/memory_store.cpp
backend/src/memory/memory_writer.cpp
backend/src/memory/memory_decay.cpp
backend/src/memory/memory_events.cpp
backend/tests/unit/memory/memory_types_test.cpp
backend/tests/unit/memory/memory_record_test.cpp
backend/tests/unit/memory/memory_store_test.cpp
backend/tests/unit/memory/memory_writer_test.cpp
backend/tests/unit/memory/memory_decay_test.cpp
backend/tests/integration/p16/memory_from_cognition_flow_test.cpp
backend/tests/integration/p16/memory_decay_flow_test.cpp
backend/tests/integration/p16/memory_boundary_security_test.cpp
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
```

Do not depend on AgentRuntime, Policy, SaveManager, HTTP/WebSocket/JSON, or raw GameState.

## Required Enums

- `MemoryOwnerKind`
- `MemorySubjectKind`
- `MemoryKind`
- `MemoryScope`
- `MemoryLifecycle`
- `MemoryStrengthBand`
- `MemoryImportance`
- `MemorySourceKind`
- `MemoryWriteDecision`
- `MemoryDecayDecision`
- `MemoryRejectReason`

Each enum requires:

```cpp
std::string toString(Enum value);
Result<Enum> enumFromString(const std::string& value);
```

Unknown strings must return `Result::fail`.

## Required DTOs

- `MemoryOwner`
- `MemorySubject`
- `MemoryEvidenceRef`
- `MemoryCandidate`
- `MemoryRecord`
- `MemoryWriteOptions`
- `MemoryDecayOptions`
- `MemoryWriteIssue`
- `MemoryWriteResult`
- `MemoryDecayResult`
- `MemoryQuery`
- `MemoryQueryResult`
- `MemoryEventDraft`
- `MemoryStateChangeDraft`

Every DTO needs `validateBasic()`.

## Required Services

- `MemoryCandidateFactory`
- `MemoryIdFactory`
- `MemoryStore`
- `MemoryWriteService`
- `MemoryDecayService`
- `MemoryStateChangeBuilder`
- `MemoryEventBuilder`

## Core Rules

- First safe cognition evidence creates a `ShortTerm` memory.
- Repeated similar evidence reinforces an existing memory.
- Repeated or strong memory promotes `ShortTerm -> MidTerm`.
- Critical/high-risk memory can become protected and promote toward `LongTerm`.
- Normal `ShortTerm` memory decays with ticks.
- `Protected` memory does not decay.
- `LongTerm` does not decay by default.
- `Forgotten` memory is excluded from normal query by default.
- P16 only does basic query; P17 handles compression and recall.

## Forbidden In P16

Do not implement:

```text
MemorySummary
MemoryBundle
MemoryCompressionPolicy
MemoryRecallQuery
MemoryRecallResult
MemoryDecompressionView
KnowledgeClaim
KnowledgeRepository
Propagation
H5/API/HTTP/JSON
SaveManager
RL reward/done
```

Do not read:

```text
ObjectDefinition hidden truth
GameState raw state
AgentTickRecord raw data
SaveGame
```

## Hidden Truth Guard

Implement:

```cpp
std::vector<std::string> memoryForbiddenKeys();
bool containsMemoryForbiddenKey(const std::string& text);
bool containsMemoryForbiddenKey(const std::vector<std::string>& texts);
```

Must reject at least:

```text
ObjectDefinition hidden
edible_profile
hunger_delta
health_delta
effect_kind
GameState
AgentTickRecord
reward_value
done =
is_done
SaveGame
SaveManager
raw_state
hidden_truth
```

## Tests

Run:

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "memory|p16" --output-on-failure
ctest --test-dir build/backend -R "cognition|memory|p16|p15|p14|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scan:

```bash
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|SaveManager|httplib|asio|WebSocket|HTTP server|nlohmann|json|socket|send\(|recv\(" backend/include/pathfinder/memory backend/src/memory backend/tests/integration/p16
```

Expected: no hits.
