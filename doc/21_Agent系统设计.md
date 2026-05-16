# 21_Agent系统设计

## 1. 设计目标

Agent 系统负责回答一个核心问题：

```text
在规则引擎之外，谁来观察世界、形成意图、选择行动，并把行动提交给 RuleEngine。
```

前面阶段已经设计了：

```text
世界有什么。
对象有什么特性。
行为如何提交。
规则如何结算。
反馈如何产生。
认知、记忆、知识如何变化。
传播、族群、文明如何推进。
存档、复盘、协议如何保证权威性。
```

但还缺一个抽象：

```text
谁基于这些信息做选择。
```

Agent 系统就是这个选择层。

它既可以代表玩家，也可以代表普通族人、敌对族群、野生生物、脚本控制者、调试控制者、自动测试控制者、强化学习训练体。

核心原则：

```text
Agent 只产生意图，不直接修改世界。
Agent 只能通过 CommandEnvelope 调用 RuleEngine。
Agent 看到的不是全量真实状态，而是符合权限、认知和观察范围的投影。
Agent 决策必须可记录、可复盘、可测试、可替换。
```

本章要保证：

```text
玩家和 AI 使用同一套行为命令入口。
AI 不能绕过规则管线作弊。
强化学习后续能接入统一观察、动作、奖励、终止信号。
普通 NPC、族群、敌对群体、脚本事件都能用同一套 Agent 抽象。
Agent 行为可以被存档、复盘、调试、测试。
后续新增决策算法不推翻现有规则引擎架构。
```

## 2. 不解决的问题

| 不解决的问题 | 原因 | 后续阶段 |
|---|---|---|
| 具体 AI 算法 | 本章只定义接口，不绑定行为树、GOAP、效用 AI、神经网络 | AI 实现阶段 |
| 强化学习训练算法 | 本章只预留 Observation / Action / Reward / Done | 强化学习阶段 |
| NPC 剧情对白 | 本章只处理决策抽象 | 内容系统阶段 |
| 前端 UI 操作布局 | 本章只定义玩家 Agent 如何提交意图 | UI/UX 阶段 |
| 多人联网同步 | 本章只预留 session 和 controller | 网络阶段 |
| 战斗详细战术 AI | 本章只定义共同作战和行动选择接口 | 战斗 AI 阶段 |
| 大模型接入 | 本章只把 LLM 看成一种 Policy 实现 | 后续平台阶段 |

禁止做法：

```text
禁止 Agent 直接修改 GameState。
禁止 Agent 直接调用内部 Resolver。
禁止 Agent 读取 DebugOnly / Hidden 数据，除非拥有调试权限。
禁止 Agent 绕过 CognitionProjection 获得真实知识。
禁止 AI 和玩家使用两套不同的行为规则。
禁止把 Agent 的预测结果当成规则结算结果。
禁止 Agent 在复盘时使用当前版本重新思考替代历史决策。
禁止强化学习环境直接改状态来加速训练。
```

## 3. 核心定位

### 3.1 Actor 和 Agent 的区别

| 概念 | 中文 | 职责 | 是否能决策 | 是否能被规则结算 |
|---|---|---|---|---|
| Actor | 行动者 | 行为命令中的执行实体 | 否 | 是 |
| Agent | 智能体 / 决策体 | 观察、思考、选择行为 | 是 | 否 |
| Controller | 控制器 | 连接输入来源和 Agent | 是 | 否 |
| Policy | 策略 | 具体决策算法 | 是 | 否 |
| RuleEngine | 规则引擎 | 权威结算行为结果 | 否 | 是 |

解释：

```text
Actor 是规则世界里的实体，例如先驱者、族人、野兽、敌人。
Agent 是外部决策层，可以控制一个或多个 Actor。
一个 Agent 可以控制一个 Actor，也可以控制一个族群、一支队伍或一个系统脚本。
一个 Actor 在同一时刻只能被一个权威 Agent 控制。
RuleEngine 不关心 Agent 怎么思考，只关心收到的 CommandEnvelope 是否合法。
```

### 3.2 Agent 不是规则层

Agent 可以做：

```text
读取允许范围内的观察。
读取自己的认知、记忆、知识投影。
评估多个候选行动。
生成 ActionIntent。
提交 CommandEnvelope。
记录决策理由和调试信息。
```

Agent 不可以做：

```text
直接改变对象状态。
直接增加知识。
直接修改族群状态。
直接解锁文明能力。
直接产生最终事件。
直接跳过错误码和前置条件。
```

## 4. 核心概念总览

| 类型 | 中文 | 作用 |
|---|---|---|
| AgentId | Agent ID | 智能体唯一标识 |
| AgentDefinitionId | Agent 定义 ID | 配置层智能体模板 |
| AgentRuntimeId | Agent 运行时 ID | 一次运行中的 Agent 实例 |
| AgentDefinition | Agent 定义 | 描述 Agent 类型、权限、策略、目标 |
| AgentScale | Agent 尺度 | 微型、小型、个体、族群、文明、星际等尺度 |
| AgentCognitionBand | Agent 认知层级 | 反射、本能、联想、工具、社会、抽象、文明等认知能力 |
| AgentEmbodiment | Agent 具身形式 | 单体、多体、分布式、远程影响或系统具身 |
| AgentState | Agent 状态 | 当前决策状态、冷却、目标、记忆引用 |
| AgentProfile | Agent 画像 | 风格、风险偏好、学习能力、社交倾向 |
| AgentBinding | Agent 绑定 | Agent 与 Actor / Tribe / Session 的关系 |
| AgentController | Agent 控制器 | 输入来源：玩家、AI、脚本、训练、系统 |
| AgentPolicy | Agent 策略 | 决策算法接口 |
| AgentObservation | Agent 观察 | Agent 可见的主观输入 |
| AgentActionSpace | Agent 动作空间 | 当前可选行为集合 |
| AgentIntent | Agent 意图 | 尚未提交的行动意图 |
| AgentDecision | Agent 决策 | 从观察到意图的完整决策结果 |
| AgentCommandAdapter | 命令适配器 | 把 AgentIntent 转成 CommandEnvelope |
| AgentDecisionTrace | 决策轨迹 | 记录为什么做这个选择 |
| AgentRewardSignal | 奖励信号 | 强化学习和行为评估使用 |
| AgentRuntime | Agent 运行时 | 管理 Agent tick、调度、暂停、复盘 |
| AgentService | Agent 服务 | 外部管理 Agent 的接口 |

### 4.1 Agent 必须支持尺度扩展

Agent 不是只给人类或玩家使用。

本游戏中所有“能基于自身认知做选择”的对象，都可以被建模为 Agent：

```text
狼是小型 Agent。
蜘蛛是微型 Agent。
先驱者个人是个体 Agent。
人类族群是大型 Agent。
高级文明外星人可以是顶级 Agent。
高级文明舰队、蜂巢意识、星际文明也可以是超尺度 Agent。
```

