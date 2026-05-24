# P63 生命状态与野生 Agent 威胁闭环任务卡设计

## 1. 设计目标

P63 解决当前开放世界原型里的四个断层：

```text
野兽仍像地面物品，而不是会行动的 Agent。
野兽不会主动移动、追击或攻击。
玩家、NPC、野兽没有可投影的生命状态。
P52 野生 Agent 设计没有接入 P53-P62 的正式客户端运行时。
```

本阶段目标不是做完整战斗系统，而是把“地图上的危险生物”接入现有权威链路：

```text
ContentRegistry
-> WorldRuntime actor/entity
-> BeastEcologyCoordinator
-> WorldCommandPipeline
-> Attack/Move handler
-> Runtime state delta / projection patch
-> Client available_commands / event feed
```

完成后，狼、蜘蛛、机械哨兵、魔法召唤物等都可以作为野生 Agent 的内容实例继续扩展，而不是每个生物写一套 if。

## 2. 不解决的问题

- 不做完整装备、护甲、命中率、暴击、死亡掉落和尸体采集，留给后续战斗与掉落阶段。
- 不做复杂寻路，野生 Agent 本阶段只做一格接近、一格远离和相邻攻击。
- 不做群体围猎、仇恨表和阵营外交，后续生态阶段扩展。
- 不让前端判断狼是否攻击、玩家是否受伤、NPC 是否害怕；前端只展示后端投影和事件。
- 不把“狼怕火”“火把驱狼”写死在前端或 command option；本阶段只允许通过内容标签、Agent profile、知识/反应链影响行为。

## 3. 核心概念

| 概念 | 含义 | 权威来源 |
|---|---|---|
| Wild Agent | 非玩家控制的野生行动体，例如狼、野兽暗影 | `ContentRegistry.agents` + `WorldRuntime.actors` |
| Actor Health | 玩家、NPC、野生 Agent 的公开生命状态 | `WorldRuntime` |
| Threat Tick | 玩家命令后由后端推进的一次野生 Agent 行动 | `ClientCommandGateway.post_command_hook` |
| Attack Command | 野生 Agent 或后续战斗 Agent 造成伤害的统一命令 | `WorldCommandPipeline` |
| Social NPC | 可教学、可查看知识、可跟随、可代工的人形/社交 Agent | `ContentRegistry.agents` |

## 4. 枚举设计

本阶段不新增公开枚举，复用已有枚举：

| 枚举 | 使用方式 | 由谁设置 | 由谁读取 |
|---|---|---|---|
| `WorldCommandKind::Attack` | 攻击目标 Actor | `BeastCommandCompiler` / 后续战斗模块 | `AttackCommandHandler` |
| `WorldCommandSource::BeastDecision` | 表示命令来自野生 Agent 决策 | `BeastCommandCompiler` | Command handler / 测试 |
| `WorldCommandResultKind::Succeeded/Blocked` | 攻击或移动结果 | Command handler | 客户端协议、测试 |
| `PatchOp::Update/Remove` | 生命变化、实体移动、实体消失 | Runtime projection adapter | 客户端 |

## 5. 数据结构

### 5.1 `WorldActorRuntime` 追加字段

| 字段 | 类型 | 含义 | 投影 |
|---|---|---|---|
| `max_health` | `int` | 最大生命值 | 安全投影 |
| `health` | `int` | 当前生命值 | 安全投影 |
| `alive` | `bool` | 是否仍可行动 | 安全投影 |

约束：

```text
health ∈ [0, max_health]
max_health >= 1
alive == false 时不能移动，不能攻击，也不应产生社交 NPC 操作。
```

### 5.2 `ActorHealthChangeResult`

| 字段 | 含义 |
|---|---|
| `ok` | 是否成功应用 |
| `actor_key` | 被修改的 Actor |
| `entity_id` | Actor 对应实体 |
| `previous_health` | 旧生命值 |
| `new_health` | 新生命值 |
| `max_health` | 最大生命值 |
| `alive` | 修改后是否存活 |
| `changed_entity_ids` | 投影需要更新的实体 |
| `reason_keys` | 原因键 |

