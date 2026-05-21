# P52 野生 Agent 与野兽生态接入任务卡设计

## 1. 阶段定位

P52 是 V2.0 开放世界网格中的“野生 Agent / 野兽生态”阶段。

它接在以下阶段之后：

```text
P43：统一 WorldCommand 管线。
P44：二维世界网格与 actor 坐标。
P45：背包、物品实例和归属接口。
P46：世界生成器和资源分布。
P47：采集、砍伐、挖掘命令。
P48：制作与世界反应。
P49：世界经验进入认知 / 记忆 / 知识。
P50：教学与无知 NPC 基础行动门禁。
P51：Agent 网格目标执行。
```

P52 的一句话定位：

```text
让狼、蜘蛛、野生动物、低级怪物等非玩家生物成为网格上的低级 Agent，能感知、接近、回避、攻击、逃离，并通过 Command 管线产生可学习经验。
```

P52 不是写死“狼怕火”，也不是恢复旧 H5 的“靠近的野兽”按钮。P52 要建立一个可扩展的野生 Agent 框架：狼只是内容配置中的一种 profile；火只是可被感知的危险/威慑效果；后续蜘蛛、机械巡逻体、魔法元素、外星生物都能复用同一套结构。

## 2. 为什么现在做

P51 完成后，系统已经具备：

```text
NPC 可以基于自己的知识和目标执行 WorldCommand。
Agent 行动可以走 P50 知识门禁。
Agent 行动结果可以保留 events / state_deltas / experiences。
Agent 不需要旧 H5 按钮驱动。
```

但如果没有 P52，会出现以下断层：

```text
世界地图上有野兽，但野兽不会自己移动。
野兽遇到玩家、NPC、火、陷阱、食物时只能靠写死逻辑反应。
野兽不能作为 Agent 产生经验，也就不能学习“火危险”“陷阱危险”。
NPC 遇到野兽时只有外界打断，但野兽本身没有权威行动来源。
前端地图即使画出狼，也只能展示假状态，不能展示后端权威行为。
```

所以 P52 必须在 H5 V2 地图前完成。否则开放世界会有空间但没有生态。

## 3. 阶段目标

P52 完成后，系统必须具备以下能力：

```text
1. 系统可以对一个 beast actor 执行一次 Beast tick。
2. Beast tick 能读取野兽 profile、actor 坐标、附近 actor、附近物体、附近效果、已有知识和本能能力。
3. 野兽行动不能直接改 runtime，必须编译成 WorldCommandDto 并走 command pipeline。
4. 野兽 command source 必须是 WorldCommandSource::BeastDecision。
5. 野兽可以基于本能执行基础行动，例如移动、接近、回避、攻击、逃离、等待。
6. 野兽可以基于学习到的 KnowledgeClaim 调整行动，例如知道某效果危险后回避。
7. 野兽不能读取玩家知识，不能读取 NPC 私有知识。
8. 野兽遭遇 command 结果时必须保留 experiences，供 P49 或后续学习链处理。
9. 野兽接近玩家 / NPC 时能输出外界打断信号，供 P51 NPC 响应。
10. 野兽被威慑、受伤、饥饿、发现目标、失去目标时能形成可追踪决策原因。
11. P52 模块不得写死 wolf / torch / fire / berry / companion 等具体内容。
12. 测试必须覆盖“学会危险后回避”的通用能力，而不是只测某个狼怕火文本。
```

## 4. 本阶段不解决的问题

```text
不做复杂群体狩猎队形，只做单野兽 / 单低级 Agent tick。
不做完整 A* 寻路，只生成 Move / Flee 的下一步或目标 command。
不做完整战斗伤害系统，Attack command 可以通过 pipeline fake 或已有 handler 验证来源与目标。
不做繁殖、生态食物链、领地刷新、季节迁徙。
不做前端地图 UI。
不做把本能直接写入 KnowledgeRepository；本能是 profile capability，学习仍走 P49 / P18-P21。
不做内容 JSON 的完整加载，P52 只定义 profile 结构和 port，P42 / 后续内容阶段再接真实 JSON。
```

## 5. 核心设计原则

### 5.1 野兽也是 Agent，但不是人类 NPC

野兽要复用 Agent 的核心思想：感知、目标、行动、反馈、经验。但野兽不等于人类 NPC：

