# P50 教学与无知 NPC 测试文档

## 1. 测试目标

P50 测试必须证明：

```text
知识能从一个 owner 传播到另一个 owner。
传播不是复制粘贴。
NPC 的知识可以被查询和用于基础行动门禁。
失败情况不会污染 repository。
```

## 2. 单元测试

### 2.1 world_teaching_enum_roundtrip

覆盖：

```text
WorldTeachingFailureKind
WorldTeachingDecision
NpcActionKnowledgeGateDecision
```

要求：

```text
toString / fromString 稳定。
Unknown 可解析。
BadValue 返回 error。
TestOnly 只在测试中允许。
```

### 2.2 teaching_owner_resolver_maps_actor

输入：

```text
actor_key = npc_001
```

输出：

```text
KnowledgeOwner.kind = Actor
KnowledgeOwner.entity_id = npc_001
```

### 2.3 eligibility_rejects_missing_source_claim

教师 repository 没有该 knowledge id。

期望：

```text
allowed=false
failure=SourceClaimMissing
repository 不变
```

### 2.4 eligibility_rejects_owner_mismatch

source claim 存在，但 owner 是别的 actor。

期望：

```text
failure=SourceClaimOwnerMismatch
```

### 2.5 eligibility_rejects_not_teachable

claim.teaching_profile.teachable=false。

期望：

```text
failure=ClaimNotTeachable
```

### 2.6 eligibility_rejects_out_of_range

fake actor query port 返回：

```text
teacher=(0,0,surface)
recipient=(5,0,surface)
max_distance=2
```

期望：

```text
failure=OutOfRange
```

### 2.7 teaching_loop_creates_recipient_claim

教师拥有可教学 claim，接收者无 claim。

期望：

```text
result.ok=true
recipient_created_claims.size >= 1
created.owner == recipient owner
created.related_knowledge_ids 包含 source knowledge id 或 propagation 关系可追踪
```

### 2.8 teaching_existing_claim_reinforces_or_updates

接收者已有相似 claim。

期望：

```text
不产生重复垃圾 claim
结果为 ReinforcedExistingClaim / RevisedExistingClaim / SkippedAlreadyKnown 之一
```

### 2.9 action_gate_blocks_unknown_npc

NPC 没有任何 claim。

请求：

```text
subject=wood
action=craft
effect=make_torch
```

期望：

```text
allowed=false
decision=BlockedNoKnowledge
```

### 2.10 action_gate_allows_known_action

NPC 有 usable_for_action claim。

期望：

```text
allowed=true
decision=AllowedByKnowledge
matched_claim 有值
```

### 2.11 action_gate_blocks_risk_without_goal

NPC 有毒蘑菇中毒知识。

请求主动 eat toxic_mushroom，allow_risk_action=false。

期望：

```text
allowed=false
decision=BlockedDangerWithoutGoal
```

## 3. 集成测试

### 3.1 teach_command_player_to_npc

构造：

```text
WorldCommandKind::Teach
actor_key=player
target_actor_key=npc_001
target.knowledge_key=source claim id
```

期望：

```text
WorldCommandExecutionResult.result_kind=Succeeded
events 包含 knowledge_taught
state_deltas 包含 recipient_knowledge_delta
projection_patch.changed_knowledge actor_key=npc_001
repository 中 npc_001 owner 有 claim
```

### 3.2 teach_command_npc_to_player

同一 handler，teacher=npc_001，recipient=player。

期望：

```text
玩家获得自己的 claim
没有 player 特判
```

### 3.3 teach_command_out_of_range_no_patch

距离过远。

期望：

```text
result_kind=Blocked
failure_reason_keys 包含 OutOfRange
projection_patch.changed_knowledge 为空
repository 不变
```

### 3.4 teach_then_action_gate

流程：

```text
玩家拥有采集知识。
玩家教 NPC。
查询 NpcBasicActionKnowledgeGate。
```

期望：

```text
教学前 gate=BlockedNoKnowledge
教学后 gate=AllowedByKnowledge
```

### 3.5 no_h5_dependency_scan

命令：

```bash
rg -n "h5|dialog|playable" backend/include/pathfinder/world_teaching backend/src/world_teaching -S
```

期望无结果。

### 3.6 no_content_hardcode_scan

命令：

```bash
rg -n "red_berry|torch|wood|beast|wolf|mushroom" backend/include/pathfinder/world_teaching backend/src/world_teaching -S
```

期望生产代码无结果。测试代码允许使用测试内容。

## 4. 回归测试

P50 完成后必须运行：

```bash
cmake --build build/backend --target pathfinder_tests_world_teaching pathfinder_tests_world_teaching_integration -j 2
ctest --test-dir build/backend -R '^world_teaching_' --output-on-failure
```

再运行相关回归：

```bash
cmake --build build/backend -j 2
ctest --test-dir build/backend -R '^(world_command_|world_learning_|learning_|knowledge_|memory_|cognition_)' --output-on-failure
```

## 5. 审核门禁

以下任一情况直接不通过：

```text
生产代码写死 companion。
生产代码写死 torch/berry/beast 等内容。
教学不走 LearningLoop 或 KnowledgePropagation。
recipient claim owner 仍是 teacher。
失败教学仍写入 repository。
前端可以传 recipient claim 内容。
缺少距离测试。
缺少反向教学测试。
缺少行动门禁测试。
```