## 6. 接口设计

### 6.1 `IWorldRuntime::applyActorHealthDelta`

```text
输入：actor_key, delta, reason_keys
输出：ActorHealthChangeResult
副作用：修改 WorldActorRuntime.health/alive，并同步 actor entity 的 numeric_states
事件：不直接生成事件，由 Command handler 生成
失败：Actor 不存在、max_health 非法、Actor 已死亡
禁止：调用方直接改 actors_ 或 entities_
```

### 6.2 `createAttackCommandHandler(IWorldRuntime&)`

```text
输入：WorldCommandDto(kind=Attack, target_actor_key)
输出：WorldCommandExecutionResult
副作用：通过 applyActorHealthDelta 扣目标生命
事件：ActorDamaged / ActorDowned
经验：DamageTaken，用于后续认知归因
投影：更新目标 actor entity 的 health/max_health/alive
失败：目标不存在、目标不相邻、目标已倒下、来源非法
```

### 6.3 Beast Tick 接入客户端运行时

```text
ClientCommandGateway.post_command_hook
-> 玩家命令成功后
-> BeastEcologyCoordinator.tick(wild_actor)
-> BeastCommandCompiler 生成 Move / Attack / Wait
-> WorldCommandPipeline 执行 BeastDecision 命令
-> 合并事件、经验、projection patch 到当前客户端响应
```

注意：P63 不允许前端发“狼行动”命令，也不允许前端本地模拟狼移动。

## 7. 状态流转

### 7.1 野兽追击与攻击

```text
Wild Agent alive
-> 感知附近 actor/entity/effect
-> 发现 prey 标签的玩家或 NPC
-> 距离 > 1：生成 Move，一格靠近
-> 距离 <= 1：生成 Attack
-> Attack handler 扣目标 health
-> health 变化进入 projection patch 和 event feed
```

### 7.2 火或威慑影响

```text
Wild Agent 感知到 danger/deterrent tag
-> BeastDecisionPolicy 选择 Flee
-> Flee 编译为一格远离 Move
-> Move 走 WorldCommandPipeline
```

### 7.3 NPC 社交操作过滤

```text
ClientRuntimeCommandOptionBridge 遍历附近 actors
-> 读取 actor.entity_key 对应 AgentTemplate
-> 只有 social/humanoid/can_teach 的 Agent 才显示查看知识、教学、跟随、代工
-> wildlife/predator 不显示 NPC 社交按钮
```

## 8. 事件输出

| 事件 | 触发 |
|---|---|
| `ActorMoved` | 野生 Agent 追击或逃离成功 |
| `ActorDamaged` | Attack 造成生命变化 |
| `ActorDowned` | 目标生命归零 |
| `BeastTickSkipped` | 野生 Agent 无合法行动或被阻塞时可选输出 |

事件必须使用安全文本，不暴露 hidden truth、真实仇恨值或前端不可知状态。

## 9. 配置格式

P63 复用现有内容配置：

```text
content/core/agents/*.json      定义 AgentTemplate
content/core/objects/*.json     定义 safe_tags、显示名、初始 numeric
content/core/threats/*.json     定义威胁基础阶段和反制概念
```

本阶段允许给 `beast_shadow` 的 `initial_state.numeric` 增加安全数值：

```json
{
  "health": 3,
  "max_health": 3,
  "attack_damage": 1
}
```

这些数值是内容数据，不是核心 if。后续狼、熊、机械哨兵只要改内容配置即可改变生命和攻击强度。

## 10. 示例用法

```text
玩家等待
-> post_command_hook 推进 Wild Agent tick
-> 野兽暗影看见玩家
-> 若距离远，野兽通过 Move 靠近
-> 若相邻，野兽通过 Attack 扣玩家生命
-> 客户端收到 ActorDamaged 事件和 player entity health 字段更新
```

```text
玩家教会 NPC 制作火把
-> NPC 制作火把
-> 反应结果产生火焰工具
-> 附近 predator 被已有 NPC counter threat 逻辑逼退
-> destroyEntity 同步移除对应 wild actor，避免被移除实体继续行动
```

