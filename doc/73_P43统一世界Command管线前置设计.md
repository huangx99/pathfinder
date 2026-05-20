# P43 统一世界 Command 管线前置设计

## 1. 阶段定位

P43 是 V2.0 的第一个正式前置阶段。

它不是世界网格阶段，不是背包阶段，不是前端阶段，也不是制作系统阶段。

P43 的核心目标是先建立 V2.0 所有世界行为的统一入口：

```text
WorldCommandPipeline
```

后续所有世界变化都必须通过 Command 管线进入系统，包括：

```text
玩家移动
玩家观察
玩家等待
NPC 自主行动
野兽行动
世界生成
资源刷新
区域事件
区域魔法
天气推进
反应链触发
测试脚本
复盘重放
```

一句话定义：

```text
P43 先定义“世界里任何事情如何发生”，再让 P44 以后把地图、背包、采集、制作、NPC、野兽接进来。
```

如果不先做 P43，后续 P44-P54 很容易写成：

```text
grid.movePlayer()
inventory.addItem()
resource.gather()
agent.doSomething()
areaEffect.tick()
```

这种分散函数调用会破坏事件、复盘、知识、Agent、前端投影的一致性。

P43 完成后，外部系统只能提交 Command，不能直接改世界状态。

## 2. 为什么现在做

V2.0 的核心玩法已经确定：

```text
二维世界网格。
玩家探索世界。
玩家采集、砍树、挖掘、制作。
玩家形成知识。
玩家把知识教给无知 NPC。
NPC 根据知识在世界里自主行动。
野兽在世界里移动、攻击、逃跑、绕行。
区域事件、天气、区域魔法会改变世界。
前端只显示后端投影，不推规则。
```

这些玩法看起来不同，但工程上都应该是同一件事：

```text
一个来源发出 Command。
Command 经过统一管线。
管线校验条件、知识、距离、材料、区域、层级。
管线产出状态变化、事件、经验、投影变化。
前端或复盘系统读取结果。
```

所以 P43 必须先于世界网格。

原因：

1. 世界网格的移动必须走 Command，否则后续 NPC 移动、复盘、前端按钮会重写。
2. 世界生成必须走 Command，否则 seed 生成不可复盘。
3. 采集制作必须走 Command，否则知识发现、材料消耗、事件记录会分叉。
4. 区域魔法和区域事件必须走 Command，否则它们会绕过规则管线直接改状态。
5. 前端必须只发 Command，否则前端会再次理解引擎。

## 3. 玩法场景脑暴

本章节不是实现清单，而是为了验证 Command 设计能覆盖未来玩法。

### 3.1 玩家探索类

玩家在二维地图上移动：

```text
玩家点击东边格子。
前端发送 move command。
后端校验目标是否相邻、是否同 layer、是否可进入。
后端移动玩家，揭示新视野。
返回 changed_cells、changed_entities、event_feed、available_commands。
```

玩家观察未知物：

```text
玩家点击未知灌木。
前端发送 inspect command。
后端返回安全描述，不暴露隐藏效果。
可能生成观察记忆，但不一定形成知识。
```

玩家等待：

```text
玩家发送 wait command。
世界 tick 推进。
区域效果 tick。
NPC 行动。
野兽移动。
资源可能生长。
前端收到事件和地图变化。
```

### 3.2 采集与资源类

采集红果：

```text
玩家对红果灌木发送 gather command。
后端校验距离和目标存在。
红果进入玩家背包。
灌木剩余资源减少。
产生经验：红果灌木 + gather -> 红果。
前端显示 +红果、背包变化、灌木状态变化。
```

砍树：

```text
玩家对树发送 chop command。
后端校验是否有可用工具、树是否可砍。
斧头耐久下降。
树资源下降或变成树桩。
木头进入背包或掉在地上。
产生经验：树 + chop -> 木头。
```

挖石：