核心原则：

```text
Agent 的强弱不由是否叫 Agent 决定，而由 AgentScale、CognitionBand、ActionSpace、Policy 和权限共同决定。
```

因此不能把狼、蜘蛛、人类、外星文明写成四套不同架构。

它们必须共享：

```text
Observation。
ActionSpace。
Intent。
Decision。
CommandEnvelope。
RuleEngine。
Replay。
Test。
```

区别只在：

```text
观察范围不同。
认知复杂度不同。
动作空间不同。
目标系统不同。
策略能力不同。
记忆和知识容量不同。
是否能传播知识不同。
是否能跨个体、族群、文明做决策不同。
```


### 4.2 Agent 可以形成层级控制

Agent 支持小 Agent 被大 Agent 管理，但不能让大 Agent 直接跳过小 Agent 的规则边界。

```text
蜘蛛个体 Agent 可以组成蜘蛛群 Pack Agent。
狼个体 Agent 可以组成狼群 Pack Agent。
人类个体 Agent 可以组成 Tribe Agent。
多个 Tribe Agent 可以组成 Civilization Agent。
高级外星个体、舰队、文明可以组成 Interstellar Agent。
```

层级关系使用 `AgentHierarchyLink` 表达。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| parent_agent_id | AgentId | 上级 Agent | 群体、族群、文明或外星文明 |
| child_agent_id | AgentId | 下级 Agent | 个体、小队、族群或舰队 |
| relation_type | string | 关系类型 | command / influence / observe / coordinate |
| authority_weight | float | 权威权重 | 上级对下级决策影响程度 |
| autonomy_limit | float | 自主限制 | 下级保留多少自主性 |
| valid_condition | optional<ExpressionRef> | 生效条件 | 例如距离、通讯、忠诚、科技能力 |

约束：

```text
上级 Agent 可以影响下级目标、优先级和动作候选。
下级 Agent 仍必须通过自己的 AgentIntent 和 CommandEnvelope 行动。
如果下级 Agent 有自主性，可以拒绝、延迟或误解上级意图。
文明 Agent 不能直接把某个个体的生命值改掉，只能发起命令、政策、战争、教学、迁移、建设等意图。
```

### 4.3 小 Agent、大 Agent、顶级 Agent 的设计边界

| 级别 | 示例 | 决策特点 | 不能做什么 |
|---|---|---|---|
| 小 Agent | 蜘蛛、狼、野兽 | 局部感知、短期生存、本能或联想决策 | 不能使用抽象文明知识 |
| 中 Agent | 先驱者、族人、敌人个体 | 探索、尝试、记忆、学习、战斗、教学 | 不能绕过规则直接获得真实答案 |
| 大 Agent | 人类族群、敌对族群、狼群 | 群体目标、共同作战、迁移、传播、分裂风险 | 不能无视个体状态和族群条件 |
| 顶级 Agent | 高级文明外星人、星际舰队、蜂巢文明 | 跨区域、跨文明、超长周期策略 | 不能越过权限、科技、距离和规则管线 |

核心结论：

```text
所有尺度都共用 Agent 架构。
越高级的 Agent 只拥有更大的 Observation、ActionSpace、Goal 和 Policy，不拥有改状态特权。
```

## 5. 枚举设计

### 5.1 AgentType（Agent 类型）

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| Player | player | 玩家 Agent | 由真实玩家输入驱动 |
| Companion | companion | 同伴 Agent | 族人或伙伴个体 |
| Tribe | tribe | 族群 Agent | 控制族群级策略 |
| EnemyTribe | enemy_tribe | 敌对族群 Agent | 控制敌对群体 |
| Creature | creature | 生物 Agent | 野兽、危险生物 |
| Environment | environment | 环境 Agent | 周期性环境或灾害决策 |
| Script | script | 脚本 Agent | 剧情、教程或测试脚本 |
| Training | training | 训练 Agent | 强化学习或批量模拟 |
| Debug | debug | 调试 Agent | GM / 调试工具 |
| System | system | 系统 Agent | 内部系统调度 |

使用规则：

```text
Player 只能由合法 session 驱动。
Debug 必须拥有 debug 权限。
Training 只能在训练运行时或沙盒环境中启用。
Environment 不代表物体本身，只代表环境规则的决策入口。
```

### 5.2 AgentScale（Agent 尺度）

`AgentScale` 表示 Agent 决策覆盖的生命、群体或文明尺度。

| 枚举 | 值 | 中文 | 示例 | 说明 |
|---|---|---|---|---|
| Micro | micro | 微型智能体 | 蜘蛛、昆虫、寄生体 | 动作简单，感知局部，目标短期 |
| SmallCreature | small_creature | 小型生物智能体 | 狼、蛇、野兽 | 有生存、攻击、逃跑、领地行为 |
| Individual | individual | 个体智能体 | 先驱者、族人、外星个体 | 可探索、学习、记忆、教学 |
| Pack | pack | 群体智能体 | 狼群、蜘蛛群、狩猎小队 | 多个 Actor 的协同行为 |
| Tribe | tribe | 族群智能体 | 人类部落、敌对族群 | 族群策略、传播、共同作战、分裂风险 |
| Civilization | civilization | 文明智能体 | 高级人类文明、外星文明 | 跨区域、技术、外交、战争决策 |
| Planetary | planetary | 行星级智能体 | 星球生态意识、行星 AI | 大范围环境与文明级影响 |
| Interstellar | interstellar | 星际级智能体 | 高级外星文明、星际舰队 | 跨星系行动、超长周期目标 |
| Abstract | abstract | 抽象智能体 | 命运系统、神秘观察者 | 非普通生物，但仍通过规则提交意图 |

使用规则：

```text
AgentScale 不代表强度等级，也不直接解锁能力。
AgentScale 只决定默认观察范围、动作空间规模、目标周期和调度频率。
狼和蜘蛛可以是 Agent，但它们的 CognitionBand、ActionSpace、MemoryCapacity 会很低。
外星文明可以是 Agent，但它也不能绕过 RuleEngine 直接改世界。
```

### 5.3 AgentCognitionBand（Agent 认知层级）

`AgentCognitionBand` 表示 Agent 能理解和使用知识的复杂程度。

| 枚举 | 值 | 中文 | 示例 | 说明 |
|---|---|---|---|---|
| Reflex | reflex | 反射层 | 蜘蛛避火、昆虫趋光 | 几乎不推理，只做刺激反应 |
| Instinct | instinct | 本能层 | 狼捕猎、野兽逃跑 | 有固定生存策略和简单记忆 |
| Associative | associative | 联想层 | 动物记住食物和危险 | 能把对象、行为、反馈建立关联 |
| ToolAware | tool_aware | 工具认知层 | 原始人使用木棍、火把 | 能理解可使用、可制造、可传授 |
| SocialLearning | social_learning | 社会学习层 | 部落成员学习知识 | 能模仿、教学、传播、形成群体认知 |
| AbstractReasoning | abstract_reasoning | 抽象推理层 | 高级人类、研究者 | 能使用条件化知识和长期计划 |
| Civilizational | civilizational | 文明认知层 | 国家、文明、外星文明 | 能进行制度、技术、战争、外交决策 |
| Transcendent | transcendent | 超越认知层 | 顶级外星文明、高维观察者 | 能处理超长周期、跨尺度、复杂隐藏信息 |