## 11. 校验规则

- `Attack` 必须有 `target_actor_key`。
- 攻击者和目标必须存在、存活、同 layer，且八方向相邻或同格。
- `BeastDecision` 只能由后端生成，客户端提交仍只能是 `PlayerInput`。
- `destroyEntity(actor.entity_id)` 必须同步移除对应 actor，避免幽灵 Agent。
- NPC 社交按钮不得出现在 `wildlife` / `predator` / `creature` Agent 上。
- Projection 只能暴露 `health/max_health/alive` 等安全状态，不能暴露真实仇恨表、隐藏 AI 计划。

## 12. 测试点

1. `beast_shadow` 作为 actor 出现在 runtime，而不是只作为可拾取实体。
2. `beast_shadow` 不产生 Pickup option。
3. 野兽距离玩家较远时，等待后会通过 BeastDecision Move 靠近。
4. 野兽相邻玩家时，等待后会通过 BeastDecision Attack 扣玩家 health。
5. 攻击事件进入 `event_feed`，投影含 health/max_health/alive。
6. 玩家/NPC 死亡或倒下后不能继续被当作可行动目标。
7. NPC 社交按钮只对 companion 生成，不对 wild beast 生成。
8. NPC 用火焰工具移除威胁时，destroyEntity 同步移除 wild actor。
9. 所有移动、攻击必须经过 `WorldCommandPipeline` 或 command handler，不直接改坐标/生命。
10. P52/P60/P62 关键测试继续通过。

## 13. 架构复查结论

当前 P52 代码本身符合“野兽决策应编译成 Command”的方向，但正式客户端运行时缺少接线：

```text
已有：BeastEcologyCoordinator / BeastDecisionPolicy / BeastCommandCompiler
缺少：ClientServerRuntimeFactory 中的 profile/world/knowledge/pipeline port
缺少：Attack runtime handler
缺少：WorldRuntime 生命状态接口
缺少：NPC 社交过滤，wild actor 会被误当作普通 NPC
```

P63 的开发重点就是把这些缺口接回现有架构，而不是另起一套前端或临时生态脚本。

## 14. 待定问题

- 完整死亡掉落和尸体对象进入后续战斗/掉落阶段。
- 玩家主动攻击、武器伤害和耐久消耗进入后续战斗阶段。
- 野兽学习“火危险”的长期记忆目前只保留 P52/P49 接口，完整学习曲线后续扩展。
- 大世界多区块野生 Agent 调度、休眠、恢复需要接 P59/P60 区块生命周期。

## 15. 玩法场景头脑风暴

P63 不是只为“狼攻击玩家”服务，而是为了后续所有外部危险 Agent 建立最小权威闭环。设计时按以下场景验证扩展性：

| 场景 | 未来内容例子 | P63 是否支撑 | 后续需要扩展 |
|---|---|---|---|
| 普通野兽 | 狼、野猪、蛇 | 支撑：wildlife actor + Attack | 不同攻击类型、毒、流血 |
| 怕火生物 | 野狼、夜兽、虫群 | 支撑：deterrent tag + Flee | 长期学习火危险 |
| 机械敌人 | 机械哨兵、无人机 | 支撑：AgentTemplate + actor health | 非生物抗性、弹药、远程攻击 |
| 魔法召唤物 | 火元素、冰灵 | 支撑：actor + command | 元素伤害、区域效果 |
| 中立动物 | 鹿、兔子、马 | 支撑：非 social wildlife | 驯服、坐骑、逃跑欲望 |
| 敌对 NPC | 强盗、敌对部落 | 部分支撑：actor health | 阵营、装备、知识与社会行为 |
| 群体生态 | 狼群、蜂群 | 不在 P63 | 群体决策、仇恨表 |
| 区域 Boss | 洞穴巨兽 | 部分支撑 | 多阶段技能、Boss UI |
| 环境灾害 | 会移动的火、毒雾 | 不直接支撑 | AreaEffectTick 与 actor 化灾害选择 |
| 宠物/护卫 | 被驯服的狼 | 部分支撑 | ownership、命令、忠诚度 |