```text
玩家对石块发送 mine command。
工具不够时 command blocked。
工具足够时产出石料。
可能暴露地下入口。
```

### 3.3 制作与反应类

制作火把：

```text
玩家发送 craft command。
目标 recipe 是 torch。
后端解析材料来源：玩家背包 / 营地库存 / 附近火源。
材料足够则消耗材料并产出火把。
材料不足则返回 failure_reason 和 missing_materials。
产生制作知识候选。
```

点火：

```text
玩家对干草使用火种。
Command 触发反应链。
生成 camp_fire 或 fire_area_effect。
后续 AreaEffectTick 可能点燃附近可燃物。
```

### 3.4 教学与知识传播类

玩家教 NPC：

```text
玩家选择 knowledge_key，目标 NPC。
发送 teach command。
后端校验玩家是否拥有可教学知识、NPC 是否在交流范围内。
传播系统生成 NPC 知识。
返回 NPC 知识 patch 和事件。
```

NPC 反向教学：

```text
未来专家 NPC 拥有知识。
玩家请求学习。
专家 NPC 通过 teach command 传授给玩家。
同一管线可复用。
```

族群知识传播：

```text
族群会议、营地教学、口口相传都可以是 command。
source 可以是 SystemTick 或 AgentDecision。
```

### 3.5 NPC 自主行动类

NPC 饿了：

```text
AgentReasoner 发现 hunger 需要降低。
根据知识推理出红果可食用。
根据来源知识推理出红果来自红果灌木。
根据位置记忆找到附近灌木。
Agent 生成 move / gather / eat command 序列。
每一步都走 WorldCommandPipeline。
```

NPC 不知道红果：

```text
Agent 没有红果食用知识。
不能凭空生成 gather/eat command。
只能等待、跟随、求助或本能移动。
```

NPC 遇到狼：

```text
狼进入同一或相邻格子。
外界打断系统生成 interrupt command 或 danger event。
NPC 若知道火把能驱狼且有火把，则生成 use command。
不知道则逃跑或求助。
```

### 3.6 野兽生态类

狼巡逻：

```text
SystemTick 触发 BeastDecision。
狼生成 move command。
后端校验地形、领地、目标。
狼移动后前端收到 entity patch。
```

狼攻击：

```text
狼感知范围内发现玩家或 NPC。
生成 attack command。
管线校验距离、layer、阻挡、恐惧状态。
成功则产生伤害 delta 和事件。
```

狼怕火：

```text
狼进入火焰区域或看到火把。
如果狼有本能 fear_fire，生成 flee command。
不是直接删除狼。
```

### 3.7 世界生成类

新游戏开始：

```text
bootstrap 触发 generate_world command。
source = SystemTick 或 ScriptedScenario。
输入 seed、尺寸、内容包。
输出格子、初始资源、玩家出生点、基础野兽、无知 NPC。
生成过程进入 replay trace。
```

局部区域生成：

```text
玩家接近未生成边缘。
系统生成 generate_region command。
可以生成新资源、新野兽、新遗迹。
```

### 3.8 区域事件类

火灾：

```text
火焰 area effect 每隔 N tick 生成 burn command。
burn command 影响区域内可燃物、NPC、野兽。
产生火焰蔓延、受伤、资源破坏事件。
```

下雨：

```text
weather command 生成 rain area effect。
rain tick command 可能熄灭火、提升植物湿度、降低可见性。
```

地震：

```text
trigger_area_event command 影响矩形区域。
可能破坏建筑、改变地形、打开裂缝。
```

兽潮：

```text
区域事件生成一组 spawn_beast command。
野兽从地图边缘进入。
玩家和 NPC 可基于知识应对。
```

### 3.9 区域魔法类

治疗法阵：

```text
cast_area_effect command 创建 healing_area_effect。
每 tick 生成 heal command。
影响区域内友方单位。
```

毒雾：

