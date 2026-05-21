# P51 Agent 网格目标执行风险头脑风暴

## 1. 风险结论

P51 最大风险不是“NPC 不会动”，而是“NPC 看起来会动，但其实绕过知识、背包、Command 或世界规则”。

因此 P51 审核重点必须放在：

```text
是否所有行动都走 WorldCommand。
是否所有行动都经过 NPC 自己知识门禁。
是否所有材料和物品都经过 P45 归属。
是否所有世界变化都来自 P43-P48。
是否没有 H5 / 内容写死。
```

## 2. 架构风险

### 2.1 绕过 WorldCommand

风险：研发为了快速实现，直接调用 runtime 或 inventory 修改状态。

后果：

```text
没有 events。
没有 projection patch。
没有 experiences。
不能复盘。
不能学习。
前端看到的状态和后端真实状态可能不同。
```

防护：

```text
AgentExecutionCoordinator 只能通过 IAgentCommandPipelinePort 提交 command。
架构扫描禁止在 world_agent_execution 中出现 create/update/put/remove runtime map 的直接调用。
```

### 2.2 绕过知识门禁

风险：NPC 只要有目标就行动，不检查自己是否知道。

后果：

```text
玩家教学失去意义。
NPC 变成脚本机器人。
知识系统被架空。
```

防护：

```text
Gather/Chop/Eat/Use/Craft/Attack/CastAreaEffect 前必须调用 AgentActionKnowledgeGuard。
Move/Wait 可豁免，但要写明原因。
```

### 2.3 读取玩家知识

风险：NPC 查询知识时误用玩家 owner 或全局 repository。

后果：NPC 还没被教就会行动。

防护：

```text
IAgentKnowledgeQueryPort 只返回 actor_key 对应 KnowledgeOwner 的 claims。
测试必须覆盖“玩家知道但 NPC 不知道 -> NPC 不能行动”。
```

### 2.4 物品归属被绕过

风险：NPC 把地图上、玩家背包里、营地未授权的物品当成自己可用。

防护：

```text
材料检查只能通过 P45 inventory/location port。
制作/拾取/消耗只能通过 command。
```

### 2.5 计划层硬编码内容

风险：代码写死火把、木头、红果、狼。

防护：

```text
架构扫描关键词。
测试内容使用 neutral_object / neutral_resource / neutral_effect。
```

## 3. 玩法风险

### 3.1 NPC 频繁切换目标

风险：每 tick 重新推理，导致 NPC 一会儿采集、一会儿等待、一会儿换目标。

防护：

```text
ExecutionContext 保存 active goal。
只有完成、失败、强打断、replan 条件满足时才换目标。
```

### 3.2 NPC 无限卡在缺材料

风险：缺材料 -> 子目标 -> 仍缺材料 -> 无限循环。

防护：

```text
max_subgoal_depth。
attempt_count。
blocker trace。
重复失败进入 Wait / NeedTeaching / NeedPlayerHelp。
```

### 3.3 NPC 远程采集

风险：目标不相邻却直接发 Gather。

防护：

```text
Compiler 先检查距离。
距离不足生成 Move command。
集成测试覆盖远距离资源。
```

### 3.4 外界打断吞掉原目标

风险：狼出现后暂停采集，处理后无法恢复。

防护：

```text
PausedExecutionContext 保存 driver state。
ResumeValidator 通过后恢复。
测试覆盖 pause -> emergency -> resume。
```

### 3.5 NPC 行为不可解释

风险：前端只看到 NPC 动了，不知道为什么。

防护：

```text
projection 输出 goal、step、reason_keys、blockers、issued command。
```

## 4. 工程风险

### 4.1 P39/P40/P41 与 V2 DTO 不兼容

风险：旧推理层使用 WorldSnapshot，V2 使用 WorldRuntimeSnapshot。

防护：

```text
WorldAgentReasoningBridge 做适配层。
不改旧 P39 核心，除非有明确必要且单独记录。
```

### 4.2 fake 测试过强，真实集成缺失

风险：fake pipeline 测试都过，但真实 command handler 接不上。

防护：

```text
必须至少做一个 WorldCommandPipeline fake registry 集成测试。
后续再接真实 runtime/inventory。
```

### 4.3 projection 泄漏隐藏信息

风险：把不可见资源、隐藏真相、未观察威胁暴露给前端。

防护：

```text
visibleSnapshotForActor 只给可见数据。
ProjectionBridge 只输出安全 summary。
```

## 5. 未来扩展压力测试场景

```text
1. NPC 学会采集后自动采集。
2. NPC 学会制作但缺材料，先采集再制作。
3. NPC 目标执行中遇到威胁，暂停目标。
4. NPC 处理威胁后恢复原目标。
5. NPC 没知识时等待教学。
6. NPC 知道风险知识但没有允许风险目标，不执行危险行动。
7. BeastDecision 使用同一套 command 提交，不走同伴特判。
8. 高级 Agent 长链计划超过深度时安全失败。
9. 玩家命令打断 NPC 当前计划。
10. 区域魔法/区域事件通过 command 进入执行，不直接改地图。
```

## 6. 风险审核清单

P51 实现后必须逐项确认：

```text
[ ] world_agent_execution 无 H5 依赖。
[ ] world_agent_execution 无内容硬编码。
[ ] world_agent_execution 不直接改 runtime / inventory / knowledge。
[ ] 所有行动 command.source 正确。
[ ] KnowledgeGate 被调用。
[ ] 失败时不写世界状态。
[ ] projection 不泄漏隐藏真相。
[ ] 测试不是靠固定中文文本判断成功。
```
