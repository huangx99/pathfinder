# P21 Learning Loop Context Pack

## Purpose

P21 implements the backend minimal playable learning loop for Pathfinder.

It composes P15-P20 into one auditable story:

```text
safe feedback -> cognition -> memory -> compression/recall -> knowledge formation -> revision -> propagation -> recipient influence
```

P21 is an orchestration layer. It must not rewrite cognition, memory, knowledge, revision, or propagation rules.

## Required Reading

- `doc/47_P21学习闭环任务卡设计.md`
- `doc/41_P15认知系统正式化任务卡设计.md`
- `doc/42_P16记忆系统基础任务卡设计.md`
- `doc/43_P17记忆压缩与检索任务卡设计.md`
- `doc/44_P18知识系统任务卡设计.md`
- `doc/45_P19知识冲突与修正任务卡设计.md`
- `doc/46_P20知识传播系统任务卡设计.md`
- `doc/review/P20知识传播系统复查报告.md`
- `context_packs/cognition_p15.md`
- `context_packs/memory_p16.md`
- `context_packs/memory_p17.md`
- `context_packs/knowledge_p18.md`
- `context_packs/knowledge_p19.md`
- `context_packs/knowledge_p20.md`

## Core Scope

Add:

```text
backend/include/pathfinder/learning/learning_loop.h
backend/src/learning/learning_loop.cpp
backend/tests/unit/learning/learning_loop_test.cpp
backend/tests/integration/p21/learning_loop_discovery_story_test.cpp
backend/tests/integration/p21/learning_loop_revision_story_test.cpp
backend/tests/integration/p21/learning_loop_security_test.cpp
```

Modify:

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

Implement:

```text
LearningLoopStoryKind
LearningLoopStage
LearningLoopDecision
LearningLoopFailureReason
LearningStepDecision
LearningActorRef
LearningSafeFeedbackInput
LearningLoopOptions
LearningLoopInput
LearningEventChain
LearningLoopStepTrace
LearningLoopResult
LearningDebugReport
LearningLoopPlanner
LearningLoopService
LearningDebugReportBuilder
LearningLoopApplier
```

`LearningEventChain` and `LearningDebugReport` are required roadmap deliverables, not optional debug extras. They must contain only ids and safe keys, not hidden truth or large raw objects.

## Architecture Rule

P21 composes existing services:

```text
P15 cognition services
P16 memory writer
P17 memory compression/recall
P18 knowledge formation/projection
P19 knowledge revision
P20 knowledge propagation
```

Do not duplicate their internal logic inside `learning`.

## Forbidden

Do not implement:

```text
H5/API/HTTP/WebSocket/JSON
SaveManager
tribe politics/morale/split/combat
civilization unlocks
object reactions
RL reward/done
AgentRuntime/Policy authority
free-text AI knowledge generation
```

Do not read:

```text
ObjectDefinition hidden truth
GameState raw state
AgentTickRecord raw data
SaveGame
```

Do not use or accept unsafe keys:

```text
hidden_truth
real_effect
true_trait
object_truth
raw_state
edible_profile
hunger_delta
health_delta
```

## Required P20 Integration

P20復查 confirmed this behavior:

```text
same subject/action/relation + same effect -> reinforce existing recipient claim
same subject/action/relation + different effect -> should_route_to_revision=true
```

P21 must consume this route signal.

If any `KnowledgeTransferAssessment` has:

```cpp
assessment.should_route_to_revision == true
```

then P21 must:

```text
add a KnowledgeRevised trace
route recipient knowledge through P19 revision or return a clear RoutedToRevision business result
not create a normal duplicate recipient claim
not mark propagation as fully completed without revision trace
```

## Minimum Gameplay Stories

### Unknown Red Berry Discovery

```text
A eats red_berry.
Safe feedback says NeedImproved and effect_key=restore_hunger.
A forms cognition, memory, memory summary, and knowledge claim.
A teaches B.
B receives a Teaching-derived subjective claim.
B projection can show red_berry is edible/useful.
```

Must verify:

```text
recipient owner is B
recipient evidence includes Teaching
claim is not copied from A as truth
all ok DTOs validateBasic
```

### Decayed Red Berry Correction

```text
A previously knows red_berry + eat -> restore_hunger.
A receives new feedback: decayed red_berry + eat -> poison.
P19 revises or specializes A knowledge.
A propagates corrected knowledge to B.
If B has conflicting old knowledge, P20 routes to revision.
P21 consumes that route.
```

Must verify:

```text
KnowledgeRevised trace exists
condition key such as decayed is preserved
conflicting active recipient claims are not silently duplicated
```

### Low Confidence Teaching Trace

```text
Low-confidence or wrong knowledge may be taught.
Recipient claim must remain low confidence and traceable to Teaching.
Later direct contradiction can enter P19 revision.
```

## Validation Commands

Run:

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "learning|p21|cognition|memory|knowledge|p20" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

Boundary scan:

```bash
rg -n "ObjectDefinition hidden|edible_profile|hunger_delta|health_delta|effect_kind|true_trait|real_effect|object_truth|raw_state|hidden_truth|GameState|AgentTickRecord|SaveGame|SaveManager" backend/include/pathfinder/learning backend/src/learning backend/tests/integration/p21 backend/tests/unit/learning
rg -n "HTTP|WebSocket|nlohmann|json|SaveManager|reward_value|done\s*=|is_done|AgentRuntime|Policy|RulePipeline|GameState" backend/include/pathfinder/learning backend/src/learning backend/tests/integration/p21 backend/tests/unit/learning
```

Only explicit security rejection tests or comments should match these scans.

## Implementation Order

Use small steps:

```text
1. DTO/enums/string conversion/validate tests.
2. Planner and security boundary tests.
3. Service chain through cognition-memory-compression-recall-formation.
4. Propagation and recipient projection.
5. Revision and P20 route-to-revision handling.
6. Report builder and stable trace output.
```

## Acceptance

P21 passes only if:

```text
backend learning module compiles
P21 unit tests pass
P21 integration story tests pass
P15-P20 tests do not regress
full ctest passes
hidden truth/raw state scans are clean or explicitly justified
P20 route_to_revision is handled
```

After P21, the project has a backend minimal playable learning loop even without H5.