```text
人类 NPC：主要靠被教学和认知知识行动。
野兽 / 低级 Agent：主要靠本能 profile + 少量经验学习行动。
高级文明 Agent：未来可以靠知识、规划、工具、组织协作行动。
```

因此 P52 要同时支持：

```text
InstinctCapability：先天能力 / 本能。
KnowledgeClaim：后天学习 / 经验。
```

### 5.2 本能不能伪装成知识

野兽出生就会移动、嗅探、追逐、逃离，这是本能，不是已学习知识。

禁止：

```text
开局给野兽写入“火危险”的 KnowledgeClaim。
把所有野兽本能都塞进 KnowledgeRepository。
让野兽无条件知道所有物品效果。
```

允许：

```text
profile 里配置 can_move / can_attack / avoid_danger_tag。
野兽被火烧、被威慑、靠近火后形成 experience。
P49 / 后续学习链把 experience 转成野兽自己的 KnowledgeClaim。
```

### 5.3 行动必须走 WorldCommand

P52 不允许直接移动野兽坐标、直接修改威胁等级、直接攻击 actor 状态。

正确流程：

```text
BeastEcologyCoordinator
-> BeastPerceptionBuilder
-> BeastDecisionPolicy
-> BeastCommandCompiler
-> WorldCommandDto(source=BeastDecision)
-> IBeastCommandPipelinePort.executeBeastCommand
-> WorldCommandExecutionResult
-> BeastTickResult
```

### 5.4 学习改变倾向，不改变隐藏真相

野兽学会“某效果危险”后，只能影响：

```text
目标选择。
风险评分。
回避距离。
是否继续攻击。
是否转向寻找其他目标。
```

不能让野兽读取隐藏真相：

```text
不能知道玩家背包里隐藏的火把。
不能知道未观察过的陷阱。
不能知道地图外的危险源。
不能知道某物真正效果，除非有 experience / knowledge / perceptible effect。
```

### 5.5 野兽对 NPC 是外界打断

玩家 / NPC 自己缺材料是内部阻塞；野兽靠近是外界打断。

P52 需要输出 `ExternalInterruptSignal`：

```text
ThreatAppeared
ThreatEscalated
DependentInDanger
PathBlocked
```

由 P51 / P40 去暂停当前目标、插入紧急目标，而不是 P52 直接替 NPC 决策。

### 5.6 内容可扩展，代码只认抽象标签

P52 不能写死：

```text
wolf
spider
torch
fire
red_berry
poison_mushroom
```

代码只认：

```text
prey_tag
predator_tag
danger_effect_tag
deterrent_effect_tag
food_tag
territory_tag
pack_tag
```

具体内容由 profile / JSON / content registry 后续接入。

## 6. 模块划分

| 模块 | 中文含义 | 职责 | 输入 | 输出 | 禁止事项 |
|---|---|---|---|---|---|
| `world_beast_ecology` | 野生 Agent 生态模块 | P52 主模块 | BeastTickRequest | BeastTickResult | 禁止旧 H5 依赖 |
| `BeastProfileRepositoryPort` | 野兽 profile 查询端口 | 查询低级 Agent 本能、感知、倾向 | actor_key / profile_key | BeastAgentProfile | 禁止写死狼 |
| `BeastPerceptionBuilder` | 野兽感知构建器 | 构造附近 actor / 物体 / 效果 / 威胁 | world query port | BeastPerceptionResult | 禁止读取不可见对象 |
| `BeastKnowledgeAdapter` | 野兽知识适配器 | 查询该野兽自己的 KnowledgeClaim | actor_key | claims | 禁止读取玩家知识 |
| `BeastInstinctGate` | 本能门禁 | 判断本能是否允许行动 | profile + intent | allowed / blocker | 禁止伪造知识 |
| `BeastDecisionPolicy` | 野兽决策策略 | 根据感知、本能、知识选 intent | context | BeastActionIntent | 禁止内容 if/else |
| `BeastCommandCompiler` | 野兽命令编译器 | intent -> WorldCommandDto | intent + context | command | source 必须 BeastDecision |
| `BeastEcologyCoordinator` | 野兽生态协调器 | 串联感知、决策、命令、结果 | tick request | tick result | 禁止直改 runtime |
| `BeastInterruptProjector` | 外界打断投影器 | 将野兽状态转成 P51 可读打断 | tick result / perception | ExternalInterruptSignal | 禁止替 NPC 决策 |
| `BeastProjectionBridge` | 安全投影 | 输出前端/调试可读摘要 | tick result | safe projection | 禁止 hidden truth |