```text
poison_cloud area effect 每 tick 生成 poison command。
不知道毒雾危险的 NPC 可能误入。
知道后会绕路。
```

火焰结界：

```text
fire_barrier area effect 对进入区域的野兽生成 fear 或 burn command。
```

传送门：

```text
portal area effect 对进入者生成 teleport command。
跨 layer 或跨坐标移动。
```

### 3.10 高级文明与科技类

自动炮塔：

```text
turret area effect 扫描范围。
发现敌对单位后生成 attack command。
```

辐射区：

```text
radiation area effect 每 tick 生成 damage 或 mutation experience。
```

护盾范围：

```text
shield area effect 拦截 projectile / beast attack command。
```

纳米污染：

```text
nano_swarm area effect 逐步修改对象状态。
本质仍是 tick command + state delta。
```

## 4. 本阶段目标

P43 完成后系统必须具备：

1. 能表示统一的 `WorldCommandDto`。
2. 能表示 Command 来源、目标、上下文、参数。
3. 能执行最小 Command 管线。
4. 能返回标准 `WorldCommandResponseDto`。
5. Response 中包含 result、events、projection_patch、frontend_hints、available_commands。
6. 能记录 command trace，支撑复盘。
7. 能区分玩家输入、Agent 决策、系统 tick、区域效果、测试、复盘来源。
8. 能承载未来世界生成、移动、采集、制作、教学、区域事件、区域魔法。
9. 前端只需要发送 Command 和消费 Response，不需要知道引擎函数。

## 5. 本阶段不解决的问题

P43 不实现完整玩法。

不做：

```text
不做真实世界网格。
不做背包。
不做资源采集。
不做制作。
不做 NPC 自主推理。
不做野兽 AI。
不做区域魔法实际结算。
不做 H5 V2 正式界面。
不做复杂存档。
```

P43 只做 Command 基座。

第一版 Command 可以只执行：

```text
noop
wait
inspect_stub
system_tick_stub
generate_world_stub
```

这些 stub 不代表假架构，而是为了验证统一输入输出和管线结构。

## 6. 核心设计原则

### 6.1 所有世界变化都由 Command 产生

任何世界状态变化都必须来自 Command。

禁止外部直接调用：

```text
movePlayer()
addItem()
spawnBeast()
learnKnowledge()
applyAreaMagic()
```

服务可以存在，但只能由 CommandPipeline 内部调用。

### 6.2 Command 是意图，不是函数名

前端发送：

```text
command_key = gather
```

不是：

```text
call ResourceService.gatherRedBerry()
```

后端根据 command_key、目标、上下文、内容配置、状态来决定如何执行。

### 6.3 玩家和 NPC 使用同一管线

玩家采集红果和 NPC 采集红果都走：

```text
gather command
```

差异只在：

```text
source
actor_key
knowledge_gate
可见投影
```

### 6.4 前端只消费 CommandOption

前端按钮来自后端返回的：

```text
available_commands
```

前端不能自己判断按钮是否可用。

### 6.5 Command 输出必须服务前端

Command 结果不能只给后端用。

必须返回前端可消费的变化：

```text
projection_patch
event_feed
frontend_hints
available_commands
```

### 6.6 区域事件和区域魔法也是 Command 来源

火焰、毒雾、雨、地震、治疗法阵、辐射区，都不能直接改状态。

它们只能在 tick 时生成 Command。

### 6.7 可复盘优先

Command 必须记录：

```text
输入
规范化结果
校验结果
状态变化
事件
经验
后续命令
```

否则以后很难调 NPC 和世界生成。

## 7. 模块划分

