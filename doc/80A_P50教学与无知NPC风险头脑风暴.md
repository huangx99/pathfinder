# P50 教学与无知 NPC 风险头脑风暴

## 1. 目的

本文件用于在 P50 开发前提前暴露风险。

P50 的危险点不在“能不能复制一个知识”，而在于如果实现错了，后续 P51/P52 会建立在假智能上。

## 2. 十类后续场景压力测试

### 2.1 玩家教同伴火把知识

玩家知道火把能驱赶野兽，教给同伴。

要求：同伴获得自己的 claim，后续 P51 查询同伴知识时能看到。

风险：如果实现成共享玩家 claim，同伴看似知道，实际 owner 错误。

### 2.2 NPC 反向教玩家

未来驾校教练教玩家开车。

要求：teacher 可以是 NPC，recipient 可以是玩家。

风险：如果 handler 写死 actor_key=player，则专家 NPC 系统无法扩展。

### 2.3 危险知识教学

玩家知道毒蘑菇会中毒，教给 NPC。

要求：NPC 不是去吃毒蘑菇，而是获得风险知识，后续行动门禁应阻止无目标危险行为。

风险：把危险知识也当成行动推荐，会让 NPC 主动送死。

### 2.4 错误知识修正

NPC 原来以为某物可食用，玩家教它“这个会中毒”。

要求：传播系统能路由到 revision 或更新已有 claim。

风险：简单新增第二条 claim 会造成 NPC 同时相信两种冲突结论。

### 2.5 距离约束

玩家和 NPC 不在同一格附近。

要求：教学失败，不产生知识变化。

风险：全图远程教学会破坏开放世界空间意义。

### 2.6 教学不可教学知识

某些知识是个人体验、低可信、梦境、debug、隐藏真相。

要求：teaching_profile.teachable=false 时拒绝。

风险：如果只看 claim 存在，隐藏知识会被传播。

### 2.7 接收者已有相同知识

同伴已经知道火把知识，再教一次。

要求：强化或跳过，不产生重复垃圾 claim。

风险：repository 中出现大量重复 claim，后续 AI 决策噪声增加。

### 2.8 多 NPC 教学

玩家先教 A，A 再教 B。

要求：B 的 source owner 是 A，但 related_knowledge_ids 可追溯到原始知识。

风险：传播链断裂，后续复盘看不出知识从哪里来。

### 2.9 野生 Agent 观察学习

狼看到火会害怕，未来可能不是“教学”，而是 Demonstration / Observation。

要求：P50 的 channel 设计不能只支持玩家对话教学。

风险：把教学写成中文对话系统，后续野兽学习无法复用。

### 2.10 文明级教育

高级文明向族群传播科技知识。

要求：owner 类型不能只支持玩家和同伴，至少保持 KnowledgeOwner 可扩展。

风险：写死 actor owner，后续 Tribe/Civilization 重构成本巨大。

## 3. 核心风险清单

| 风险 | 严重度 | 表现 | 预防 |
|---|---|---|---|
| 复制 claim 不改 owner | 阻塞 | NPC 知识属于玩家 | 使用 propagation service |
| 绕过 LearningLoop | 阻塞 | 没有传播 trace | TeachingLoopBridge |
| 前端伪造知识 | 阻塞 | 客户端传 claim | 只接受 knowledge_id |
| 不校验距离 | 高 | 全图教学 | Actor query port |
| 不可教学也能教 | 高 | hidden truth 泄露 | teaching_profile 校验 |
| 重复 claim 膨胀 | 中 | 决策噪声 | applyRecipientClaims |
| 风险知识变推荐 | 高 | NPC 自杀行为 | Action gate 区分 risk |
| 写死 companion | 阻塞 | 后续 NPC 无法复用 | actor_key 泛化 |
| 依赖 H5 dialog | 高 | V2 前端无法用 | world_command handler |
| 测试只测火把 | 中 | 假可扩展 | 多内容测试 |

## 4. 风险对应测试

```text
T01 owner 转移正确。
T02 不可教学 claim 被拒绝。
T03 距离过远失败。
T04 不存在知识失败。
T05 同一 owner 自教默认失败或跳过。
T06 已有知识强化不重复。
T07 NPC 教玩家。
T08 危险知识通过传播但行动门禁不主动允许危险。
T09 projection patch 指向 recipient。
T10 生产代码扫描无 H5 和具体内容 key。
```

## 5. 设计修订结论

P50 必须增加两个看似“小”，但对长期架构关键的接口：

```text
IWorldActorQueryPort：只读查询 actor 坐标，用于教学距离。
NpcBasicActionKnowledgeGate：只判断 NPC 是否有行动知识，不负责执行和规划。
```

这两个接口能避免 P50 写死演示，也能让 P51 直接复用。