结论：P63 只实现“生命 + 野生 Agent 移动/攻击 + 投影”的最小闭环，不提前把阵营、装备、驯服、远程技能塞进本阶段。这样可以避免一次性做成巨型战斗系统，同时保证架构入口是正确的。

## 16. 玩家体验映射

P63 对玩家体验的贡献不是“多了一个扣血数字”，而是让危险从背景描述变成可观察、可验证、可反制的系统行为：

```text
玩家看到野兽在地图上存在
-> 等待或未来消耗时间的动作推进世界
-> 野兽靠近或攻击
-> 玩家/NPC 生命变化可见
-> 玩家理解火把、逃跑、NPC 协助等行为有实际意义
```

必须保证以下体验对应关系：

| 玩家看到 | 后端真实来源 | 禁止实现 |
|---|---|---|
| 野兽移动 | `BeastDecision -> Move Command -> Runtime` | 前端自己移动 sprite |
| 玩家受伤 | `Attack Command -> applyActorHealthDelta` | 前端本地扣血 |
| 野兽不出现拾取按钮 | option adapter 过滤 actor/creature/danger | 前端隐藏按钮但后端仍允许 |
| NPC 仍可教学/跟随 | social actor 过滤 | 把所有 actor 都当 NPC |
| 血条变化 | projection patch | 前端猜测攻击结果 |

## 17. 模块划分细化

| 模块 | 中文含义 | 职责 | 输入 | 输出 | 禁止事项 |
|---|---|---|---|---|---|
| `WorldActorRuntime` | 运行时行动体 | 保存 actor 坐标、生命、存活状态 | 生成/移动/伤害接口 | actor snapshot | 不允许外部直接改 map |
| `IWorldRuntime::applyActorHealthDelta` | 生命权威修改接口 | 校验并修改 health/alive | actor_key、delta、reason | `ActorHealthChangeResult` | 不生成 UI，不做战斗公式 |
| `AttackCommandHandler` | 攻击命令处理器 | 校验距离并造成伤害 | `WorldCommandDto(Attack)` | event、delta、projection patch | 不直接访问内部数组 |
| `BeastEcologyCoordinator` | 野生 Agent 编排器 | 感知、决策、编译命令 | wild actor、profile、world query | issued command/result | 不绕过 Command |
| `RuntimeBeastProfilePort` | Runtime 到 P52 profile 适配 | 从内容和 runtime 生成 profile | actor/entity/content | `BeastAgentProfile` | 不写前端专属规则 |
| `RuntimeBeastPipelinePort` | 野生命令执行适配 | 将 BeastDecision 命令交给管线 | `WorldCommandDto` | execution result | 不直接调用 handler |
| `ClientRuntimeCommandOptionBridge` | 客户端可用操作生成 | 生成社交、采集、拾取等 options | runtime snapshot | `available_commands` | 不给野兽生成教学/跟随 |
| `WorldProjectionAdapter` | 安全投影 | 暴露 health/max_health/alive | runtime entity | entity patch | 不暴露仇恨表、隐藏 AI |
| `Axmol LocalClientRuntime` | 客户端协议读取 | 读取投影字段 | projection/patch | 本地 view model | 不推导玩法结果 |
| `Axmol MainScene` | 表现层 | 绘制血条 | 本地 view model | 画面反馈 | 不决定伤害和存亡 |

## 18. 数据来源与配置边界

P63 的内容数据来源分三层：

1. `content/core/agents/*.json` 定义这个对象是不是野生 Agent、是否可教学、认知等级和默认策略。
2. `content/core/objects/*.json` 定义地图实体的安全标签、显示名、安全数值。
3. `WorldRuntime` 在实例化 actor 时把内容数值复制为运行时状态。

`beast_shadow` 的 `health/max_health/attack_damage` 必须视为内容数值，而不是核心代码里的固定结论。核心代码只认识这些通用字段的含义：