| 模块 | 中文含义 | 职责 | 输入 | 输出 | 禁止事项 |
|---|---|---|---|---|---|
| WorldCommandDto | 世界命令 DTO | 表示外部或系统提交的命令 | 前端/Agent/系统 | 标准命令数据 | 不执行逻辑 |
| WorldCommandPipeline | 命令管线 | 统一执行命令 | WorldCommandDto | WorldCommandResponseDto | 不绕过校验 |
| CommandNormalizer | 命令规范化 | 补齐默认字段、转换别名 | request dto | normalized command | 不查隐藏规则 |
| CommandValidator | 命令校验 | 校验来源、actor、target、layer、条件 | normalized command | validation result | 不修改状态 |
| CommandExecutor | 命令执行器 | 生成状态变化计划 | validated command | state delta plan | 不直接提交状态 |
| StateDeltaApplier | 状态变更提交器 | 原子提交状态变化 | delta plan | committed state | 不产生 UI 文案 |
| WorldEventBuilder | 世界事件构建 | 生成可复盘事件 | command/result/delta | event_feed | 不改状态 |
| ExperienceEmitter | 经验输出器 | 把命令结果转为学习输入 | command/result/event | experience records | 不直接伪造知识 |
| ProjectionPatchBuilder | 投影补丁构建 | 输出前端变化 | old/new projection | projection_patch | 不暴露隐藏真相 |
| FrontendHintBuilder | 前端表现提示 | 生成表现建议 | command/result/event | frontend_hints | 不决定规则 |
| AvailableCommandBuilder | 可用命令构建 | 输出下一步按钮 | current state/projection | command options | 不让前端推导 |
| CommandTraceRecorder | 命令追踪 | 记录复盘信息 | pipeline stages | trace records | 不参与裁决 |

## 8. 核心枚举

### 8.1 WorldCommandKind

```cpp
enum class WorldCommandKind {
    Unknown,
    Noop,
    Wait,
    Inspect,
    Move,
    Gather,
    Chop,
    Mine,
    Dig,
    Pickup,
    Drop,
    Eat,
    Use,
    Craft,
    Teach,
    Attack,
    Flee,
    GenerateWorld,
    GenerateRegion,
    SpawnEntity,
    DespawnEntity,
    TriggerAreaEvent,
    ApplyAreaEffectTick,
    CastAreaEffect,
    SystemTick
};
```

P43 第一版只要求支持：

```text
Noop
Wait
Inspect
GenerateWorld stub
SystemTick stub
```

其他枚举先定义，后续阶段逐步实现。

### 8.2 WorldCommandSource

```cpp
enum class WorldCommandSource {
    Unknown,
    PlayerInput,
    AgentDecision,
    BeastDecision,
    SystemTick,
    WorldGeneration,
    AreaEffectTick,
    ReactionChain,
    ScriptedScenario,
    Replay,
    Test
};
```

### 8.3 WorldCommandResultKind

```cpp
enum class WorldCommandResultKind {
    Unknown,
    Succeeded,
    Failed,
    Blocked,
    Interrupted,
    Deferred,
    Noop
};
```

说明：

```text
Failed    表示命令执行失败，例如目标不存在。
Blocked   表示规则阻止，例如条件不满足、知识不足、材料不足。
Deferred  表示命令进入队列，例如长动作或等待外部结果。
Noop      表示命令合法但没有产生变化。
```

### 8.4 WorldCommandTargetKind

```cpp
enum class WorldCommandTargetKind {
    None,
    Coordinate,
    Entity,
    Actor,
    Item,
    Inventory,
    Area,
    Knowledge,
    Recipe
};
```

### 8.5 PatchOp

```cpp
enum class PatchOp {
    Add,
    Update,
    Remove,
    Replace,
    Clear
};
```

### 8.6 FrontendHintKind

```cpp
enum class FrontendHintKind {
    HighlightCell,
    HighlightEntity,
    FloatingText,
    ShakeCell,
    PlaySound,
    SpawnEffect,
    RemoveEffect,
    OpenPanel,
    UpdateCommandBar,
    FocusCamera
};
```

### 8.7 AreaShapeKind

