# P22 H5 Dialog Context Pack

## Purpose

P22 turns the P21 backend learning loop into a minimal Chinese H5 text game.

It is not a formal UI, not the final protocol layer, and not a new rule system.

The goal is to let a player open port 1999 on a phone and complete:

```text
observe -> try -> learn -> inspect knowledge -> teach companion -> inspect companion -> contradiction/revision
```

## Required Task Card

Read first:

```text
doc/48_P22H5中文对话框最小可玩任务卡设计.md
doc/47_P21学习闭环任务卡设计.md
doc/review/P21学习闭环修复说明.md
doc/39_项目完成总计划路线图.md
```

## Architecture

Namespace:

```cpp
namespace pathfinder::h5_dialog
```

Allowed dependency direction:

```text
h5_dialog -> learning -> cognition/memory/knowledge/foundation
h5_dialog -> foundation
```

Do not depend on:

```text
AgentRuntime
Policy
RulePipeline
SaveManager
replay raw records
formal protocol envelope
```

## Required Modules

Implement in this order:

```text
1. dialog_types + validateBasic
2. DialogIntentParser
3. DialogScenarioCatalog
4. DialogSessionStore
5. DialogLearningAdapter
6. DialogPresenter
7. DialogTurnService
8. DialogWireCodec
9. H5DialogServer
10. frontend/h5_dialog
```

## Core Rule

P22 must not produce knowledge conclusions.

Knowledge must come from P21:

```text
LearningLoopService::run
LearningDebugReportBuilder::buildReport
LearningLoopResult actor_claim_snapshot_after
LearningLoopResult recipient_claim_snapshot_after
```

The H5 page only displays `DialogResponseDto`.

## Required Objects

At least three cases must work through the same flow:

```text
red_berry + Eat -> restore_hunger

decayed_red_berry + Eat -> poison + condition decayed

bitter_leaf + Eat -> no_visible_effect
or
stone_flake + Use -> use_hint
```

Do not hardcode “red berry is edible” in frontend code.

## Dialog Inputs

Required Chinese mappings:

```text
观察 / 看看周围 -> Observe
吃红果 / 吃一口红果 -> TryEat(red_berry)
再吃一口 / 继续吃 -> RepeatLastAction
告诉同伴 / 教同伴 -> TeachRecipient
查看知识 / 我知道什么 -> InspectActorKnowledge
查看同伴 / 同伴知道什么 -> InspectRecipientKnowledge
吃腐烂红果 / 尝试腐烂红果 -> TryEat(decayed_red_berry)
吃苦叶 / 尝试苦叶 -> TryEat(bitter_leaf)
使用石片 / 用石片 -> TryUse(stone_flake)
帮助 / 指令 -> Help
重开 / 重新开始 -> Restart
```

## API Shape

P22 may use HTTP because it is the H5 entry stage.

First version:

```text
GET /
GET /style.css
GET /app.js
POST /api/dialog
```

Request:

```text
application/x-www-form-urlencoded
session_id=...
input_text=...
client_turn=...
```

Response:

```text
application/json; charset=utf-8
session_id
decision
reply_text
state_summary_lines
actor_knowledge_lines
recipient_knowledge_lines
quick_actions
debug_keys
warning_keys
```

Do not add WebSocket.
Do not add accounts.
Do not add cloud saves.
Do not introduce nlohmann/json; use fixed-field JSON encoding with tests.

## Session Requirements

Session state must retain:

```text
actor_memories
actor_summaries
actor_claims
recipient_claims
last_learning_intent
visible_object_keys
turn_index
```

After P21 result:

```text
actor_claims = result.actor_claim_snapshot_after
recipient_claims = result.recipient_claim_snapshot_after
append memory_write_result record when present
append memory_compression_result summary when present
```

Do not save to disk in P22.

## Security Forbidden Keys

Reject unsafe input/config/debug text containing:

```text
hidden_truth
real_effect
true_trait
object_truth
raw_state
edible_profile
hunger_delta
health_delta
effect_kind
GameState
AgentTickRecord
SaveGame
SaveManager
AgentRuntime
Policy
RulePipeline
```

Allowed only in forbidden-key tests and guard lists.

Frontend must use `textContent`, not `innerHTML`, for backend text.

## Tests Required

Unit tests:

```text
h5_dialog_smoke
h5_dialog_intent_parse_chinese
h5_dialog_scenario_catalog
h5_dialog_learning_adapter_valid_input
h5_dialog_presenter_chinese_response
h5_dialog_wire_codec_escape
```

Integration tests:

```text
p22_red_berry_learning_flow
p22_teach_companion_flow
p22_decayed_red_berry_revision_flow
p22_non_red_object_flow
p22_security_forbidden_input
```

Manual run:

```bash
./build/backend/pathfinder_h5_dialog_server --host 0.0.0.0 --port 1999 --static-root frontend/h5_dialog
```

Manual phone flow:

```text
观察
吃红果
查看知识
教同伴
查看同伴
吃腐烂红果
教同伴
查看同伴
使用石片
```

## Completion Definition

P22 passes only if:

```text
backend builds
P22 unit tests pass
P22 integration tests pass
full regression passes
phone can open H5 page on port 1999
all visible UI text is Chinese
red berry learning works
teaching works
correction/revision works
at least one non-red object works
frontend does not hardcode knowledge conclusions
h5_dialog does not read hidden truth/raw GameState/SaveGame/AgentRuntime
review report is written under doc/review
```