使用规则：

```text
CognitionBand 决定 Agent 能使用哪些 CognitionEntry、KnowledgeClaim 和计划深度。
低认知层 Agent 也能学习，但学习形式更像条件反射或经验关联。
高认知层 Agent 可以做抽象计划，但仍必须通过规则引擎提交行为。
```

### 5.4 AgentEmbodiment（Agent 具身形式）

`AgentEmbodiment` 表示 Agent 如何存在于世界中。

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| SingleBody | single_body | 单一身体 | 一个 Agent 控制一个 Actor |
| MultiBody | multi_body | 多身体 | 一个 Agent 控制多个 Actor，如狼群 |
| Distributed | distributed | 分布式 | 族群、蜂巢、文明共享决策 |
| RemoteInfluence | remote_influence | 远程影响 | 外星文明、神秘力量远程下达意图 |
| Systemic | systemic | 系统具身 | 环境、生态、灾害类 Agent |

约束：

```text
MultiBody / Distributed Agent 必须通过 AgentBinding 明确可控制对象。
RemoteInfluence 也不能无视距离、权限、科技或规则条件。
```

### 5.5 AgentControlMode（控制模式）

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| Manual | manual | 手动 | 玩家或人工输入 |
| Assisted | assisted | 辅助 | AI 给建议，玩家确认 |
| Autonomous | autonomous | 自主 | AI 自动行动 |
| Scripted | scripted | 脚本 | 固定脚本驱动 |
| Training | training | 训练 | 训练算法驱动 |
| ReplayLocked | replay_locked | 回放锁定 | 复盘时只重放历史命令 |
| Disabled | disabled | 禁用 | 暂停决策 |

注意：

```text
ReplayLocked 模式下 Agent 不能重新决策，只能读取历史 AgentDecisionRecord。
Assisted 模式产生建议，不自动提交命令。
```

### 5.6 AgentDecisionStatus（决策状态）

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| Idle | idle | 空闲 | 暂无决策 |
| Observing | observing | 观察中 | 正在收集观察 |
| Planning | planning | 规划中 | 正在生成候选行动 |
| WaitingInput | waiting_input | 等待输入 | 等玩家或外部控制器输入 |
| IntentReady | intent_ready | 意图就绪 | 已产生意图，未提交 |
| Submitted | submitted | 已提交 | 已提交 CommandEnvelope |
| Blocked | blocked | 被阻断 | 无合法动作或条件不满足 |
| Cooldown | cooldown | 冷却中 | 行动间隔未结束 |
| Error | error | 错误 | 决策或适配失败 |

### 5.7 AgentPolicyType（策略类型）

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| HumanInput | human_input | 人类输入 | 来自前端操作 |
| RuleBased | rule_based | 规则策略 | 固定规则和优先级 |
| Utility | utility | 效用策略 | 对候选动作打分 |
| BehaviorTree | behavior_tree | 行为树 | 树状 AI |
| GoalOriented | goal_oriented | 目标导向 | GOAP 类规划 |
| ScriptSequence | script_sequence | 脚本序列 | 固定步骤 |
| RandomValid | random_valid | 随机合法 | 测试或探索使用 |
| ReinforcementLearning | reinforcement_learning | 强化学习 | 模型或训练器输出动作 |
| Remote | remote | 远程策略 | 外部服务返回决策 |
| Null | null | 空策略 | 不行动 |

### 5.8 AgentVisibilityMode（可见性模式）

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| CognitionOnly | cognition_only | 只看认知 | 只看主体已知或相信的内容 |
| PerceptionLimited | perception_limited | 感知限制 | 受距离、环境、状态影响 |
| TribeShared | tribe_shared | 族群共享 | 可使用族群共享知识 |
| OmniscientDebug | omniscient_debug | 调试全知 | 仅 Debug 权限 |
| TrainingMasked | training_masked | 训练遮罩 | 训练时统一裁剪观察 |

默认：

```text
普通 Agent 使用 CognitionOnly + PerceptionLimited。
训练 Agent 使用 TrainingMasked，保证观察空间稳定。
Debug Agent 才允许 OmniscientDebug。
```

### 5.9 AgentGoalStatus（目标状态）

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| Candidate | candidate | 候选 | 可能目标 |
| Active | active | 激活 | 当前目标 |
| Suspended | suspended | 暂停 | 暂时不可执行 |
| Completed | completed | 完成 | 目标达成 |
| Failed | failed | 失败 | 无法完成 |
| Abandoned | abandoned | 放弃 | 主动放弃 |

### 5.10 AgentActionSelectionMode（动作选择模式）

| 枚举 | 值 | 中文 | 说明 |
|---|---|---|---|
| SingleBest | single_best | 选最高分 | 选择最高评分动作 |
| WeightedRandom | weighted_random | 加权随机 | 按权重随机选择 |
| ExploreFirst | explore_first | 优先探索 | 优先未知或低置信目标 |
| RiskAvoidant | risk_avoidant | 风险规避 | 避免高风险动作 |
| RiskSeeking | risk_seeking | 风险偏好 | 接受高风险高收益 |
| PlayerChosen | player_chosen | 玩家选择 | 玩家指定 |
| ReplayChosen | replay_chosen | 回放选择 | 历史记录指定 |

## 6. AgentDefinition

`AgentDefinition` 是配置层模板，用于定义某类 Agent 的默认行为边界。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| agent_definition_id | AgentDefinitionId | Agent 定义 ID | 稳定配置 ID |
| agent_type | AgentType | Agent 类型 | 玩家、族群、生物等 |
| agent_scale | AgentScale | Agent 尺度 | 微型、小型、个体、族群、文明等 |
| cognition_band | AgentCognitionBand | 认知层级 | 决定可理解知识和计划深度 |
| embodiment | AgentEmbodiment | 具身形式 | 单体、多体、分布式、远程影响等 |
| default_control_mode | AgentControlMode | 默认控制模式 | 初始控制方式 |
| default_policy_type | AgentPolicyType | 默认策略类型 | 默认决策算法 |
| visibility_mode | AgentVisibilityMode | 可见性模式 | 决定观察裁剪方式 |
| allowed_command_types | list<ActionType> | 允许行为 | 可生成的行为类型 |
| forbidden_command_types | list<ActionType> | 禁止行为 | 即使策略生成也要拒绝 |
| decision_interval | Duration | 决策间隔 | 多久可决策一次 |
| max_actions_per_tick | int | 每 tick 最大动作 | 防止爆量提交 |
| profile_template | AgentProfile | 画像模板 | 默认偏好和能力 |
| policy_config_ref | ConfigRef | 策略配置引用 | 指向 AI 参数配置 |
| reward_config_ref | optional<ConfigRef> | 奖励配置引用 | 训练或评估使用 |
| memory_capacity_hint | int | 记忆容量提示 | 决定默认可保留经验规模 |
| planning_depth_hint | int | 计划深度提示 | 决定默认前瞻步数 |
| debug_tags | list<string> | 调试标签 | 便于过滤 |

