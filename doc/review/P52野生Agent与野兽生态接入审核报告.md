# P52 野生 Agent 与野兽生态接入审核报告

## 1. 审核结论

结论：通过。

P52 已完成“野生 Agent / 野兽生态接入”的后端抽象、Command 管线接入、测试覆盖与架构扫描。实现没有接入旧 H5，没有写死狼、火把、红果等演示内容；野兽行动通过 `WorldCommandDto` 输出，`source` 为 `WorldCommandSource::BeastDecision`；经验结果保留在 `WorldCommandExecutionResult` 中，可继续进入后续学习链。

本次审核发现并已修复 3 个关键问题：

1. 风险知识关系修正：不再把 `HasEffect` 当作风险知识，改为只认 `HasRisk` 和 `IsDangerousUnder`。
2. 游荡硬编码修正：不再把 `Wander` 编译为移动到 `(0,0,surface)`，无目标时安全降级为 `Wait`。
3. 感知链路修正：`BeastEcologyCoordinator` 不再绕过 `BeastPerceptionBuilder`，而是通过 raw world query 获取附近 actor/entity/resource/effect，再由 builder 统一构建感知。

## 2. 审核范围

### 2.1 设计文档

- `doc/82_P52野生Agent与野兽生态接入任务卡设计.md`
- `doc/82A_P52野生Agent与野兽生态风险头脑风暴.md`
- `doc/82B_P52野生Agent与野兽生态测试文档.md`
- `doc/review/P52_Kimi实现任务单.md`

### 2.2 后端代码

- `backend/include/pathfinder/world_beast_ecology/`
- `backend/src/world_beast_ecology/`
- `backend/tests/unit/world_beast_ecology/`
- `backend/tests/integration/world_beast_ecology/`
- `backend/CMakeLists.txt`
- `backend/tests/CMakeLists.txt`

## 3. 架构符合性

### 3.1 本能与知识分离

通过。

`BeastAgentProfile` 保存本能能力、感知半径、性情、猎物标签、危险标签、威慑标签。它没有把本能写入 `KnowledgeRepository`。

野兽后天知识通过 `IBeastKnowledgeQueryPort::claimsForBeast` 获取，策略层只把 `HasRisk` 与 `IsDangerousUnder` 作为风险知识输入，避免把普通效果误判为危险。

### 3.2 Command 管线接入

通过。

`BeastCommandCompiler` 将野兽意图编译成 `WorldCommandDto`，并强制：

```text
source = WorldCommandSource::BeastDecision
```

`BeastEcologyCoordinator` 只通过 `IBeastCommandPipelinePort::executeBeastCommand` 执行命令，不直接修改 runtime。

### 3.3 感知构建链路

通过。

最终链路为：

```text
IBeastWorldQueryPort.findBeastActor
IBeastWorldQueryPort.nearbyActorsForBeast
IBeastWorldQueryPort.nearbyEntitiesForBeast
IBeastWorldQueryPort.nearbyResourcesForBeast
IBeastWorldQueryPort.nearbyEffectsForBeast
-> BeastPerceptionBuilder.build
-> BeastDecisionPolicy.selectIntent
```

这比初版更符合设计文档，不再让 world port 直接返回已经加工好的 perception。

### 3.4 外界打断信号

通过。

`BeastInterruptProjector` 在野兽接近或攻击 actor 时输出 `ExternalInterruptSignal`，只通知 P51/P40 后续系统，不替 NPC 做决策。

### 3.5 经验保留

通过。

`BeastTickResult.command_result` 保留 `WorldCommandExecutionResult`，测试覆盖了 `experiences` 不被吞掉。

## 4. 修复记录

### 4.1 风险知识关系修复

初版问题：

```text
HasEffect 被当作风险知识；HasRisk 没有被识别。
```

修复后：

```text
只有 HasRisk / IsDangerousUnder 会触发 LearnedRisk。
```

影响：后续“普通效果知识”和“风险知识”不会混淆，野兽不会因为学到某物“有作用”就错误逃跑。

### 4.2 Wander 原点硬编码修复

初版问题：

```text
Wander 无目标时被编译为 Move 到 (0,0,surface)。
```

修复后：

```text
Wander 无 target_coord 时降级为 Wait。
```

影响：不会出现所有野兽默认向世界原点移动的假生态行为。未来如需真实游荡，应由地图/寻路/生态策略生成明确目标坐标后再编译 Move。

### 4.3 感知 builder 真实接入

初版问题：

```text
Coordinator 注入了 BeastPerceptionBuilder，但没有使用。
```

修复后：

```text
Coordinator 通过 raw world query 获取原始世界数据，再调用 BeastPerceptionBuilder.build。
```

影响：感知层成为稳定抽象点，后续可扩展视野、听觉、气味、遮挡、区域魔法、声音诱导，而不是散落在 world port 里。

## 5. 测试结果

### 5.1 构建

```bash
cmake --build build/backend --target pathfinder_tests_world_beast_ecology pathfinder_tests_world_beast_ecology_integration -j 2
```

结果：通过。

### 5.2 P52 专属测试

```bash
ctest --test-dir build/backend -R '^(world_beast_ecology_|architecture_world_beast_ecology_)' --output-on-failure
```

结果：12/12 通过，0 失败。

覆盖：

- enum roundtrip
- profile validate
- perception builder
- instinct gate
- decision policy
- command compiler
- coordinator
- integration
- no H5 dependency
- no content hardcode
- no direct runtime mutation
- no frontend claim injection

### 5.3 相关回归测试

```bash
ctest --test-dir build/backend -R '^(world_agent_execution_|world_command_|world_runtime_|world_learning_|world_teaching_|agent_|goal_execution_|knowledge_)' --output-on-failure
```

结果：527/527 通过，0 失败。

## 6. 架构扫描结果

### 6.1 H5 / 对话 / playable 依赖

结果：无匹配。

### 6.2 内容硬编码

扫描项：

```text
wolf|torch|fire|red_berry|berry|wood|beast_shadow|companion
```

结果：无匹配。

### 6.3 直接 runtime / inventory 修改

结果：无匹配。

### 6.4 前端注入知识

扫描项：

```text
recipient_claim_json|learned=true|force_learn|force_teach|knowledge_json
```

结果：无匹配。

## 7. 剩余说明

P52 完成的是“野生 Agent 接入框架”，不是完整生态模拟。以下能力按设计留给后续阶段：

1. 真实游荡目标生成。
2. 完整战斗伤害结算。
3. 群体狩猎、呼叫同伴、领地协同。
4. 驯化、坐骑、护卫关系。
5. 高级生物升级到 P51/P39 复杂规划能力。
6. 地图遮挡、气味、声音传播、区域魔法对感知的高级影响。

这些没有阻塞 P52，因为 P52 已经把扩展点放在 profile、world query port、perception builder、decision policy、command compiler、interrupt projector 上。

## 8. 最终判断

P52 可以验收。

本阶段实现严格保留现有认知体系和 Command 管线，没有另起炉灶；野兽是低级 Agent，具备本能、感知、行动、经验保留和外界打断投射能力。后续 P53 或 V2 地图阶段可以基于该模块继续扩展野生生态，而不需要重构整体架构。