| 字段 | 含义 | 是否可配置 | 约束 |
|---|---|---|---|
| `health` | 初始生命 | 是 | `0..max_health` |
| `max_health` | 最大生命 | 是 | `>= 1` |
| `attack_damage` | 基础近战伤害 | 是 | `1..1000` |
| `safe_tags` | 被感知/过滤的安全标签 | 是 | 不暴露 hidden truth |

后续新增“熊”时应该新增内容配置，例如：

```json
{
  "object_key": "bear",
  "safe_tags": ["creature", "danger", "predator"],
  "initial_state": {
    "numeric": {
      "health": 12,
      "max_health": 12,
      "attack_damage": 3
    }
  }
}
```

同时在 `agents` 中增加 `agent_key=bear` 且 `embodiment=wildlife`，不应新增 `if (key == "bear")`。

## 19. Command 管线规则

P63 里有两类命令来源：

| 来源 | 谁生成 | 能否由客户端提交 | 用途 |
|---|---|---|---|
| `PlayerInput` | 客户端通过 `available_commands` materialize | 能，但必须匹配 session snapshot | 玩家操作 |
| `BeastDecision` | 后端 `BeastCommandCompiler` | 不能 | 野生 Agent 自主行动 |

客户端提交 `WorldCommandDto` 时必须仍被 P53 规则约束：

```text
actor_key 必须等于 session actor。
source 必须是 PlayerInput。
command kind/key/target 必须匹配 latest available_commands snapshot。
parameters 必须为空，避免伪造 damage 等参数。
```

野生 Agent 的 `Attack` 可以携带 `damage` 参数，但这只能由后端 `BeastCommandCompiler` 生成，并通过 `RuntimeBeastPipelinePort` 进入 `WorldCommandPipeline`。

## 20. Threat Tick 的临时边界

P63 当前只在 `Wait` 成功后推进 wild tick。原因：项目尚未有统一时间/回合/动作耗时系统。

如果直接在所有命令后推进，会出现严重问题：

```text
教学命令推进野兽 -> 教学测试被生态打断。
代工命令推进野兽 -> NPC 任务半途中被不可控插入。
刷新/查看类命令推进野兽 -> 玩家只是看信息也被攻击。
移动/采集/制作到底消耗多少时间没有统一规则。
```

因此本阶段采用安全边界：

```text
Wait = 显式让时间推进。
Move/Gather/Craft/Teach/Inspect 暂不推进 wild tick。
```

后续必须新增统一 `TimeCost/TurnAdvance` 阶段，再决定哪些 command 消耗时间、消耗多少、是否触发外部打断。不能在 P63 里临时扩大触发范围。

## 21. 生命周期与区块风险

P63 的 wild actor 已进入 `WorldRuntime.actors`，但大世界区块生命周期仍有后续风险：

| 风险 | 当前处理 | 后续阶段 |
|---|---|---|
| 区块卸载后 wild actor 是否休眠 | 未完整处理 | 接 P59/P60 lifecycle |
| 区块恢复后 actor 坐标和 health 是否恢复 | 依赖现有 runtime snapshot，未做完整存档 | 后续 runtime actor seal/restore |
| actor entity 被销毁但 actor 仍存在 | P63 修 `destroyEntity` 同步移除 actor | 已处理最小风险 |
| actor 死亡后掉落 | 未做 | 战斗/掉落阶段 |
| 死亡尸体是否仍可采集 | 未做 | 尸体对象阶段 |

这部分不能由前端补，也不能通过“隐藏 entity”假装解决。

## 22. 知识与 Agent 认知关系

P63 只把攻击和受伤产出 `damage_taken` experience，供后续知识归因使用；它不在本阶段做完整“狼攻击会受伤”的学习闭环。

原因：知识形成需要回答：

```text
谁观察到了攻击？
受伤归因给攻击者、环境还是状态？
NPC 是否能从旁观学习？
野兽是否能学习火危险？
知识是否可教学？
```

这些问题属于 P49/P52/P40/P41 链式推理和经验归因组合，不应在 P63 的 Attack handler 里硬写结论。P63 只保证 experience 存在，让后续知识系统有材料可接。

## 23. 测试设计细化

### 23.1 必跑定向测试

