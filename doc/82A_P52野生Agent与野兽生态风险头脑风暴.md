# P52 野生 Agent 与野兽生态风险头脑风暴

## 1. 复杂场景压力测试

### 1.1 普通野兽

场景：低级捕食者发现猎物、靠近、攻击、被危险源吓退。

要求：不能写死某种狼或某种火，只能靠 prey/danger/deterrent tag 和 profile。

### 1.2 蜘蛛 / 毒虫

场景：蜘蛛不会正面追逐，而是守株待兔、靠近后攻击。

要求：P52 profile 要支持 temperament 和 action preference，不能只有“追玩家”。

### 1.3 群体动物

场景：一只野兽发现玩家后呼叫同伴。

P52 不做完整群体，但必须预留 `CallPack` / `Pack`，不能把单体假设写死。

### 1.4 被驯化生物

场景：野兽可以从野生变成坐骑 / 护卫。

P52 不实现驯化，但 profile / actor / ownership 不能阻止后续把野兽转为友方 Agent。

### 1.5 机械巡逻体

场景：机械单位不是饥饿驱动，而是巡逻、警戒、攻击入侵者。

要求：需求系统不能只写 hunger，必须支持 Territory / Constructed。

### 1.6 魔法元素

场景：火元素不怕火，水元素怕雷，亡灵怕圣光。

要求：危险和威慑必须来自 profile tag / learned risk，不是全局规则。

### 1.7 外星高级生物

场景：高级生物有复杂知识和工具。

要求：P52 作为低级 Agent 框架，不能堵死未来升级到 P51/P39 完整规划。

### 1.8 野兽学习

场景：野兽第一次接近危险效果，受伤或被驱退；第二次看到相同 tag 会回避。

要求：P52 只保留 experience 和读取自己的 claim，不能直接写知识。

### 1.9 地图遮挡

场景：玩家躲在障碍后，野兽听到声音但看不到。

要求：perception kind 要区分 Actor / Sound / Area，不能只支持视觉。

### 1.10 区域性危险

场景：毒雾、火墙、区域魔法会影响野兽路径。

要求：支持 Effect / Area perception，后续路径规划可以使用。

## 2. 主要风险

| 风险 | 后果 | P52 对策 |
|---|---|---|
| 写死狼怕火 | 后续内容扩展失败 | 只认 tag / profile / learned claim |
| 野兽直接改坐标 | 绕过 P43/P44/P49 | 所有行动必须 WorldCommand |
| 本能伪装成知识 | 知识系统污染 | 本能 profile 与 KnowledgeClaim 分离 |
| 野兽读玩家知识 | NPC/生物全知 | IBeastKnowledgeQueryPort 只读自身 owner |
| Attack 无 handler | 实现卡住 | P52 可用 fake pipeline 验证结构，不做战斗结算 |
| 威胁直接替 NPC 决策 | P51/P40 失效 | 只输出 ExternalInterruptSignal |
| 感知泄露隐藏真相 | 玩家体验崩坏 | perception port 只能返回可见/可闻/可感知摘要 |
| 群体行为太早做 | 范围爆炸 | P52 只预留，不实现群体调度 |
| 学习被短路 | 野兽永远假智能 | experiences 必须保留，不直接造 claim |
| profile 太薄 | 后续机械/魔法不够用 | temperament、need、capability、tags 分层 |

## 3. 实现门禁

```text
如果 P52 后端模块出现 wolf/torch/fire/red_berry/companion 字符串，直接不通过。
如果 P52 command source 不是 BeastDecision，直接不通过。
如果 P52 直接访问 runtime map 修改状态，直接不通过。
如果 P52 的测试只验证固定中文输出，直接不通过。
如果 P52 不保留 command_result.experiences，直接不通过。
```