## 7. 核心枚举

### 7.1 `BeastTemperamentKind`

中文：野兽气质 / 基础性格。

| 值 | 中文 | 使用场景 |
|---|---|---|
| `Unknown` | 未知 | 默认非法输入 |
| `Passive` | 被动 | 只会逃离或游荡 |
| `Skittish` | 胆小 | 易被危险源吓退 |
| `Curious` | 好奇 | 会接近未知对象 |
| `Territorial` | 领地型 | 目标进入领地会驱赶 |
| `Predatory` | 捕食型 | 会追逐 prey tag |
| `Pack` | 群体型 | 后续群体行为预留 |
| `Constructed` | 构造体 | 机械 / 魔法造物预留 |
| `TestOnly` | 测试专用 | 只允许测试 |

### 7.2 `BeastNeedKind`

中文：野兽需求类型。

| 值 | 中文 | 使用场景 |
|---|---|---|
| `Unknown` | 未知 | 非法输入 |
| `Hunger` | 饥饿 | 寻找食物 / 猎物 |
| `Fear` | 恐惧 | 回避危险源 |
| `Territory` | 领地 | 驱赶入侵者 |
| `Curiosity` | 好奇 | 接近未知对象 |
| `Pain` | 疼痛 | 逃离伤害来源 |
| `Rest` | 休息 | 停留 / 游荡 |
| `ProtectYoung` | 保护幼崽 | 后续生态扩展 |
| `TestOnly` | 测试专用 | 只允许测试 |

### 7.3 `BeastPerceptionKind`

中文：感知项类型。

| 值 | 中文 | 使用场景 |
|---|---|---|
| `Unknown` | 未知 | 非法输入 |
| `Actor` | Actor | 玩家 / NPC / 其他野兽 |
| `Object` | 物体 | 可见物品 / 掉落物 |
| `Resource` | 资源 | 资源节点 |
| `Effect` | 效果 | 火光、气味、魔法区、毒雾 |
| `Sound` | 声音 | 后续声音系统 |
| `Area` | 区域 | 危险区 / 领地 |
| `MemoryCue` | 记忆线索 | 后续野兽记忆 |
| `TestOnly` | 测试专用 | 只允许测试 |

### 7.4 `BeastActionIntentKind`

中文：野兽行动意图。

| 值 | 中文 | 使用场景 |
|---|---|---|
| `Unknown` | 未知 | 非法输入 |
| `Wait` | 等待 | 无目标 / 冷却 |
| `Wander` | 游荡 | 无明确目标随机移动 |
| `Approach` | 接近 | 接近猎物 / 好奇对象 |
| `Flee` | 逃离 | 远离危险源 |
| `Attack` | 攻击 | 攻击目标 actor |
| `Threaten` | 威吓 | 领地驱赶但未攻击 |
| `Observe` | 观察 | 学习 / 判断未知效果 |
| `AvoidArea` | 回避区域 | 远离危险区 |
| `CallPack` | 呼叫同伴 | 群体行为预留 |
| `TestOnly` | 测试专用 | 只允许测试 |

### 7.5 `BeastDecisionReasonKind`

中文：野兽决策原因。

| 值 | 中文 | 使用场景 |
|---|---|---|
| `Unknown` | 未知 | 非法输入 |
| `InstinctNeed` | 本能需求 | 饥饿、领地、恐惧 |
| `PerceivedPrey` | 发现猎物 | 接近 / 攻击 |
| `PerceivedDanger` | 发现危险 | 逃离 / 回避 |
| `LearnedRisk` | 学到风险 | 根据 KnowledgeClaim 回避 |
| `TerritoryIntrusion` | 领地入侵 | 威吓 / 攻击 |
| `CommandBlocked` | 命令被阻断 | 记录失败 |
| `NoValidAction` | 无可行动作 | wait |
| `TestOnly` | 测试专用 | 只允许测试 |

## 8. 核心 DTO / 数据结构

### 8.1 `BeastAgentProfile`

