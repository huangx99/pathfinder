# P50 教学与无知 NPC 基础行动审核报告

## 1. 审核结论

结论：通过。

P50 已按设计接入“教学传播”和“无知 NPC 单步行动知识门禁”：

- 教学通过 `WorldCommandKind::Teach` 进入后端，不依赖旧 H5。
- 教学者和接收者都使用 `KnowledgeOwner`，接收者获得自己的 `KnowledgeClaim`。
- 教学必须经过 source claim 存在、owner 匹配、可教学、同层距离等校验。
- 传播使用 `KnowledgePropagationService` + `KnowledgePropagationApplier`，不是手写复制 claim。
- 教学结果输出 `events`、`state_deltas`、`projection_patch.changed_knowledge`。
- NPC 行动门禁只允许拥有可行动且 AI 可用知识的 NPC 通过。

## 2. 审核中修复的问题

### 2.1 NPC 门禁缺少 `usable_by_ai` 校验

问题：原实现只检查 `usable_for_action`，如果某条知识可展示为行动知识但不允许 AI 使用，NPC 仍可能通过门禁。

修复：`NpcBasicActionKnowledgeGate` 现在同时要求：

```text
projection_flags.usable_for_action == true
projection_flags.usable_by_ai == true
```

新增测试：

```text
world_teaching_action_gate_blocks_ai_disabled_claim
```

### 2.2 Teach handler 缺少 actor query port 空指针防护

问题：如果运行时未注入 `IWorldActorQueryPort`，handler 会在执行时解引用空指针。

修复：空 port 直接返回 `Blocked + InvalidRequest`，并且不写入 repository。

新增测试：

```text
world_teaching_command_missing_actor_query_port_blocks
```

## 3. 设计符合性检查

### 3.1 教学不是 UI 写死

已确认：P50 后端模块未出现 H5 / dialog / playable 依赖。

扫描命令：

```bash
rg -n "h5|dialog|playable" backend/include/pathfinder/world_teaching backend/src/world_teaching -S || true
```

结果：无匹配。

### 3.2 没有内容写死

已确认：P50 后端模块未写死红果、火把、木头、野兽、狼、蘑菇、同伴等内容。

扫描命令：

```bash
rg -n "red_berry|torch|wood|camp_fire|beast|mushroom|berry|wolf|companion" backend/include/pathfinder/world_teaching backend/src/world_teaching -S || true
```

结果：无匹配。

### 3.3 前端不能伪造知识

已确认：P50 后端模块未发现 `recipient_claim_json`、`force_teach`、`learned=true`、直接读 `parameters[]` 等风险入口。

扫描命令：

```bash
rg -n "recipient_claim_json|force_teach|learned=true|parameters\[|TODO|FIXME" backend/include/pathfinder/world_teaching backend/src/world_teaching -S || true
```

结果：无匹配。

## 4. 测试结果

### 4.1 P50 专项测试

命令：

```bash
cmake --build build/backend --target pathfinder_tests_world_teaching pathfinder_tests_world_teaching_integration -j 2
ctest --test-dir build/backend -R '^(world_teaching_|architecture_world_teaching_)' --output-on-failure
```

结果：

```text
22/22 passed
```

覆盖重点：

- 枚举 roundtrip。
- actor -> KnowledgeOwner 映射。
- 缺失 source claim 拒绝。
- owner mismatch 拒绝。
- 不可教学 claim 拒绝。
- 超距离拒绝。
- 教学创建接收者 claim。
- 已有 claim 强化 / 更新。
- 无知 NPC 行动门禁阻断。
- AI 禁用知识不能驱动 NPC 行动。
- 风险知识无目标时阻断。
- 玩家教 NPC。
- NPC 教玩家。
- 缺少 actor query port 时阻断且不写 repository。
- handler 可注册到 command registry。
- 教学后行动门禁放行。
- 无 H5 依赖扫描。
- 无内容写死扫描。

### 4.2 相关回归测试

命令：

```bash
ctest --test-dir build/backend -R '^(world_command_|world_learning_|world_teaching_|architecture_world_teaching_|learning_|knowledge_|memory_|cognition_)' --output-on-failure
```

结果：

```text
165/165 passed
```

## 5. 文件变更范围

新增模块：

```text
backend/include/pathfinder/world_teaching/
backend/src/world_teaching/
```

新增测试：

```text
backend/tests/unit/world_teaching/world_teaching_test.cpp
backend/tests/integration/world_teaching/world_teaching_integration_test.cpp
```

更新构建：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

## 6. 剩余非阻塞风险

### 6.1 P50 只做单步门禁

`NpcBasicActionKnowledgeGate` 只判断 NPC 是否知道某个单步行动，不负责链式目标、材料补齐、移动寻路、世界打断。

归属：P51 / 后续 Agent 网格目标执行。

### 6.2 当前 actor query port 仍由集成方注入

P50 已定义只读 port 并用 fake 测试覆盖，但真实 runtime 接线需要在后续 command runtime / world runtime 集成阶段继续扩大。

### 6.3 教学事件 id 目前使用 knowledge id + tick

在同一 tick 重复教学同一 knowledge id 时可能需要更强唯一性。当前不影响 P50 逻辑正确性，后续存档 / 复盘阶段可统一升级事件 id 生成策略。

## 7. 最终判断

P50 可以验收进入下一阶段。

它解决了 P49 之后的关键断点：玩家学到的世界知识可以通过正式 command 传播给 NPC，NPC 不再读玩家知识，也不靠前端写死，而是拥有自己的知识 owner 和自己的行动门禁输入。