```cpp
enum class AreaShapeKind {
    SingleCell,
    CircleRadius,
    Rectangle,
    Line,
    Cone,
    CustomMask
};
```

P43 只定义，不实现复杂区域结算。

## 9. 核心 DTO

### 9.1 WorldCoordinateDto

```cpp
struct WorldCoordinateDto {
    int x = 0;
    int y = 0;
    std::string layer_key = "surface";
};
```

说明：V2.0 是二维游戏，`layer_key` 是虚拟 Z 层。

### 9.2 WorldCommandTargetDto

```cpp
struct WorldCommandTargetDto {
    WorldCommandTargetKind target_kind = WorldCommandTargetKind::None;
    std::optional<WorldCoordinateDto> target_coord;
    std::string target_entity_id;
    std::string target_actor_key;
    std::string target_item_key;
    std::string target_inventory_id;
    std::string target_area_id;
    std::string knowledge_key;
    std::string recipe_key;
};
```

### 9.3 WorldCommandContextDto

```cpp
struct WorldCommandContextDto {
    uint64_t issued_tick = 0;
    uint64_t client_sequence = 0;
    std::string correlation_id;
    std::string parent_command_id;
    std::string actor_location_layer;
    bool allow_hidden_debug = false;
};
```

### 9.4 WorldCommandDto

```cpp
struct WorldCommandDto {
    std::string command_id;
    WorldCommandKind command_kind = WorldCommandKind::Unknown;
    std::string command_key;
    WorldCommandSource source = WorldCommandSource::Unknown;
    std::string actor_key;
    WorldCommandTargetDto target;
    WorldCommandContextDto context;
    std::map<std::string, std::string> parameters;
};
```

### 9.5 WorldStateDeltaDto

```cpp
struct WorldStateDeltaDto {
    std::string delta_id;
    std::string delta_kind;
    std::string target_ref;
    PatchOp op = PatchOp::Update;
    std::map<std::string, std::string> fields;
    std::vector<std::string> reason_keys;
};
```

P43 只定义通用结构，不要求所有 delta 类型实现。

### 9.6 WorldEventDto

```cpp
struct WorldEventDto {
    std::string event_id;
    std::string event_kind;
    uint64_t tick = 0;
    std::string title_text;
    std::string body_text;
    std::optional<WorldCoordinateDto> coord;
    std::string actor_key;
    std::string target_entity_id;
    int priority = 0;
    std::vector<std::string> reason_keys;
};
```

### 9.7 WorldExperienceDto

```cpp
struct WorldExperienceDto {
    std::string experience_id;
    std::string actor_key;
    std::string command_key;
    std::string subject_entity_key;
    std::string target_entity_key;
    std::string effect_key;
    uint64_t tick = 0;
    std::vector<std::string> condition_keys;
    std::vector<std::string> reason_keys;
};
```

### 9.8 WorldCommandResultDto

```cpp
struct WorldCommandResultDto {
    std::string command_id;
    std::string command_key;
    WorldCommandResultKind result_kind = WorldCommandResultKind::Unknown;
    std::string actor_key;
    std::string target_ref;
    std::vector<std::string> failure_reason_keys;
    std::vector<std::string> warning_keys;
    std::vector<WorldStateDeltaDto> state_deltas;
    std::vector<WorldCommandDto> spawned_commands;
};
```

### 9.9 Projection Patch DTO

```cpp
struct WorldProjectionPatchDto {
    uint64_t base_projection_version = 0;
    uint64_t new_projection_version = 0;
    bool requires_full_refresh = false;
    std::vector<WorldCellPatchDto> changed_cells;
    std::vector<WorldEntityPatchDto> changed_entities;
    std::vector<InventoryPatchDto> changed_inventories;
    std::vector<KnowledgePatchDto> changed_knowledge;
    std::vector<AreaEffectPatchDto> changed_area_effects;
};
```

P43 可以先让 patch 为空，但结构必须固定。

### 9.10 WorldFrontendHintDto