约束：

```text
AgentDefinition 不保存运行时状态。
allowed_command_types 只定义上限，最终合法性仍由 RuleEngine 判断。
Debug / Training 类型必须显式配置权限边界。
agent_scale、cognition_band、embodiment 只能影响默认能力边界，不能绕过规则。
```

## 7. AgentProfile

`AgentProfile` 描述 Agent 的长期行为倾向，不代表世界状态。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| curiosity | float | 好奇心 | 越高越愿意尝试未知对象 |
| caution | float | 谨慎度 | 越高越避免风险 |
| aggression | float | 攻击性 | 越高越倾向战斗 |
| cooperation | float | 合作性 | 越高越愿意共同作战或教学 |
| teaching_bias | float | 教学倾向 | 越高越主动传播知识 |
| imitation_bias | float | 模仿倾向 | 越高越学习他人行为 |
| memory_reliance | float | 记忆依赖 | 越高越依赖历史经验 |
| novelty_preference | float | 新奇偏好 | 越高越探索低置信目标 |
| risk_tolerance | float | 风险承受 | 越高越能接受负面反馈概率 |
| tribe_loyalty | float | 族群忠诚 | 越高越维护本族利益 |
| split_tendency | float | 分裂倾向 | 越高越容易与族群目标冲突 |
| abstraction_bias | float | 抽象推理倾向 | 越高越愿意做长期、条件化、跨对象推理 |
| territoriality | float | 领地性 | 越高越保护区域、巢穴或势力范围 |
| survival_priority | float | 生存优先级 | 越高越优先吃、逃、躲避危险 |

取值约定：

```text
所有 profile 数值默认范围为 0.0 到 1.0。
越界配置必须由配置校验拒绝。
Profile 只影响决策评分，不能绕过规则条件。
```

## 8. AgentState

`AgentState` 是 Agent 的运行时状态。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| agent_id | AgentId | Agent ID | 实例标识 |
| definition_id | AgentDefinitionId | 定义 ID | 来源模板 |
| runtime_id | AgentRuntimeId | 运行时 ID | 当前运行实例 |
| status | AgentDecisionStatus | 决策状态 | 当前状态 |
| control_mode | AgentControlMode | 控制模式 | 当前控制方式 |
| policy_type | AgentPolicyType | 策略类型 | 当前策略 |
| bound_actor_ids | list<EntityId> | 绑定行动者 | 可控制实体 |
| bound_tribe_ids | list<TribeId> | 绑定族群 | 可控制族群 |
| owner_session_id | optional<SessionId> | 所属会话 | 玩家或远程控制 |
| active_goal_ids | list<AgentGoalId> | 当前目标 | 正在追求的目标 |
| cooldown_until_tick | Tick | 冷却截止 | 冷却前不可行动 |
| last_decision_id | optional<AgentDecisionId> | 最近决策 | 追踪用 |
| last_command_id | optional<CommandId> | 最近命令 | 与规则引擎关联 |
| deterministic_seed | RandomSeed | 确定性种子 | 决策随机源 |
| local_blackboard | AgentBlackboard | 局部黑板 | 决策临时信息 |

注意：

```text
AgentState 可以存档。
local_blackboard 只能保存决策辅助信息，不能保存权威世界事实。
权威事实必须来自 GameState、CognitionState、Memory、Knowledge、TribeState。
```

## 9. AgentBinding

`AgentBinding` 描述 Agent 控制范围。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| agent_id | AgentId | Agent ID | 控制者 |
| binding_type | string | 绑定类型 | actor / tribe / team / session / system |
| target_id | string | 目标 ID | EntityId、TribeId、SessionId 等 |
| authority | string | 权限 | primary / assistant / observer |
| priority | int | 优先级 | 冲突时使用 |
| valid_from_tick | Tick | 生效 tick | 开始时间 |
| valid_until_tick | optional<Tick> | 失效 tick | 可选 |
| permission_profile | ClientPermissionProfile | 权限画像 | 可见性和操作边界 |

约束：

```text
同一个 Actor 同一 tick 只能有一个 primary Agent。
assistant Agent 只能建议，不能直接提交最终命令，除非 primary 授权。
observer Agent 只能观察，不能行动。
```

## 10. AgentObservation

`AgentObservation` 是 Agent 决策输入，不是完整 GameState。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| observation_id | ObservationId | 观察 ID | 用于追踪 |
| agent_id | AgentId | Agent ID | 观察者 |
| state_version | StateVersion | 状态版本 | 观察对应状态 |
| config_version_id | ConfigVersionId | 配置版本 | 观察对应配置 |
| tick | Tick | 当前 tick | 时间 |
| visible_objects | list<ObjectProjection> | 可见对象投影 | 已按认知和感知裁剪 |
| visible_actors | list<ActorProjection> | 可见行动者 | 可见实体摘要 |
| cognition_projection | CognitionProjection | 认知投影 | 主观认知 |
| memory_projection | MemoryProjection | 记忆投影 | 可用记忆摘要 |
| knowledge_projection | KnowledgeProjection | 知识投影 | 可用知识摘要 |
| tribe_projection | optional<TribeProjection> | 族群投影 | 族群态势 |
| civilization_projection | optional<CivilizationProjection> | 文明投影 | 能力可用摘要 |
| recent_events | list<EventProjection> | 最近事件 | 按权限过滤 |
| available_actions_hint | list<ActionType> | 可行动作提示 | UI / AI 辅助，不代表最终合法 |
| reward_hint | optional<AgentRewardSignal> | 奖励提示 | 训练环境可用 |

观察裁剪顺序：

```text
1. 权限过滤。
2. 感知范围过滤。
3. 认知投影过滤。
4. 事件可见性过滤。
5. 训练遮罩过滤。
```

## 11. AgentActionSpace

`AgentActionSpace` 表示当前 Agent 可以尝试生成的动作集合。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| agent_id | AgentId | Agent ID | 所属 Agent |
| state_version | StateVersion | 状态版本 | 生成时版本 |
| candidate_actions | list<AgentActionCandidate> | 候选动作 | 可尝试动作 |
| blocked_actions | list<AgentActionBlockReason> | 被阻断动作 | 为什么不可尝试 |
| action_mask | optional<ActionMask> | 动作遮罩 | 强化学习使用 |
| generation_trace | TraceId | 生成轨迹 | 调试用 |

