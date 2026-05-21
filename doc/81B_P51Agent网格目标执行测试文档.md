# P51 Agent 网格目标执行测试文档

## 1. 测试目标

P51 测试必须证明：

```text
NPC 行动来自自身知识和目标推理。
NPC 行动通过 WorldCommand 管线执行。
NPC 不绕过背包、归属、资源节点、制作反应和学习链路。
NPC 能处理内部阻塞和外界打断。
```

## 2. 单元测试

### 2.1 world_agent_execution_enum_roundtrip

覆盖：

```text
WorldAgentExecutionDecision
WorldAgentExecutionFailureKind
WorldAgentGoalSourceKind
WorldAgentStepCommandKind
```

期望：

```text
toString/fromString 稳定。
Unknown 可解析。
BadValue 返回 error。
TestOnly 只用于测试。
```

### 2.2 context_builder_actor_missing

输入：actor_key 不存在。

期望：

```text
Result error 或 tick result failure=ActorMissing。
不调用 command pipeline。
```

### 2.3 context_builder_uses_actor_owner_knowledge_only

构造：

```text
player 拥有 gather 知识。
npc_001 没有任何知识。
```

期望：

```text
context.actor_claims 为空。
NPC 不能行动。
```

### 2.4 knowledge_guard_blocks_unknown_action

输入：NPC 没有 step 对应知识。

期望：

```text
Blocked / WaitingForKnowledge。
不生成 command。
```

### 2.5 knowledge_guard_allows_taught_action

输入：NPC 拥有 P50 传播后的 usable_for_action + usable_by_ai claim。

期望：

```text
允许执行。
matched claim owner == npc_001。
```

### 2.6 command_compiler_move_step

输入：MoveTo step。

期望：

```text
WorldCommandKind::Move。
source=AgentDecision。
actor_key 保持不变。
target_coord 正确。
```

### 2.7 command_compiler_gather_step

输入：Gather step。

期望：

```text
WorldCommandKind::Gather。
target_entity_id / node_id 进入 target 或安全 parameters。
不写执行结果到 parameters。
```

### 2.8 command_compiler_rejects_far_gather

输入：资源不相邻。

期望：

```text
不直接生成 Gather。
生成 Move command 或 LocationMismatch blocker。
```

### 2.9 execution_context_store_roundtrip

输入：保存 context，再读取。

期望：

```text
active goal、paused contexts、driver state 保持。
```

### 2.10 projection_no_hidden_truth

输入：tick result 带有隐藏诊断字段。

期望：

```text
projection 只输出 safe summary / trace key。
不输出 hidden_truth / raw_state / direct_state。
```

## 3. 集成测试

### 3.1 npc_without_knowledge_does_not_gather

流程：

```text
World 有 NPC 和可见 resource node。
玩家拥有知识，但 NPC 未被教学。
执行 Agent tick。
```

期望：

```text
result.decision=WaitingForKnowledge 或 CommandBlocked。
fake pipeline 调用次数=0。
世界状态不变。
```

### 3.2 taught_npc_issues_gather_command

流程：

```text
NPC 拥有可行动 gather 知识。
资源在相邻格。
执行 Agent tick。
```

期望：

```text
fake pipeline 收到 WorldCommandKind::Gather。
source=AgentDecision。
actor_key=npc_001。
result 合并 pipeline events/deltas/experiences。
```

### 3.3 far_resource_moves_before_gather

流程：

```text
资源距离 2 格。
NPC 有 gather 知识。
执行 tick。
```

期望：

```text
第一 tick 发 Move。
不发 Gather。
```

### 3.4 command_blocked_updates_execution_context

流程：

```text
fake pipeline 返回 Blocked。
```

期望：

```text
ExecutionContext 记录 blocker / attempt。
不标记目标完成。
```

### 3.5 missing_material_inserts_subgoal

流程：

```text
目标是 craft。
材料不足。
```

期望：

```text
InsertedSubGoal。
blocker.kind=MissingObject 或 InsufficientQuantity。
子目标为 AcquireObject。
不凭空生成材料。
```

### 3.6 interrupt_pauses_goal_and_inserts_emergency

流程：

```text
active goal=Gather。
输入 ExternalInterruptSignal ThreatAppeared。
```

期望：

```text
PausedForInterrupt。
paused context 存在。
新 emergency goal 入栈。
```

### 3.7 command_result_experiences_are_preserved

流程：fake pipeline 返回 experiences。

期望：

```text
WorldAgentTickResult.command_result.experiences 不为空。
P51 不删除 experiences。
```

### 3.8 beast_decision_source_supported

流程：actor capability = InstinctAgent / beast。

期望：

```text
command.source=BeastDecision。
不出现 companion 特判。
```

## 4. 架构扫描测试

### 4.1 no_h5_dependency_scan

扫描：

```bash
rg -n "h5|dialog|playable" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S
```

期望：无匹配。

### 4.2 no_content_hardcode_scan

扫描：

```bash
rg -n "red_berry|torch|wood|beast|wolf|mushroom|companion" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S
```

期望：无匹配。

### 4.3 no_direct_runtime_mutation_scan

扫描：

```bash
rg -n "\.actors\[|\.entities\[|\.resource_nodes\[|\.inventories\[|\.put\(|createOrUpdateGeneratedCell|upsertGeneratedResourceNode" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S
```

期望：

```text
P51 核心模块不能直接修改这些结构。
测试 fake/store 除外必须放在 tests 目录。
```

### 4.4 no_frontend_claim_injection_scan

扫描：

```bash
rg -n "recipient_claim_json|learned=true|force_learn|force_teach|knowledge_json" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S
```

期望：无匹配。

## 5. 必跑命令

```bash
cmake --build build/backend --target pathfinder_tests_world_agent_execution pathfinder_tests_world_agent_execution_integration -j 2
ctest --test-dir build/backend -R '^(world_agent_execution_|architecture_world_agent_execution_)' --output-on-failure
ctest --test-dir build/backend -R '^(world_command_|world_runtime_|world_inventory_|world_harvest_|world_reaction_|world_learning_|world_teaching_|agent_|goal_execution_|knowledge_)' --output-on-failure
```

## 6. 验收结论模板

实现完成后审核报告必须写：

```text
P51 是否通过。
是否所有 Agent 行动都走 command。
是否调用 P50 gate。
是否没有内容硬编码。
是否没有直接改 runtime / inventory。
测试命令和结果。
剩余非阻塞风险。
```