```cpp
struct WorldFrontendHintDto {
    FrontendHintKind hint_kind;
    std::string target_entity_id;
    std::optional<WorldCoordinateDto> target_coord;
    std::string text;
    std::string effect_key;
    int priority = 0;
    int duration_ms = 0;
};
```

### 9.11 WorldCommandOptionDto

```cpp
struct WorldCommandOptionDto {
    std::string option_id;
    WorldCommandKind command_kind;
    std::string command_key;
    std::string label_text;
    WorldCommandTargetDto target;
    bool enabled = true;
    std::string disabled_reason_text;
    int priority = 0;
};
```

### 9.12 WorldCommandResponseDto

```cpp
struct WorldCommandResponseDto {
    std::string session_id;
    std::string command_id;
    WorldCommandResultDto result;
    WorldProjectionPatchDto projection_patch;
    std::vector<WorldEventDto> event_feed;
    std::vector<WorldExperienceDto> experiences;
    std::vector<WorldFrontendHintDto> frontend_hints;
    std::vector<WorldCommandOptionDto> available_commands;
    std::vector<std::string> warning_keys;
    std::vector<std::string> debug_trace_keys;
};
```

这是前端 Command 调用的标准响应。

## 10. Command 管线流程

P43 管线按以下顺序设计：

```text
1. ReceiveCommand
2. NormalizeCommand
3. ValidateCommandShape
4. ResolveActor
5. ResolveTarget
6. ValidateSource
7. ValidateLayer
8. ValidateRange
9. ValidateKnowledgeGate
10. ValidateConditions
11. ValidateMaterials
12. BuildEffectPlan
13. SimulateStateDelta
14. ApplyStateDelta
15. EmitWorldEvents
16. EmitExperiences
17. SpawnFollowupCommands
18. BuildProjectionPatch
19. BuildAvailableCommands
20. RecordTrace
```

P43 第一版可以把第 7-17 步做成 stub，但接口必须存在。

原因：P44 以后每个阶段只填充对应步骤，不再改整体管线。

## 11. 前后端交互协议

### 11.1 Bootstrap

```text
POST /api/v2/world/bootstrap
```

P43 第一版可以返回空世界投影或 stub 投影。

响应至少包含：

```text
session_id
projection_version
available_commands
```

### 11.2 Command

```text
POST /api/v2/world/command
```

请求：

```json
{
  "session_id": "s1",
  "command": {
    "command_id": "cmd_001",
    "command_key": "wait",
    "command_kind": "Wait",
    "source": "PlayerInput",
    "actor_key": "player",
    "target": {
      "target_kind": "None"
    },
    "context": {
      "client_sequence": 1
    }
  }
}
```

响应：

```json
{
  "session_id": "s1",
  "command_id": "cmd_001",
  "result": {
    "command_key": "wait",
    "result_kind": "Succeeded"
  },
  "projection_patch": {
    "base_projection_version": 1,
    "new_projection_version": 2,
    "requires_full_refresh": false
  },
  "event_feed": [
    {
      "event_kind": "TimePassed",
      "title_text": "时间流逝",
      "body_text": "你等待了一会儿。"
    }
  ],
  "frontend_hints": [],
  "available_commands": []
}
```

### 11.3 Projection

```text
GET /api/v2/world/projection?session_id=s1
```

当 `projection_patch.requires_full_refresh = true` 时，前端请求完整投影。

## 12. ProjectionPatch 设计原则

Command 返回必须能告诉前端“哪里变了”。

前端流程：

```text
发送 command。
收到 response。
应用 projection_patch。
展示 event_feed。
播放 frontend_hints。
刷新 available_commands。
```

Patch 不是规则来源，只是显示更新。

第一版可以先返回空 patch，但必须带版本号：

```text
base_projection_version
new_projection_version
requires_full_refresh
```

后续 P44-P52 逐步填充：