`AgentActionCandidate` 字段：

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| candidate_id | string | 候选 ID | 本次决策内唯一 |
| action_type | ActionType | 行为类型 | Eat / Use / Attack / Teach 等 |
| actor_id | EntityId | 行动者 | 谁执行 |
| target | ActionTarget | 目标 | 行为目标 |
| parameters | ActionParameters | 参数 | 行为参数 |
| expected_utility | float | 预期效用 | 策略评分 |
| expected_risk | float | 预期风险 | 主观风险 |
| novelty_score | float | 新奇度 | 探索价值 |
| knowledge_gain_score | float | 知识收益 | 可能形成新认知 |
| cooperation_score | float | 协作价值 | 对共同目标帮助 |
| reason_tags | list<string> | 理由标签 | 便于调试 |

注意：

```text
ActionSpace 是 Agent 的主观候选集合。
RuleEngine 仍可能拒绝其中任何动作。
```

## 12. AgentIntent

`AgentIntent` 是 Agent 准备提交给规则引擎的意图。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| intent_id | AgentIntentId | 意图 ID | 唯一标识 |
| agent_id | AgentId | Agent ID | 决策来源 |
| selected_candidate_id | string | 候选 ID | 来源候选 |
| action_type | ActionType | 行为类型 | 最终选择 |
| actor_id | EntityId | 行动者 | 执行动作者 |
| target | ActionTarget | 目标 | 目标 |
| parameters | ActionParameters | 参数 | 参数 |
| priority | int | 优先级 | 调度冲突时使用 |
| expected_state_version | StateVersion | 期望版本 | 防止旧观察提交 |
| idempotency_key | IdempotencyKey | 幂等键 | 防止重复提交 |
| decision_reason | string | 决策理由 | 可本地化摘要键或调试文本 |
| confidence | float | 决策置信度 | 策略对选择的信心 |

约束：

```text
AgentIntent 不是 CommandEnvelope。
AgentCommandAdapter 必须把它转换成合法 CommandEnvelope。
转换失败必须返回错误码，不允许静默丢弃。
```

## 13. AgentDecision

`AgentDecision` 记录从观察到意图的完整决策结果。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| decision_id | AgentDecisionId | 决策 ID | 唯一标识 |
| agent_id | AgentId | Agent ID | 决策者 |
| observation_id | ObservationId | 观察 ID | 输入观察 |
| policy_type | AgentPolicyType | 策略类型 | 使用策略 |
| selection_mode | AgentActionSelectionMode | 选择模式 | 如何选出动作 |
| candidates | list<AgentActionCandidate> | 候选 | 评估过的动作 |
| selected_intent | optional<AgentIntent> | 选择意图 | 可能为空 |
| status | AgentDecisionStatus | 状态 | 结果状态 |
| random_draws | list<RandomDrawRecord> | 随机记录 | 复盘用 |
| trace | AgentDecisionTrace | 轨迹 | 调试和训练用 |
| created_command_id | optional<CommandId> | 生成命令 | 适配后命令 ID |
| error | optional<ErrorDetail> | 错误 | 决策失败原因 |

复盘要求：

```text
回放时不重新计算 AgentDecision。
回放使用历史 AgentDecisionRecord 和 CommandReplayLog。
如果需要验证新策略，必须在独立模拟环境运行。
```

## 14. AgentDecisionTrace

`AgentDecisionTrace` 用于解释 Agent 为什么这样做。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| trace_id | TraceId | 轨迹 ID | 追踪 |
| visible_fact_refs | list<string> | 可见事实引用 | 决策依据 |
| knowledge_refs | list<KnowledgeId> | 知识引用 | 使用了哪些知识 |
| memory_refs | list<MemoryId> | 记忆引用 | 使用了哪些记忆 |
| goal_refs | list<AgentGoalId> | 目标引用 | 服务哪些目标 |
| score_breakdown | map<string,float> | 分数拆解 | 风险、收益、知识等 |
| rejected_candidates | list<string> | 被拒候选 | 未选原因 |
| policy_debug | map<string,string> | 策略调试 | 非权威调试信息 |

注意：

```text
Trace 可以用于调试和训练分析。
普通前端只能看到安全摘要，不能看到 Hidden / DebugOnly 依据。
```

## 15. AgentGoal

`AgentGoal` 描述 Agent 当前追求的目标。

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| goal_id | AgentGoalId | 目标 ID | 唯一标识 |
| owner_agent_id | AgentId | 所属 Agent | 目标拥有者 |
| goal_type | string | 目标类型 | survive / explore / teach / fight / flee 等 |
| status | AgentGoalStatus | 状态 | 当前状态 |
| priority | int | 优先级 | 目标排序 |
| target_ref | optional<string> | 目标引用 | 对象、区域、族群等 |
| success_condition | ExpressionRef | 成功条件 | 受限表达式 |
| failure_condition | optional<ExpressionRef> | 失败条件 | 可选 |
| expires_at_tick | optional<Tick> | 过期时间 | 可选 |
| source | string | 来源 | player / tribe / script / cognition / survival |

目标来源示例：

```text
饥饿状态生成 survive_food 目标。
未知对象生成 explore_object 目标。
族群危险生成 defend_tribe 目标。
知识候选生成 teach_knowledge 目标。
敌对族群靠近生成 prepare_battle 目标。
```

## 16. AgentPolicy 接口

`AgentPolicy` 是可替换决策算法接口。

```cpp
class AgentPolicy {
public:
    virtual AgentPolicyType type() const = 0;

    virtual AgentDecision decide(
        const AgentObservation& observation,
        const AgentActionSpace& action_space,
        const AgentState& agent_state,
        const AgentPolicyContext& context
    ) = 0;
};
```

约束：

```text
decide 不能修改 GameState。
decide 不能调用 RulePipeline。
decide 只能使用传入的 Observation、ActionSpace、AgentState、PolicyContext。
需要随机时必须使用 context.random_stream。
```

## 17. AgentCommandAdapter 接口

`AgentCommandAdapter` 负责把 Agent 意图转换成规则引擎命令。

```cpp
class AgentCommandAdapter {
public:
    virtual Result<CommandEnvelope> toCommand(
        const AgentIntent& intent,
        const AgentState& agent_state,
        const AgentCommandAdapterContext& context
    ) const = 0;
};
```

转换规则：

```text
AgentIntent.actor_id -> ActionCommand.actor_id
AgentIntent.action_type -> ActionCommand.action_type
AgentIntent.target -> ActionCommand.target
AgentIntent.parameters -> ActionCommand.parameters
AgentIntent.expected_state_version -> CommandEnvelope.expected_state_version
AgentIntent.idempotency_key -> CommandEnvelope.idempotency_key
AgentId -> CommandEnvelope.source.agent_id
```