```text
world_beast_ecology
client_runtime_bridge_wildlife_actor_chases_and_attacks
client_runtime_bridge_pickup_options_exclude_predator
client_runtime_bridge_npc_follow_command_moves_companion
client_runtime_bridge_npc_order_torch_counters_threat_after_teaching
```

### 23.2 应覆盖的断言

| 断言 | 目的 |
|---|---|
| `findActor("beast_shadow")` 成功 | 野兽是 actor，不只是 entity |
| `scenario_beast_shadow` entity 存在 | 保持威胁反制兼容 |
| pickup options 不包含 predator | 野兽不可拾取 |
| wait 后出现 Move 或 Attack | 野生 Agent 走管线行动 |
| attack 后 health 下降 | 生命状态真实修改 |
| projection patch 有 health/max_health/alive | 前端可安全展示 |
| 教学/跟随测试仍通过 | wild tick 不污染社交命令 |
| 火把反制后 actor 被移除 | destroyEntity 同步 actors |

### 23.3 自检扫描

应人工或脚本检查：

```text
前端不得出现狼是否攻击的判断。
后端不得新增 wolf/bear/torch 专属攻击分支。
Attack 不得直接改 actors_ 容器外部引用。
ClientCommandGateway 仍禁止客户端提交 BeastDecision。
RuntimeBeastProfilePort 中内容标签映射若继续扩展，必须迁移配置。
```

## 24. P63 完成标准

P63 验收不是“看到血条”即可，而是必须同时满足：

1. 野兽以 Agent/actor 身份存在。
2. 野兽行动由 P52 决策模块产生。
3. 野兽命令进入 `WorldCommandPipeline`。
4. 移动和攻击由 runtime command handler 执行。
5. 生命状态由 runtime 权威接口修改。
6. 前端只显示投影，不写规则。
7. NPC 社交操作不把 wildlife 当可教学对象。
8. 旧知识、教学、制作、跟随、采集链路不被破坏。
9. 文档明确当前只在 Wait 后 tick，避免后续误扩展。
10. 遗留风险进入后续阶段，而不是隐藏在代码里。

## 25. 后续阶段建议

P64 或后续阶段建议优先做“统一时间/动作耗时与外部打断”：

```text
Command metadata -> time_cost
Command result -> world_time_advanced
World tick scheduler -> wildlife/area_effect/weather/need decay
Interrupt queue -> wolf attack / weather change / NPC emergency
Projection -> 事件和状态变化
```

只有这个阶段完成后，才能合理回答：移动一步是否让狼行动一次、砍树是否消耗半回合、制作火把是否让夜晚推进、NPC 代工过程中遇到狼是否被打断。

再后续才适合做：

- 玩家主动攻击和武器伤害。
- 护甲、闪避、命中、暴击。
- 死亡、尸体、掉落和采集。
- 阵营与敌对 NPC。
- 驯服、坐骑和护卫。
- 大世界区块休眠/恢复 wild actor。
- 野生 Agent 对火、陷阱、玩家行为的长期学习。

## 26. 追加修正：威慑后的自主游走

P63 初版只有靠近、攻击、逃离和等待。试玩反馈说明：如果野生 Agent 看到威慑源，它不能只是停住，也不能在刚逃离后立刻重新直线冲向玩家。

修正规则：

```text
近距离威慑源（距离 <= 2） -> Flee，一格远离。
仍可见威慑源但已不贴近 -> Wander，随机性游走并尽量不靠近威慑源。
不可见威慑源且看见 prey -> Approach / Attack。
没有目标且有坐标 -> Wander。
没有目标且无坐标 -> Wait。
```

这个修正仍然遵守架构边界：

```text
BeastDecisionPolicy 只产出 intent。
BeastCommandCompiler 把 Wander(target_coord) 编译成 Move command。
WorldCommandPipeline 和 Runtime 仍是移动权威。
前端只提交自动 Wait tick 和显示投影，不模拟狼移动。
```

后续如果要做更真实的生态游走，应把随机游走升级为内容可配置的 `movement_profile`，例如巡逻、盘旋、领地边缘徘徊、群体围绕，而不是在策略里继续堆内容专属分支。