```text
changed_cells
changed_entities
changed_inventories
changed_knowledge
changed_area_effects
```

## 13. 区域事件与区域魔法预留

P43 只定义区域事件和区域魔法如何接入 Command，不实现完整效果。

### 13.1 AreaShapeDto

```cpp
struct AreaShapeDto {
    AreaShapeKind shape_kind = AreaShapeKind::SingleCell;
    WorldCoordinateDto origin;
    int radius = 0;
    int width = 0;
    int height = 0;
    std::vector<WorldCoordinateDto> cells;
};
```

### 13.2 AreaEffectCommand 输入

区域效果 tick 产生 Command：

```text
command_kind = ApplyAreaEffectTick
source = AreaEffectTick
target.target_area_id = area_effect_id
```

### 13.3 区域事件输入

一次性区域事件产生 Command：

```text
command_kind = TriggerAreaEvent
source = SystemTick / ScriptedScenario / Test
target.target_area_id = area_event_id
```

### 13.4 区域魔法输入

玩家或 NPC 释放区域魔法：

```text
command_kind = CastAreaEffect
source = PlayerInput / AgentDecision
target.target_coord = 施法中心
parameters["effect_key"] = "poison_cloud"
parameters["shape"] = "circle_radius"
```

## 14. 与现有系统接入

| 现有系统 | P43 接入方式 |
|---|---|
| P1 Command / Config | 复用命令思想，V2 新增 WorldCommand 专用 DTO |
| P4 Replay | CommandTraceRecorder 为复盘提供输入 |
| P27 条件表达式 | ValidateConditions 后续接 P27 |
| P28/P28.5 反应系统 | BuildEffectPlan 后续接反应和制作 |
| P32 投影 | ProjectionPatchBuilder 遵守安全投影原则 |
| P39 Agent 推理 | AgentDecision 作为 CommandSource |
| P40 动态执行 | GoalExecutionStep 生成 WorldCommand |
| P41 通用效果执行 | CommandExecutor 通过 EffectPlan 接入 |
| P42 JSON 内容包 | command_key、effect_key、area_effect_key 后续来自内容包 |

## 15. 文件与接口建议

P43 建议新增：

```text
backend/include/pathfinder/world_command/world_command_types.h
backend/include/pathfinder/world_command/world_command_pipeline.h
backend/include/pathfinder/world_command/world_command_projection.h
backend/include/pathfinder/world_command/world_command_trace.h
backend/src/world_command/world_command_types.cpp
backend/src/world_command/world_command_pipeline.cpp
backend/src/world_command/world_command_projection.cpp
backend/src/world_command/world_command_trace.cpp
backend/tests/unit/world_command/world_command_test.cpp
backend/tests/integration/world_command/world_command_flow_test.cpp
```

是否单独 target：

```text
pathfinder_world_command
```

依赖方向：

```text
world_command -> foundation / config / event / protocol
world_grid -> world_command
h5_v2 -> world_command projection
```

禁止：

```text
world_command 反向依赖 H5。
world_command 直接依赖旧 h5_dialog。
world_command 使用旧 DialogScenario。
```

## 16. P43 最小实现范围

P43 最小实现只做：

```text
WorldCommandKind / Source / ResultKind / TargetKind / PatchOp / FrontendHintKind / AreaShapeKind
WorldCommandDto
WorldCommandResponseDto
WorldCommandPipeline::execute
NoopCommandHandler
WaitCommandHandler stub
InspectCommandHandler stub
SystemTickCommandHandler stub
ProjectionPatch empty builder
AvailableCommand empty builder
Trace recorder
```

最小行为：

```text
Noop -> Succeeded / Noop event
Wait -> Succeeded / TimePassed event / projection version +1
Inspect -> Succeeded or Blocked / Inspect event
SystemTick -> Succeeded / SystemTick event
Unknown command -> Failed / failure_reason unknown_command
```

## 17. 测试设计