适配失败示例：

| 失败 | 错误码建议 |
|---|---|
| Actor 不属于该 Agent | agent_actor_not_bound |
| 行为类型被 AgentDefinition 禁止 | agent_action_forbidden |
| 意图状态版本过旧 | agent_observation_stale |
| 缺少幂等键 | command_missing_idempotency_key |
| 目标格式非法 | command_invalid_target |

## 18. AgentRuntime

`AgentRuntime` 负责调度所有 Agent。

职责：

```text
加载 AgentDefinition。
创建 AgentState。
维护 AgentBinding。
为 Agent 生成 Observation。
生成 ActionSpace。
调用 AgentPolicy。
调用 AgentCommandAdapter。
提交 CommandEnvelope 到 RuleEngine。
记录 AgentDecisionRecord。
处理冷却、暂停、复盘锁定和训练批量运行。
```

推荐流程：

```text
1. collectAgents(tick)
2. buildObservation(agent)
3. buildActionSpace(agent, observation)
4. policy.decide(observation, action_space, state)
5. adapter.toCommand(intent)
6. RuleEngine.submitCommand(command)
7. recordDecision(decision, command_result)
8. updateAgentStateAfterResult(agent, result)
```

伪接口：

```cpp
class AgentRuntime {
public:
    Result<AgentTickResult> tickAgents(const AgentTickRequest& request);
    Result<AgentObservation> buildObservation(AgentId agent_id, StateVersion version) const;
    Result<AgentActionSpace> buildActionSpace(AgentId agent_id, const AgentObservation& observation) const;
    Result<AgentDecision> decide(AgentId agent_id, const AgentObservation& observation);
    Result<EngineCommandResult> submitDecision(const AgentDecision& decision);
    Result<void> setControlMode(AgentId agent_id, AgentControlMode mode);
};
```

## 19. AgentService

`AgentService` 是外部管理 Agent 的稳定接口。

```cpp
class AgentService {
public:
    Result<AgentId> createAgent(const CreateAgentRequest& request);
    Result<void> removeAgent(AgentId agent_id);
    Result<void> bindAgent(const AgentBinding& binding);
    Result<void> unbindAgent(AgentId agent_id, const string& target_id);
    Result<void> setPolicy(AgentId agent_id, AgentPolicyType policy_type, ConfigRef config_ref);
    Result<void> setControlMode(AgentId agent_id, AgentControlMode mode);
    Result<AgentState> getAgentState(AgentId agent_id) const;
    Result<AgentObservation> getObservation(AgentId agent_id) const;
    Result<AgentDecisionRecord> getDecisionRecord(AgentDecisionId decision_id) const;
};
```

权限要求：

```text
普通玩家只能查看和控制自己的 Player Agent。
族群 AI 只能由系统或授权模拟环境创建。
Debug Agent 只能由 debug_admin 创建。
Training Agent 只能由训练运行时创建。
```

## 20. 与认知、记忆、知识的关系

Agent 决策必须基于主观信息。

```text
CognitionState 决定 Agent 当前相信什么。
MemoryRecord 决定 Agent 经历过什么。
KnowledgeClaim 决定 Agent 可传授、可推理、可复用什么。
AgentProfile 决定 Agent 如何偏好这些信息。
AgentPolicy 决定 Agent 如何把信息变成行动。
RuleEngine 决定行动是否成功以及产生什么后果。
```

示例：

```text
对象 A 对真实世界是有毒果实。
Agent 不知道毒性，只知道“可食用置信度 0.3”。
高好奇、低谨慎 Agent 可能尝试吃一口。
高谨慎 Agent 可能先观察、让低价值样本测试，或传播风险警告。
吃一口后的 Feedback 进入 Memory。
Memory 可能形成 KnowledgeClaim。
后续 Agent 决策会读取新的 CognitionProjection。
```

## 21. 与传播、族群、文明的关系

Agent 不只服务个体行动，也服务群体行为。

```text
Tribe Agent 可以选择是否组织共同作战。
Companion Agent 可以选择是否模仿先驱者。
Teaching-oriented Agent 可以主动传播知识。
EnemyTribe Agent 可以评估是否攻击、撤退、结盟。
Civilization 能力只提供可用条件和工具，不直接替 Agent 做选择。
```

注意：

```text
族群分裂不是 Agent 直接决定的状态修改。
Agent 可以产生导致分裂风险变化的行为意图。
最终族群状态变化仍由 TribeState / RulePipeline 结算。
```

## 22. 与强化学习的关系

Agent 系统为强化学习预留标准接口。

强化学习四要素：

| 概念 | 中文 | 对应类型 |
|---|---|---|
| Observation | 观察 | AgentObservation |
| Action | 动作 | AgentActionCandidate / AgentIntent |
| Reward | 奖励 | AgentRewardSignal |
| Done | 终止 | AgentEpisodeEnd |

`AgentRewardSignal` 字段：

| 字段 | 类型 | 中文 | 说明 |
|---|---|---|---|
| reward_id | RewardSignalId | 奖励 ID | 唯一标识 |
| agent_id | AgentId | Agent ID | 接收者 |
| tick | Tick | 时间 | 奖励发生时间 |
| value | float | 奖励值 | 可正可负 |
| components | map<string,float> | 奖励组成 | 生存、知识、族群、安全等 |
| source_event_ids | list<EventId> | 来源事件 | 可追溯 |
| terminal | bool | 是否终止 | episode 是否结束 |
| debug_reason | string | 调试原因 | 非业务依赖 |

奖励原则：

```text
奖励从事件、状态变化和目标达成中派生。
奖励不能反向修改规则结果。
训练观察必须可遮罩，避免模型看到真实隐藏状态。
训练必须使用确定性随机源和可复盘命令序列。
```

## 23. 存档与复盘

Agent 存档必须包含：

```text
AgentDefinition 引用。
AgentState。
AgentBinding。
AgentProfile 当前值。
AgentGoal 列表。
AgentDecisionRecord。
Agent 随机流状态。
策略配置版本。
```

复盘必须遵守：

```text
历史命令优先于重新决策。
历史 AgentDecisionRecord 用于解释当时为什么行动。
如果新版本 AgentPolicy 与旧版本不同，不影响旧回放。
如果要比较新旧策略，必须开新模拟。
```

## 24. 事件设计

Agent 系统可以产生事件，但最终事件仍必须经过事件系统封装。

| 事件 | 中文 | 触发 |
|---|---|---|
| AgentCreated | Agent 创建 | 创建 Agent |
| AgentBound | Agent 绑定 | 绑定 Actor / Tribe |
| AgentControlModeChanged | 控制模式变化 | 手动、自主、训练切换 |
| AgentObservationBuilt | 观察生成 | 调试或训练可见 |
| AgentDecisionMade | 决策完成 | 生成 AgentDecision |
| AgentIntentSubmitted | 意图提交 | 转成 CommandEnvelope |
| AgentDecisionBlocked | 决策阻断 | 无合法行动或权限不足 |
| AgentPolicyChanged | 策略变化 | 切换策略 |
| AgentRewardGenerated | 奖励生成 | 训练奖励产生 |
| AgentEpisodeEnded | 回合结束 | 训练 episode 结束 |

