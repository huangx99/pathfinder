# P27 Condition Expression Refactor Context Pack

## Purpose

P27 is a full condition-expression refactor for Pathfinder.

It is not the object reaction system. It must unify all condition judgment, condition normalization, condition trace, and condition display behind one backend authority.

After P27, legacy `condition_key` and `condition_keys` may remain as compatibility input, but business systems must not use raw string comparisons as rule authority.

## Required Reading

Read first:

```text
doc/54_P27受限条件表达式全量重构任务卡设计.md
doc/05_配置系统设计.md
doc/13_知识系统设计.md
doc/14_传播系统设计.md
doc/16_文明能力设计.md
doc/44_P18知识系统任务卡设计.md
doc/45_P19知识冲突与修正任务卡设计.md
doc/46_P20知识传播系统任务卡设计.md
doc/47_P21学习闭环任务卡设计.md
doc/53_P26文明能力任务卡设计.md
```

## Core Rule

All condition authority must go through:

```text
ConditionExpressionRef
ConditionEvaluationContext
ConditionExpressionEvaluator
ConditionEvaluationResult
ConditionEvaluationTrace
ConditionNormalizer
LegacyConditionAdapter
```

Forbidden after P27:

```text
business logic checks condition_key == "..."
business logic checks condition_keys contains "decayed"
H5 derives rules from condition strings
civilization unlocks directly from condition strings
knowledge revision ignores canonical condition keys
```

Allowed compatibility:

```text
legacy input DTOs may still carry condition_key / condition_keys
legacy adapter may map old keys
unit tests may construct legacy keys as fixtures
DTO validation may check non-empty fields without deciding gameplay rules
```

## New Module

Add target:

```text
pathfinder_condition
```

Expected files:

```text
backend/include/pathfinder/condition/condition_expression_types.h
backend/include/pathfinder/condition/condition_expression_context.h
backend/include/pathfinder/condition/condition_expression_registry.h
backend/include/pathfinder/condition/condition_expression_evaluator.h
backend/include/pathfinder/condition/condition_normalizer.h
backend/include/pathfinder/condition/condition_summary.h
backend/include/pathfinder/condition/legacy_condition_adapter.h
backend/src/condition/condition_expression_types.cpp
backend/src/condition/condition_expression_context.cpp
backend/src/condition/condition_expression_registry.cpp
backend/src/condition/condition_expression_evaluator.cpp
backend/src/condition/condition_normalizer.cpp
backend/src/condition/condition_summary.cpp
backend/src/condition/legacy_condition_adapter.cpp
```

## Core Types

Implement at minimum:

```text
ConditionExpressionRef
ConditionExpressionDefinition
ConditionExpressionNode
ConditionExpressionNodeKind
ConditionExpressionOperator
ConditionExpressionValue
ConditionEvaluationContext
ObjectConditionView
KnowledgeConditionView
TribeConditionView
CivilizationConditionView
ConflictConditionView
ConditionEvaluationResult
ConditionEvaluationTrace
NormalizedCondition
ConditionExpressionRegistry
ConditionExpressionEvaluator
ConditionNormalizer
LegacyConditionAdapter
```

## Supported First Version

Support:

```text
and
or
not
eq
neq
gt
gte
lt
lte
contains
has_tag
has_state
has_capability
knowledge_has_claim
```

Do not support:

```text
loops
recursion
variable assignment
custom user functions
random numbers
state mutation
event creation
raw GameState
SaveGame
hidden truth
```

## Legacy Key Mappings

At minimum:

```text
fresh -> condition:object_state:eq:fresh
state_fresh -> condition:object_state:eq:fresh
decayed -> condition:object_state:eq:decayed
state_decayed -> condition:object_state:eq:decayed
cooked -> condition:object_state:eq:cooked
raw -> condition:object_state:eq:raw
wet -> condition:object_state:eq:wet
dry -> condition:object_state:eq:dry
has_tool -> condition:actor_requirement:has:tool
taught -> condition:knowledge_source:eq:taught
observed -> condition:knowledge_source:eq:observed
```

Unknown legacy keys must fail unless explicitly test-only.

## Mandatory Refactor Points

Refactor:

```text
KnowledgeClaim condition comparison
KnowledgeRevision condition conflict checks
LearningLoop feedback.condition_keys handling
KnowledgePropagationContext.condition_keys handling
FeedbackResolver condition handling
Civilization capability conditions
H5 dialog knowledge key/display condition logic
```

If P26 implementation is not present yet, P26 must be implemented directly against P27 condition APIs.

## Regression Scenarios

Must pass:

```text
eat fresh red berry -> fresh red berry restore hunger knowledge
eat decayed red berry -> decayed red berry poison knowledge
eat red berry, eat decayed red berry, eat red berry -> decayed knowledge is not lost
repeating decayed red berry does not create infinite duplicate claims
teach companion preserves fresh and decayed conditional knowledge
H5 display shows Chinese condition summaries instead of raw debug keys
civilization capability conditions evaluate through ConditionExpressionEvaluator
```

## Tests

Add tests:

```text
backend/tests/unit/condition/condition_expression_types_test.cpp
backend/tests/unit/condition/condition_expression_evaluator_test.cpp
backend/tests/unit/condition/condition_normalizer_test.cpp
backend/tests/integration/p27/condition_migration_flow_test.cpp
backend/tests/integration/p27/condition_gate_scan_test.cpp
```

Run at least:

```text
cmake --build build/backend
ctest --test-dir build/backend -R "condition|p27" --output-on-failure
ctest --test-dir build/backend -R "knowledge|learning|propagation|feedback|civilization|h5_dialog" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## Acceptance

P27 passes only when:

```text
condition module exists
legacy condition input normalizes through ConditionNormalizer
business systems no longer use raw condition_key as rule authority
knowledge/learning/propagation/feedback/civilization/H5 are connected
red berry / decayed red berry regressions pass
condition gate scan passes
P28 can directly use ConditionExpressionRef
```