### 17.1 枚举测试

```text
WorldCommandKind 字符串转换。
WorldCommandSource 字符串转换。
WorldCommandResultKind 字符串转换。
PatchOp 字符串转换。
FrontendHintKind 字符串转换。
AreaShapeKind 字符串转换。
```

### 17.2 DTO 校验测试

```text
command_id 不能为空。
command_key 必须稳定英文 key。
source 不能 Unknown。
PlayerInput 必须有 actor_key。
target_coord 必须带 layer_key。
client_sequence 不能倒退。
```

### 17.3 管线测试

```text
Noop command 成功。
Wait command 产生 TimePassed event。
Unknown command 返回 Failed，不崩溃。
Invalid actor 返回 Blocked 或 Failed。
Command response 必须包含 result / patch / events / available_commands。
```

### 17.4 前端协议测试

```text
POST /api/v2/world/command 可提交 wait。
响应包含 command_id。
响应包含 projection_patch version。
响应包含 event_feed。
响应包含 available_commands。
```

P43 可以先不实现 HTTP，但 DTO 和 codec 必须为 HTTP 做准备。

### 17.5 复盘测试

```text
同一个 command 输入能记录 trace。
trace 包含 pipeline stage。
trace 不包含前端临时状态。
```

## 18. 验收标准

P43 验收必须满足：

```text
存在统一 WorldCommand DTO。
存在统一 WorldCommandPipeline。
Noop / Wait / Inspect / SystemTick stub 可执行。
所有执行都返回 WorldCommandResponseDto。
Response 包含 result、projection_patch、event_feed、frontend_hints、available_commands。
非法 command 不崩溃，返回明确失败原因。
Command source 能区分 PlayerInput / AgentDecision / SystemTick / AreaEffectTick / Test / Replay。
投影 patch 有版本号。
Command trace 可记录。
没有任何旧 H5 DialogScenario 依赖。
没有直接世界状态函数暴露给前端。
```

## 19. 风险与防线

### 19.1 风险：Command 变成函数名包装

防线：Command 必须走 normalize / validate / execute / delta / event / projection 全流程。

### 19.2 风险：前端继续自己生成按钮

防线：前端按钮只能来自 `available_commands`。

### 19.3 风险：区域事件绕过管线

防线：AreaEffectTick 和 TriggerAreaEvent 必须是 CommandSource。

### 19.4 风险：P43 过度实现玩法

防线：P43 只做基座，不做完整网格、背包、采集、制作。

### 19.5 风险：和旧 H5 纠缠

防线：P43 不依赖旧 h5_dialog / h5_playable。旧 H5 放着不动，V2 H5 以后重建。

## 20. 后续阶段关系

P43 完成后：

```text
P44 世界网格基础与探索投影：move / inspect 开始接真实 grid。
P45 背包、物品实例与归属：pickup / drop / inventory patch 接入。
P46 世界生成器与资源分布：generate_world / generate_region 接入。
P47 资源采集、砍伐、挖掘：gather / chop / mine 接入。
P48 制作与世界反应接入：craft / use / reaction chain 接入。
P49 探索行为产生知识：experiences 接入认知记忆知识。
P50 教学与无知 NPC：teach 接入传播系统。
P51 Agent 网格目标执行：AgentDecision 生成 command。
P52 野兽网格生态：BeastDecision 生成 command。
P53 H5 V2 地图界面：前端只提交 command、消费 response。
P54 存档复盘：command trace 成为复盘基础。
```

## 21. 结论

P43 是 V2.0 的地基。

它要解决的不是“玩家能不能移动”，而是：

```text
以后世界里任何事情，都必须通过同一条可验证、可复盘、可投影、可扩展的 Command 管线发生。
```

只要 P43 做稳，后续世界网格、背包、采集、制作、NPC、野兽、区域魔法、前端交互都可以一层一层接入，不会再次形成分散函数和假前端逻辑。