可见性建议：

```text
AgentCreated / AgentBound：Internal 或 DebugOnly。
AgentDecisionMade：DebugOnly，普通前端只看摘要。
AgentIntentSubmitted：Internal，可通过命令结果间接表现。
AgentRewardGenerated：TrainingOnly 或 DebugOnly。
```

## 25. 错误码补充建议

| 错误码 | 中文 | 场景 |
|---|---|---|
| agent_not_found | Agent 不存在 | 查询或调度不存在 Agent |
| agent_disabled | Agent 已禁用 | 禁用状态仍尝试行动 |
| agent_policy_missing | 缺少策略 | 没有可用 Policy |
| agent_policy_failed | 策略失败 | Policy 返回错误 |
| agent_actor_not_bound | 行动者未绑定 | 控制非授权 Actor |
| agent_tribe_not_bound | 族群未绑定 | 控制非授权 Tribe |
| agent_action_forbidden | 行为被禁止 | Definition 禁止行为 |
| agent_observation_stale | 观察过期 | 基于旧 StateVersion 决策 |
| agent_visibility_denied | 可见性拒绝 | 请求越权观察 |
| agent_control_conflict | 控制冲突 | 多 Agent 同时 primary |
| agent_replay_locked | 回放锁定 | 回放中重新决策 |
| agent_training_denied | 训练权限拒绝 | 非训练环境启用 Training Agent |
| agent_decision_timeout | 决策超时 | 远程或复杂策略超时 |
| agent_action_space_empty | 动作空间为空 | 无可尝试动作 |
| agent_reward_invalid | 奖励非法 | 奖励配置或事件来源非法 |

## 26. 配置示例

```yaml
agent_definitions:
  - agent_definition_id: agent_def_pioneer_player
    agent_type: player
    agent_scale: individual
    cognition_band: tool_aware
    embodiment: single_body
    default_control_mode: manual
    default_policy_type: human_input
    visibility_mode: cognition_only
    allowed_command_types:
      - eat
      - use
      - observe
      - teach
      - move
    decision_interval: 0
    max_actions_per_tick: 1
    memory_capacity_hint: 1000
    planning_depth_hint: 3
    profile_template:
      curiosity: 0.8
      caution: 0.4
      cooperation: 0.7
      teaching_bias: 0.6

  - agent_definition_id: agent_def_tribe_basic
    agent_type: tribe
    agent_scale: tribe
    cognition_band: social_learning
    embodiment: distributed
    default_control_mode: autonomous
    default_policy_type: utility
    visibility_mode: tribe_shared
    allowed_command_types:
      - teach
      - gather
      - assist
      - defend
      - migrate
    decision_interval: 5
    max_actions_per_tick: 3
    memory_capacity_hint: 5000
    planning_depth_hint: 5
    profile_template:
      curiosity: 0.5
      caution: 0.6
      cooperation: 0.8
      tribe_loyalty: 0.9
      split_tendency: 0.2

  - agent_definition_id: agent_def_spider_micro
    agent_type: creature
    agent_scale: micro
    cognition_band: reflex
    embodiment: single_body
    default_control_mode: autonomous
    default_policy_type: rule_based
    visibility_mode: perception_limited
    allowed_command_types:
      - move
      - flee
      - attack
      - hide
    decision_interval: 3
    max_actions_per_tick: 1
    memory_capacity_hint: 16
    planning_depth_hint: 1
    profile_template:
      caution: 0.7
      aggression: 0.3
      survival_priority: 1.0
      territoriality: 0.2

  - agent_definition_id: agent_def_wolf_small
    agent_type: creature
    agent_scale: small_creature
    cognition_band: instinct
    embodiment: single_body
    default_control_mode: autonomous
    default_policy_type: utility
    visibility_mode: perception_limited
    allowed_command_types:
      - move
      - hunt
      - attack
      - flee
      - call_pack
    decision_interval: 2
    max_actions_per_tick: 1
    memory_capacity_hint: 128
    planning_depth_hint: 2
    profile_template:
      caution: 0.4
      aggression: 0.7
      cooperation: 0.5
      survival_priority: 0.9
      territoriality: 0.8

  - agent_definition_id: agent_def_alien_civilization_top
    agent_type: enemy_tribe
    agent_scale: interstellar
    cognition_band: transcendent
    embodiment: distributed
    default_control_mode: autonomous
    default_policy_type: goal_oriented
    visibility_mode: cognition_only
    allowed_command_types:
      - observe
      - influence
      - negotiate
      - invade
      - withdraw
      - deploy_probe
    decision_interval: 100
    max_actions_per_tick: 5
    memory_capacity_hint: 1000000
    planning_depth_hint: 20
    profile_template:
      curiosity: 0.9
      caution: 0.8
      aggression: 0.2
      abstraction_bias: 1.0
      risk_tolerance: 0.6
```

## 27. 典型流程

### 27.1 玩家尝试未知果实

```text
1. Player Agent 接收 H5 输入：吃一口对象 A。
2. AgentService 校验 Player Agent 绑定当前 actor。
3. AgentIntent 生成 Eat 行为。
4. AgentCommandAdapter 转成 CommandEnvelope。
5. RuleEngine 执行 Eat。
6. Feedback 系统产生身体反馈。
7. Memory 系统记录经历。
8. Cognition 系统更新“食用对象 A 的效果”。
9. AgentDecisionRecord 保存玩家为什么执行该行为。
```

### 27.2 族群 Agent 主动教学

```text
1. Tribe Agent 观察到族群里多人不知道“某果实有毒”。
2. KnowledgeProjection 显示该知识可传播。
3. ActionSpace 生成 Teach 候选。
4. Utility Policy 评估教学收益高于采集。
5. AgentIntent 生成 Teach 行为。
6. RuleEngine 执行传播规则。
7. PropagationResult 更新接收者认知。
```

### 27.3 敌对族群共同作战

```text
1. EnemyTribe Agent 观察到本族人数、士气、资源和对方威胁。
2. AgentGoal 激活 raid 或 defend。
3. ActionSpace 生成 attack / retreat / threaten / negotiate。
4. Policy 根据 aggression、risk_tolerance、tribe_loyalty 评分。
5. 提交共同作战相关命令。
6. RulePipeline 结算战斗、伤亡、士气、分裂风险。
```

### 27.4 蜘蛛微型 Agent 避火