| 字段 | 类型 | 中文含义 | 必填 | 谁写入 | 谁读取 | 校验规则 | 禁止内容 |
|---|---|---|---|---|---|---|---|
| `profile_key` | string | profile id | 是 | content / fake repo | P52 | 非空 | 禁止写死内容判断 |
| `display_key` | string | 显示 key | 否 | content | projection | 可空 | 不能用于规则 |
| `temperament` | enum | 气质 | 是 | content | policy | 非 Unknown | - |
| `vision_radius` | int | 视觉半径 | 是 | content | perception | 0-64 | - |
| `hearing_radius` | int | 听觉半径 | 否 | content | perception | 0-64 | - |
| `base_aggression` | double | 基础攻击性 | 是 | content | policy | 0-100 | - |
| `base_fear` | double | 基础恐惧 | 是 | content | policy | 0-100 | - |
| `prey_tags` | vector<string> | 猎物标签 | 否 | content | policy | safe key | 禁止具体中文文本 |
| `danger_tags` | vector<string> | 本能危险标签 | 否 | content | gate | safe key | 不等于知识 |
| `deterrent_tags` | vector<string> | 威慑标签 | 否 | content | policy | safe key | - |
| `instinct_capabilities` | vector | 本能能力 | 是 | content | gate | 至少可 wait / move | 不能伪造 KnowledgeClaim |
| `reason_keys` | vector<string> | 调试原因 | 否 | system | report | safe key | 禁止 hidden truth |

### 8.2 `BeastInstinctCapability`

| 字段 | 类型 | 中文含义 | 必填 | 谁写入 | 谁读取 | 校验规则 | 禁止内容 |
|---|---|---|---|---|---|---|---|
| `capability_key` | string | 能力 key | 是 | content | gate | 非空 | - |
| `action_kind` | BeastActionIntentKind | 允许意图 | 是 | content | gate | 非 Unknown | - |
| `command_kind` | WorldCommandKind | 对应命令 | 是 | content | compiler | Move / Wait / Attack / Flee 等 | Unknown 禁止 |
| `requires_target` | bool | 是否需要目标 | 是 | content | compiler | - | - |
| `risk_limit` | double | 风险上限 | 否 | content | policy | 0-100 | - |
| `cooldown_ticks` | uint64 | 冷却 | 否 | content | coordinator | >=0 | - |

### 8.3 `BeastPerceptionItem`

| 字段 | 类型 | 中文含义 | 必填 | 谁写入 | 谁读取 | 校验规则 | 禁止内容 |
|---|---|---|---|---|---|---|---|
| `perception_id` | string | 感知 id | 是 | builder | policy | 非空 | - |
| `kind` | BeastPerceptionKind | 感知类型 | 是 | builder | policy | 非 Unknown | - |
| `target_ref` | string | 目标引用 | 是 | builder | compiler | 非空 | 不暴露隐藏对象 |
| `target_key` | string | 对象/actor/effect key | 是 | builder | policy | safe key | - |
| `coord` | optional coord | 坐标 | 否 | builder | compiler | layer 非空 | - |
| `distance` | int | 距离 | 是 | builder | policy | >=0 | - |
| `tag_keys` | vector<string> | 可感知标签 | 否 | builder | policy | safe key | 禁止 hidden truth |
| `intensity` | double | 强度 | 否 | builder | policy | 0-100 | - |
| `visible` | bool | 是否可见 | 是 | builder | policy | true 才可用于视觉 | - |

### 8.4 `BeastTickRequest`

| 字段 | 类型 | 中文含义 | 必填 | 谁写入 | 谁读取 | 校验规则 | 禁止内容 |
|---|---|---|---|---|---|---|---|
| `request_id` | string | tick id | 是 | scheduler | coordinator | 非空 | - |
| `actor_key` | string | 野兽 actor | 是 | scheduler | all | 非空 | - |
| `tick` | uint64 | 世界时间 | 是 | scheduler | all | >=0 | - |
| `allow_issue_command` | bool | 是否真的执行 | 是 | caller | coordinator | dry run 可 false | - |
| `allow_learning_claims` | bool | 是否读取后天知识 | 是 | caller | knowledge adapter | - | - |
| `reason_keys` | vector<string> | 调试原因 | 否 | caller | report | safe key | - |

### 8.5 `BeastActionIntent`

