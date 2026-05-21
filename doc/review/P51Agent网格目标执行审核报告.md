# P51 Agent 网格目标执行审核报告

## 1. 审核结论

结论：通过。

P51 已建立 `world_agent_execution` 模块，把“NPC / Agent 拥有知识后如何在 V2 网格中行动”接回现有架构：

- Agent tick 以 `WorldAgentTickRequest` 为入口，支持按 actor 执行单次决策。
- 决策上下文由 port 构建，读取 actor、可见世界、背包、知识和已有执行上下文。
- 行动资格通过 P50 `NpcBasicActionKnowledgeGate` 校验，NPC 不能偷用玩家知识。
- 行动被编译为 `WorldCommandDto`，再交给 command pipeline port 执行。
- Agent command source 使用 `AgentDecision` / 结构预留 `BeastDecision`，不伪装成玩家输入。
- command 执行结果的 `events`、`state_deltas`、`projection_patch`、`experiences` 没有被 Agent 层吞掉。
- P51 后端模块未依赖旧 H5，也未写死红果、火把、木头、野兽、狼、蘑菇、同伴等内容。

## 2. 审核中发现并修复的问题

### 2.1 `WorldCommandDto.command_key` 未填充

问题：Kimi 初版编译出的 Agent command 只有 `command_kind`，没有填 `command_key`。这会违反 P43 command DTO 的基本校验，也会让后续 pipeline / 复盘 / 学习链路拿不到稳定 command key。

修复：

- `AgentPlanCommandCompiler` 现在为 Move / Gather / Generic command 填充标准 `command_key`。
- 编译完成后调用 `WorldAgentCompiledCommand::validateBasic()`。
- 新增测试断言 `command.validateBasic().is_ok()`，避免以后又生成不合规 command。

### 2.2 Move 目标解析失败时不应原地 fallback

问题：Kimi 初版在 Move 目标找不到时，会 fallback 到 actor 当前坐标，相当于把“目标不可解析”伪装成一次原地移动。

修复：

- Move 目标无法解析时返回编译阻断。
- 新增测试：不可见目标不会生成可执行 Move command。

### 2.3 架构扫描误伤 context store `put()`

问题：设计要求禁止直接 `.put()` 修改 runtime map。Kimi 初版 context store 接口也叫 `put()`，虽然不是 runtime 直改，但会被架构扫描误判，也容易让后续研发误解。

修复：

- `IWorldAgentExecutionContextStore::put()` 改名为 `saveContext()`。
- 所有调用点和测试已同步。
- 架构扫描已通过。

### 2.4 P39 Reasoner 请求未正确填充

问题：Kimi 初版 `WorldAgentReasoningBridge` 没有填 `ReasoningRequest.need_state.actor_key`，导致 P39 `AgentReasoner` 校验失败，实际长期走 fallback 最小计划。

修复：

- 增加 V2 grid -> P39 `WorldSnapshot` 的最小适配。
- 将可见资源、可见实体、actor 背包、外界威胁转成 P39 可读快照。
- 根据选中目标填充 `AgentNeedState`，让 P39 reasoner 能优先参与推理。
- fallback 仍保留，但只作为 P39 暂时无法表达 V2 目标时的通用兜底，不包含具体内容写死。

## 3. 设计符合性检查

### 3.1 不绕过 Command 管线

已确认：P51 不直接调用采集、制作、移动、吃、使用等底层 service。执行路径为：

```text
AgentExecutionCoordinator
-> WorldAgentContextBuilder
-> WorldAgentGoalSelector
-> WorldAgentReasoningBridge
-> AgentActionKnowledgeGuard
-> AgentPlanCommandCompiler
-> IAgentCommandPipelinePort.executeAgentCommand
```

### 3.2 不直接修改 runtime / inventory

扫描命令：

```bash
rg -n "\.actors\[|\.entities\[|\.resource_nodes\[|\.inventories\[|\.put\(|createOrUpdateGeneratedCell|upsertGeneratedResourceNode" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S || true
```

结果：无匹配。

### 3.3 不依赖旧 H5

扫描命令：

```bash
rg -n "h5|dialog|playable" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S || true
```

结果：无匹配。

### 3.4 不写死具体玩法内容

扫描命令：

```bash
rg -n "red_berry|torch|wood|beast|wolf|mushroom|companion" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S || true
```

结果：无匹配。

### 3.5 不允许前端伪造知识

扫描命令：

```bash
rg -n "recipient_claim_json|learned=true|force_learn|force_teach|knowledge_json" backend/include/pathfinder/world_agent_execution backend/src/world_agent_execution -S || true
```

结果：无匹配。

## 4. 测试结果

### 4.1 P51 专项构建

命令：

```bash
cmake --build build/backend --target pathfinder_tests_world_agent_execution pathfinder_tests_world_agent_execution_integration -j 2
```

结果：通过。

### 4.2 P51 专项测试与架构扫描

命令：

```bash
ctest --test-dir build/backend -R '^(world_agent_execution_|architecture_world_agent_execution_)' --output-on-failure
```

结果：

```text
12/12 passed
```

覆盖重点：

- 枚举 roundtrip。
- Agent execution context store。
- Context builder。
- P50 knowledge guard。
- Command compiler。
- Coordinator。
- Projection bridge。
- Integration：未知 NPC 被阻断，教学后可行动，移动/采集走 command，经验保留。
- 架构扫描：无 H5、无内容硬编码、无 runtime 直改、无前端伪造知识。

### 4.3 相关回归测试

命令：

```bash
ctest --test-dir build/backend -R '^(world_command_|world_runtime_|world_inventory_|world_harvest_|world_reaction_|world_learning_|world_teaching_|agent_|goal_execution_|knowledge_)' --output-on-failure
```

结果：

```text
599/599 passed
```

## 5. 文件变更范围

新增模块：

```text
backend/include/pathfinder/world_agent_execution/
backend/src/world_agent_execution/
```

新增测试：

```text
backend/tests/unit/world_agent_execution/
backend/tests/integration/world_agent_execution/
```

更新构建：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

新增审核报告：

```text
doc/review/P51Agent网格目标执行审核报告.md
```

## 6. 剩余非阻塞风险

### 6.1 P51 仍是单 actor tick

当前阶段只完成单 actor 的目标选择、计划、知识门禁、command 编译和执行回写。大规模族群调度、并发抢资源、协作任务分配仍在后续阶段处理。

### 6.2 完整寻路未实现

P51 只做相邻判断和最小移动 command，不做 A*。这是设计文档明确不做的范围，后续地图扩展阶段再实现。

### 6.3 P39 快照适配是最小桥接

已修复“总是 fallback”的问题，但 V2 grid -> P39 `WorldSnapshot` 目前仍是最小适配。后续 P52 / P53 / 大地图阶段应继续扩展对象状态、地形、危险区域、阵营、占用格等信息。

### 6.4 BeastDecision 结构预留，生态端到端在 P52

P51 已预留 `BeastDecision` 和外界威胁输入，但狼、蜘蛛、野生 Agent 的生态行为和学习闭环属于 P52。

## 7. 最终判断

P51 可以验收进入 P52。

这阶段已经把“NPC 被教会以后能不能自己做”从旧 H5 逻辑里抽离出来，并接回 V2 world command、P50 知识门禁、P39 推理、P45 背包、P47/P48/P49 经验链路。当前实现不是为红果/火把/狼写死的 demo，而是通用 Agent 网格目标执行骨架。