```text
1. Spider Agent 通过 PerceptionLimited 观察到附近存在高温反馈或火焰对象。
2. 因 CognitionBand=Reflex，不做复杂推理，只触发 flee / hide 候选。
3. RuleBased Policy 选择 flee。
4. AgentIntent 转成 Move 命令。
5. RuleEngine 检查移动合法性并结算位置变化。
6. 如果逃离成功，RewardSignal 可记录 survival 正奖励。
```

### 27.5 狼小型 Agent 召集狼群

```text
1. Wolf Agent 观察到猎物，同时发现单体攻击风险较高。
2. 因 CognitionBand=Instinct，只能基于本能、饥饿、风险和狼群距离评分。
3. ActionSpace 生成 call_pack / attack / follow / flee。
4. Utility Policy 选择 call_pack。
5. Pack Agent 收到影响，生成协同狩猎目标。
6. 每只狼仍通过自己的 AgentIntent 行动，不能由 Pack Agent 直接改位置或伤害。
```

### 27.6 顶级外星文明 Agent 远程观察

```text
1. Interstellar Agent 通过远程探针获得被权限过滤后的 CivilizationProjection。
2. 因 CognitionBand=Transcendent，可以形成超长周期目标：观察、影响、接触或撤离。
3. 如果选择 deploy_probe，仍必须提交 CommandEnvelope。
4. RuleEngine 检查科技、距离、资源、隐藏条件和事件可见性。
5. 外星文明可以影响世界，但不能绕过规则直接改写人类认知或文明状态。
```

### 27.7 强化学习训练

```text
1. TrainingRuntime 创建 Training Agent。
2. AgentRuntime 构造 TrainingMasked Observation。
3. RL Policy 输出动作索引。
4. ActionMask 把动作索引映射到 AgentActionCandidate。
5. AgentIntent 转成 CommandEnvelope。
6. RuleEngine 结算。
7. AgentRewardSignal 从事件和状态变化中生成。
8. 保存 Observation、Action、Reward、Done、Trace。
```

## 28. 测试点

必须测试：

```text
同一 Actor 同一 tick 不能被两个 primary Agent 控制。
普通 Player Agent 不能读取 Hidden / DebugOnly 信息。
AgentIntent 不能绕过 CommandEnvelope。
ReplayLocked Agent 不能重新决策。
Training Agent 观察必须被遮罩。
同 seed、同观察、同策略配置必须产生同 AgentDecision。
AgentActionSpace 为空时必须返回 agent_action_space_empty。
AgentPolicy 超时必须返回 agent_decision_timeout。
Agent 决策失败不能修改 GameState。
AgentDecisionRecord 必须能关联 CommandId 和 EventId。
Spider / Wolf / Human / Alien Civilization 必须都能走 Observation -> Intent -> CommandEnvelope。
低 CognitionBand Agent 不能使用高层 KnowledgeClaim 做抽象推理。
Distributed Agent 必须通过 AgentHierarchyLink 和 AgentBinding 控制下级。
顶级 Agent 不能绕过 RuleEngine 直接修改低级 Agent 状态。
```

组合测试：

```text
未知果实：观察 -> 意图 -> 命令 -> 反馈 -> 记忆 -> 认知更新。
教学行为：知识投影 -> Agent 选择 Teach -> 传播结果 -> 族群认知变化。
共同作战：Tribe Agent 选择 assist / defend -> 规则结算 -> 士气和分裂风险变化。
回放锁定：历史 AgentDecisionRecord 存在 -> 不重新调用 Policy -> 状态哈希一致。
训练步进：Observation + ActionMask -> Command -> Reward -> Done 可批量记录。
尺度扩展：蜘蛛避火、狼群狩猎、人类教学、外星文明探针都共用同一套 AgentRuntime。
层级控制：Civilization Agent 影响 Tribe Agent，Tribe Agent 影响 Individual Agent，但最终行动仍由各自 CommandEnvelope 结算。
```

## 29. 设计注意事项

研发实现必须遵守：

```text
Agent 是决策层，不是规则层。
Agent 只能提交命令，不能直接改状态。
Agent 的观察必须经过权限、感知、认知过滤。
玩家、AI、脚本、训练必须共用 CommandEnvelope。
AI 失败、超时、无动作必须显式返回错误码。
Agent 的随机必须可记录，不能使用系统随机。
回放必须用历史决策记录，不能让 Agent 重新思考。
训练环境不能暴露真实隐藏状态。
AgentDecisionTrace 不能泄露给普通前端。
不要为狼、蜘蛛、人类、外星文明分别实现四套决策管线。
不要把 AgentScale 当作等级数值或能力解锁条件。
不要让顶级 Agent 拥有直接改写低级 Agent 状态的特权。
```

## 30. 与前面系统的关系

| 文档 | 关系 |
|---|---|
| 01_基础类型与通用约定.md | 需要补充 AgentId、AgentDefinitionId、AgentRuntimeId 等 ID 类型 |
| 02_通用枚举设计.md | 后续可把 AgentType、AgentControlMode 等纳入通用枚举 |
| 03_错误码与结果类型设计.md | 需要补充 agent_* 错误码 |
| 04_事件系统设计.md | AgentCreated、AgentDecisionMade 等事件进入事件系统 |
| 05_配置系统设计.md | AgentDefinition、PolicyConfig、RewardConfig 走配置校验 |
| 08_行为命令设计.md | AgentIntent 最终转成 CommandEnvelope / ActionCommand |
| 11_认知状态设计.md | AgentObservation 必须使用 CognitionProjection |
| 12_记忆系统设计.md | AgentDecisionTrace 可以引用 MemoryRecord |
| 13_知识系统设计.md | Agent 决策可引用 KnowledgeClaim 和 KnowledgeProjection |
| 14_传播系统设计.md | Agent 可选择 Teach / Imitate / Share 等传播行为 |
| 15_族群状态设计.md | Tribe Agent 控制族群目标、共同作战、分裂风险相关行为 |
| 16_文明能力设计.md | Agent 可使用已可用文明能力，但不能直接解锁 |
| 17_规则管线设计.md | Agent 输出命令后仍由 RulePipeline 权威结算 |
| 18_规则引擎接口设计.md | AgentRuntime 通过 RuleEngine.submitCommand 调用规则引擎 |
| 19_存档与复盘设计.md | AgentState、DecisionRecord、随机流需要保存和回放 |
| 20_前后端协议设计.md | H5 输入映射到 Player Agent，协议只传意图和结果 |
| 22_测试策略设计.md | 后续测试 Agent 决策确定性、权限、复盘和训练接口 |

## 31. 下一阶段

下一阶段进入：

```text
22_测试策略设计.md
```

测试策略系统需要把 Agent 纳入测试范围：

```text
Agent 尺度扩展测试。
Agent 层级控制测试。
Agent 决策确定性测试。
Agent 权限和观察过滤测试。
Agent 到 CommandEnvelope 的适配测试。
Agent 复盘锁定测试。
强化学习 Observation / Action / Reward / Done 测试。
玩家与 AI 共用规则入口测试。
```