| 字段 | 类型 | 中文含义 | 必填 | 谁写入 | 谁读取 | 校验规则 | 禁止内容 |
|---|---|---|---|---|---|---|---|
| `intent_id` | string | 意图 id | 是 | policy | compiler | 非空 | - |
| `actor_key` | string | 执行者 | 是 | policy | compiler | 非空 | - |
| `kind` | enum | 行动意图 | 是 | policy | compiler | 非 Unknown | - |
| `target_ref` | string | 目标引用 | 视 kind | policy | compiler | 攻击/接近需非空 | - |
| `target_key` | string | 对象 key | 否 | policy | gate | safe key | - |
| `target_coord` | optional coord | 目标坐标 | 视 kind | policy | compiler | Move/Flee 可用 | - |
| `command_kind` | WorldCommandKind | 输出命令 | 是 | policy | compiler | 非 Unknown | - |
| `reason_kind` | enum | 决策原因 | 是 | policy | projection | 非 Unknown | - |
| `risk_score` | double | 风险评分 | 是 | policy | gate | 0-100 | - |
| `safe_summary_zh_cn` | string | 安全摘要 | 否 | policy | projection | 不含 hidden truth | - |

### 8.6 `BeastTickResult`

| 字段 | 类型 | 中文含义 | 必填 | 谁写入 | 谁读取 | 校验规则 | 禁止内容 |
|---|---|---|---|---|---|---|---|
| `ok` | bool | 是否成功 | 是 | coordinator | scheduler | - | - |
| `actor_key` | string | 野兽 actor | 是 | coordinator | caller | 非空 | - |
| `selected_intent` | optional | 选中意图 | 否 | coordinator | projection | validate | - |
| `issued_command` | optional WorldCommandDto | 发出的命令 | 否 | compiler | caller | source=BeastDecision | - |
| `command_result` | optional | command 结果 | 否 | pipeline | caller | 保留 experiences | - |
| `interrupts` | vector | 对其他 agent 的打断 | 否 | projector | P51 | safe | 不替 NPC 决策 |
| `events` | vector | 事件 | 否 | pipeline | frontend | safe | - |
| `state_deltas` | vector | 状态变更 | 否 | pipeline | frontend | safe | - |
| `projection_patch` | optional | 投影补丁 | 否 | pipeline | frontend | safe | - |
| `reason_keys` | vector | 原因链 | 否 | all | report | safe key | - |

## 9. 核心服务接口

### 9.1 `IBeastProfileQueryPort`

职责：根据 actor / profile key 查询野兽 profile。

```cpp
class IBeastProfileQueryPort {
public:
    virtual Result<BeastAgentProfile> profileForActor(const std::string& actor_key) const = 0;
};
```

失败情况：actor 不存在、profile 不存在、profile 非法。

### 9.2 `IBeastWorldQueryPort`

职责：只读查询野兽可感知世界。

```cpp
class IBeastWorldQueryPort {
public:
    virtual Result<WorldActorRuntime> findBeastActor(const std::string& actor_key) const = 0;
    virtual Result<std::vector<BeastPerceptionItem>> perceptionsForBeast(const std::string& actor_key, const BeastAgentProfile& profile) const = 0;
};
```

禁止：返回不可见对象、隐藏真相、玩家背包内部物品。

### 9.3 `IBeastKnowledgeQueryPort`

职责：查询野兽自己的 KnowledgeClaim。

```cpp
class IBeastKnowledgeQueryPort {
public:
    virtual Result<std::vector<KnowledgeClaim>> claimsForBeast(const std::string& actor_key) const = 0;
};
```

禁止：读取玩家 / NPC 私有知识。

### 9.4 `IBeastCommandPipelinePort`

职责：执行野兽 command。

```cpp
class IBeastCommandPipelinePort {
public:
    virtual Result<WorldCommandExecutionResult> executeBeastCommand(const WorldCommandDto& command) = 0;
};
```

要求：command.source 必须是 `WorldCommandSource::BeastDecision`。

### 9.5 `BeastDecisionPolicy`

职责：将感知、本能、知识转成一个行动意图。

决策优先级：

```text
1. LearnedRisk / PerceivedDanger：危险过高 -> Flee / AvoidArea。
2. TerritoryIntrusion：领地被侵犯 -> Threaten / Attack。
3. Hunger / PerceivedPrey：发现猎物 -> Approach / Attack。
4. Curiosity：发现未知强信号 -> Observe / Approach。
5. 无目标 -> Wander / Wait。
```

### 9.6 `BeastCommandCompiler`

职责：把 `BeastActionIntent` 编译成 `WorldCommandDto`。

要求：

```text
command_id = request_id + intent_id
command_key = toString(command_kind)
source = BeastDecision
actor_key = beast actor
目标必须来自感知结果或安全坐标
编译后必须 validateBasic
```

### 9.7 `BeastEcologyCoordinator`

职责：P52 主入口。

流程：

```text
validate request
-> query profile
-> query actor
-> build perception
-> query beast knowledge
-> policy.selectIntent
-> instinct / knowledge gate
-> compile command
-> dry run or execute pipeline
-> collect command result events / deltas / experiences
-> project external interrupts
-> return BeastTickResult
```

## 10. 与现有系统关系

### 10.1 与 P51 的关系

P51 负责通用 NPC / Agent 目标执行。P52 复用 P51 的原则，但野兽本能和生态感知有自己的 profile / policy。

后续可以把 P52 的 policy 输出接入 P51 更深层的目标执行，但 P52 当前必须至少保证：

```text
Beast command 走 WorldCommandPipeline。
Beast command source 是 BeastDecision。
Beast experiences 不被吞掉。
Beast threats 能转成 P51 ExternalInterruptSignal。
```

### 10.2 与 P49 的关系

P52 不形成知识，只保留 experiences。

野兽学会危险的正确链路：

```text
BeastDecision command
-> command_result.experiences
-> P49 / learning pipeline
-> beast actor owner 的 KnowledgeClaim
-> 下次 Beast tick 的 IBeastKnowledgeQueryPort 读到该 claim
-> BeastDecisionPolicy 降低接近倾向或选择 Flee
```

### 10.3 与 P50 的关系

P50 是人类 NPC 被教学和行动门禁。P52 对野兽使用“本能 + 自己知识”的双门禁：

```text
本能允许的动作：移动、等待、逃离、基础攻击。
后天知识影响的动作：回避特定效果、利用某物、学习某危险。
```

P52 不允许野兽读取玩家知识，也不允许把人类教学默认作用于野兽。

### 10.4 与 P43 的关系

所有 P52 行动最终必须是 `WorldCommandDto`。

## 11. 测试与验收标准

P52 必须新增：

```text
pathfinder_world_beast_ecology
pathfinder_tests_world_beast_ecology
pathfinder_tests_world_beast_ecology_integration
```

测试至少覆盖：

```text
1. 枚举 roundtrip / validateBasic。
2. profile 非法时拒绝。
3. 无感知目标时 Wait / Wander。
4. 发现 prey tag 时 Approach / Attack intent。
5. 发现 danger / deterrent tag 时 Flee / AvoidArea。
6. 已有 LearnedRisk KnowledgeClaim 时降低接近倾向。
7. command source 必须是 BeastDecision。
8. command_key 必须非空且 validateBasic 通过。
9. command 必须经 pipeline fake 执行，不直接改 runtime。
10. command result experiences 被保留。
11. 野兽接近目标时生成 ExternalInterruptSignal。
12. 不读取玩家 / NPC 私有知识。
13. 无 H5 / dialog / playable 依赖扫描。
14. 无 wolf / torch / fire / berry / companion 等内容写死扫描。
15. 无 `.actors[` / `.entities[` / `.resource_nodes[` / `.inventories[` 直改扫描。
```

必跑命令：

```bash
cmake --build build/backend --target pathfinder_tests_world_beast_ecology pathfinder_tests_world_beast_ecology_integration -j 2
ctest --test-dir build/backend -R '^(world_beast_ecology_|architecture_world_beast_ecology_)' --output-on-failure
ctest --test-dir build/backend -R '^(world_agent_execution_|world_command_|world_runtime_|world_learning_|world_teaching_|agent_|goal_execution_|knowledge_)' --output-on-failure
```

## 12. 实现注意事项

```text
1. Kimi 实现时只能写 P52 范围文件和必要 CMake。
2. 不要改旧 H5。
3. 不要提交推送，完成后等待审核。
4. 不要写死 wolf/torch/fire/wood/beast/companion。
5. fake port 只能在测试中出现。
6. 若 command pipeline 暂无 Attack handler，测试可以用 fake pipeline 验证 command 结构；不要在 P52 写攻击结算。
7. 本能 profile 不是知识，不能直接写 KnowledgeRepository。
8. 学习只保留 experiences，知识形成交给 P49。
```
