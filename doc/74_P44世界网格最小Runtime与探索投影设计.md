# P44 世界网格最小 Runtime 与探索投影设计

## 1. 阶段定位

P44 是 V2.0 开放世界的第一个运行时阶段，接在 P43 统一世界 Command 管线之后。

P43 已经回答：

```text
世界里的任何事情都必须如何进入系统：WorldCommandPipeline。
```

P44 要回答：

```text
Command 进入系统后，作用在什么世界上，世界如何被前端看见。
```

一句话定位：

```text
P44 建立“二维世界网格 + 实体位置 + 探索视野 + 投影 patch”的最小后端世界，不做完整采集、背包、制作和 NPC 智能。
```

P44 不是新 H5 阶段，不是内容玩法阶段，不是背包阶段，也不是完整地图编辑器阶段。

P44 的核心价值是把后续所有玩法放回真实空间：

```text
玩家在哪里。
NPC 在哪里。
野兽在哪里。
资源在哪里。
哪个格子被看见。
哪个格子可以走。
Command 执行后前端应该刷新哪些格子和实体。
```

如果没有 P44，后续采集、教学、野兽、背包、制作都会继续像旧 H5 一样漂浮在按钮上，无法形成开放世界体验。

## 2. 前置依据

P44 必须严格依据以下文档和实现：

```text
doc/00_设计文档编写要求.md
doc/72_V2_0开放世界网格生存与文明启蒙整体架构设计.md
doc/73_P43统一世界Command管线前置设计.md
doc/review/P43统一世界Command管线前置复审报告.md
backend/include/pathfinder/world_command/world_command_types.h
```

P44 不允许绕开 P43：

```text
移动不能直接调用 player.moveTo。
观察不能直接调用 frontend.showCell。
世界生成不能直接改全局变量后让前端读。
所有对外行为必须通过 WorldCommandPipeline 返回 WorldCommandResponseDto。
```

## 3. 本阶段目标

P44 的目标是做出后端最小世界 Runtime：

```text
可生成一个确定性的二维世界。
玩家有坐标。
实体有坐标。
格子有地形和可通行性。
玩家可以通过 Move Command 移动。
玩家可以通过 Inspect Command 观察格子或实体。
Wait Command 可以推进世界时间步 world_tick。
ProjectionPatch 可以返回 changed_cells / changed_entities。
前端只要消费 patch，就能画出地图变化。
```

玩家体验目标：

```text
玩家不再是在文本框里点孤立按钮。
玩家出生在一个小世界中。
玩家移动后能看到新格子。
玩家能知道“我在哪里、附近有什么、什么格子不能走”。
```

工程目标：

```text
为 P45 背包和物品实例、P46 可用命令生成、P47 采集砍伐挖掘、P48 制作反应、P49 知识接入、P53 H5 V2 地图界面提供稳定坐标和投影基础。
```

## 4. 本阶段明确不做

P44 不做以下内容：

```text
不做完整背包。
不做资源采集结算。
不做砍树、挖矿、制作火把。
不做 NPC 自主移动和野兽 AI。
不做完整寻路。
不做战争、建筑、农业、天气系统。
不做前端重构。
不接复杂 JSON 地形生态生成。
不做地图存档完整格式。
不接知识形成和教学。
```

但 P44 必须为这些能力预留字段和接口，不能写死成只能支持玩家移动的小玩具。

## 5. 未来问题脑暴

本章节先把后续大世界一定会遇到的问题列出来，避免 P44 只按当前最小移动写死。

### 5.1 坐标与层级问题

后续会出现：

```text
地表、地下、水中、空中、室内、树冠。
同一个 x/y 在不同 layer_key 上有不同内容。
玩家从地表进入矿洞。
鸟在 air 层，鱼在 water 层，狼在 surface 层。
建筑内部和外部都可能占用相同 x/y。
```

P44 防线：

```text
所有坐标必须使用 WorldCoordinateDto：x / y / layer_key。
cell_id 必须包含 layer_key，不允许只用 x_y。
第一版只生成 surface，但 Runtime 结构必须支持多 layer。
```

### 5.2 地图无限扩展问题

后续开放世界可能不能一次加载全部地图：

```text
玩家走远后需要生成新区域。
区域需要按 seed 可复现。
远处区域不能一直占内存。
存档只保存改动过的区域。
```

P44 防线：

```text
WorldGridRuntime 以 region 为边界管理格子。
P44 可先只生成一个固定 region，但接口必须有 region_id。
GenerateWorldCommand 生成初始 region，GenerateRegionCommand 未来生成周边 region。
```

### 5.3 视野与探索问题

后续会出现：

```text
玩家只知道看见过的地方。
黑暗、雾、墙、树、山会遮挡视野。
NPC 和野兽也有自己的视野。
玩家教 NPC 后，NPC 不应该凭空知道没见过的地图。
```

P44 防线：

```text
WorldExplorationState 单独记录 actor_key -> discovered / visible。
P44 先做半径视野，不做遮挡。
ProjectionPatch 只返回玩家可见或本次变化允许公开的格子。
```

### 5.4 实体归属和位置问题

后续会出现：

```text
红果灌木是地图实体。
掉在地上的火把是地图实体。
玩家背包里的火把不是地图实体。
营地库存里的木头也不是地图格子实体。
NPC 手里的物品属于 NPC。
```

P44 防线：

```text
P44 只处理地图实体位置，不实现完整库存。
WorldEntityInstance 必须预留 owner_ref / container_ref。
当 entity 在地图上时，location 有值；在背包里时，location 可为空，由 P45 管。
```

### 5.5 阻挡与通行问题

后续会出现：

```text
水不能直接走。
山不能直接走。
门关闭时不能进屋。
桥建好后水面可以通过。
火区危险但不一定阻挡。
大型野兽可能临时占位阻挡。
```

P44 防线：

```text
WorldCellRuntime 有 terrain_key、movement_cost、blocks_movement。
WorldEntityInstance 有 blocks_movement。
P44 Move 校验只做相邻和阻挡，后续条件表达式接入时再扩展。
```

### 5.6 区域事件与区域魔法问题

后续会出现：

```text
一片区域下雨。
毒雾覆盖几个格子。
魔法结界禁止进入。
火焰蔓延到邻近干草。
寒冷区域让角色掉状态。
```

P44 防线：

```text
WorldAreaRuntime 预留 area_id、shape、affected_cells。
P44 不实现区域效果结算，但 ProjectionPatch 要能更新 changed_area_effects。
```

### 5.7 多 Agent 同时行动问题

后续会出现：

```text
玩家等待一段时间。
多个 NPC 移动。
多只狼移动。
区域效果按世界时间步推进。
同一时间推进窗口内多个实体可能争抢同一格子。
```

P44 防线：

```text
P44 不做完整调度，但 WorldRuntime 必须有 world_tick / world_time，用来表示世界时间推进，不表示回合制。
SystemTickCommand 只能通过管线推进 world_tick / world_time。
后续 Agent Command 必须带 source = AgentDecision / BeastDecision。
```

命名约束：

```text
文档和代码中禁止把 world_tick 称为 turn / round / 回合。
world_tick 只表示模拟时间步，用于排序、复盘、持续效果和调度。
玩家 Command、NPC Command、区域效果 Command 都可以推进时间，但不代表固定轮流行动。
```

### 5.8 前端投影过大问题

后续地图很大，如果每次返回全图会卡：

```text
手机端只能接受当前视野和变化。
移动一步不能刷新全世界。
区域生成不能一次推送所有隐藏格子。
```

P44 防线：

```text
ProjectionPatch 默认返回 changed_cells / changed_entities。
只有首次进入或版本不匹配时 requires_full_refresh = true。
FullSnapshot 与 Patch 要分清。
```

### 5.9 复盘与确定性问题

后续 bug 需要重放：

```text
同一个 seed 生成同一个世界。
同一串 Command 产生同样投影。
随机野兽移动要可复现。
```

P44 防线：

```text
WorldRuntimeConfig 必须有 seed。
GenerateWorldCommand parameters 必须可携带 seed / region_size。
P44 测试必须验证相同 seed 初始世界一致。
```

### 5.10 内容配置与硬编码问题

后续内容会很多：

```text
红果、毒蘑菇、树、石头、狼、火源、矿洞、遗迹。
```

P44 防线：

```text
P44 可以用最小内置测试地形 key，但不能把红果/狼/火把等玩法逻辑写进世界网格核心。
地形、实体原型、生成规则未来由 P42 JSON 内容包接入。
P44 只保存 key，不解释玩法。
```

### 5.11 H5 与引擎耦合问题

后续前端会重做：

```text
手机横屏地图。
点击格子移动。
点击实体打开动作列表。
右侧事件日志。
底部命令栏。
```

P44 防线：

```text
前端只读 ProjectionPatch 和 available_commands。
P44 不向前端暴露内部 WorldRuntime 指针或调试结构。
```

### 5.12 地图玩法无聊问题

如果 P44 只有移动，玩家会觉得没东西玩。

P44 防线：

```text
P44 不是最终可玩阶段，但必须让“移动 -> 看见新格子 -> 观察实体/地形 -> 事件反馈”成立。
可玩内容在 P47-P53 补齐。
```

## 6. 核心设计原则

P44 必须遵守以下原则：

```text
Command 是唯一入口。
WorldRuntime 是世界状态权威。
ProjectionPatch 是前端唯一可见变化。
坐标必须带 layer_key。
实体和格子只保存内容 key，不解释具体玩法。
移动、观察、生成都必须可复盘。
P44 只做空间与投影，不抢背包/采集/知识阶段。
```

权威边界：

| 内容 | 权威来源 | P44 角色 |
|---|---|---|
| 命令入口 | WorldCommandPipeline | 只接入 Handler |
| 世界状态 | WorldRuntime | 新增 |
| 坐标 | WorldCoordinateDto | 复用 |
| 投影变化 | WorldProjectionPatchDto | 扩展填充 |
| 前端按钮 | available_commands | P44 可少量生成或保持为空 |
| 玩法内容 | P42 JSON 内容包 | P44 仅预留 key |
| 条件判断 | P27 表达式系统 | P44 先做最小阻挡校验 |
| 知识形成 | P15-P21/P34-P36 | P44 不接 |


## 7. 坐标从协议预留到运行时权威的迁移说明

当前项目不是完全没有坐标概念，而是处于两个阶段之间：

```text
P43 已经有坐标协议。
P44 才开始建立运行时坐标世界。
```

### 7.1 当前已有的坐标

P43 的 `world_command` 协议层已经定义：

```cpp
struct WorldCoordinateDto {
    int x = 0;
    int y = 0;
    std::string layer_key = "surface";
};
```

并且以下 DTO 已经可以携带坐标：

```text
WorldCommandTargetDto::target_coord
WorldEventDto::coord
WorldFrontendHintDto::target_coord
WorldCommandTargetKind::Coordinate
```

这说明后续前端、测试、Agent 或系统可以表达：

```text
我要对某个 x/y/layer_key 坐标发 Command。
这个事件发生在某个坐标。
这个前端提示要高亮某个坐标。
```

### 7.2 当前还没有的坐标

当前旧系统里的玩家、NPC、物品、生物没有统一运行时坐标属性。

已确认现状：

```text
ActorSurvivalState：只有 actor_id / hunger / health，没有 x/y/layer_key。
AgentDefinition / AgentRuntime：描述 Agent 行为、认知、决策，没有地图坐标。
Object::WorldObject：只有 object_id / definition_id / quantity / flag，没有地图坐标。
world_interaction::WorldObjectInstance：只有 location_key 字符串，不是真正二维坐标。
world_interaction::WorldActorRuntimeState：没有坐标。
WorldThreatRuntimeState：没有坐标。
DialogScenarioObject / DialogObjectRuntimeState：是旧 H5 场景对象，不是开放世界坐标实体。
```

所以现在的准确结论是：

```text
P43 = 坐标进入协议。
P44 = 坐标成为世界状态权威。
```

### 7.3 P44 必须建立的坐标权威

从 P44 开始，凡是在地图上的东西，都必须能落到坐标上：

```text
玩家：WorldActorRuntime.coord。
NPC：WorldActorRuntime.coord。
野兽 / 生物：WorldActorRuntime.coord 或 WorldEntityInstance.coord，取决于它是否具备 Agent 行为。
地图物品：WorldEntityInstance.coord。
资源点：WorldEntityInstance.coord。
掉落物：WorldEntityInstance.coord。
格子：WorldCellRuntime.coord。
区域：由多个 WorldCellCoord 或 AreaShape 描述。
```

统一坐标格式必须是：

```text
x
y
layer_key
```

禁止只用：

```text
location_key
scene_key
room_key
object_key
entity_key
```

来代替地图坐标。

这些 key 可以作为内容、场景、区域或实例标识存在，但不能代替开放世界运行时坐标。

### 7.4 不在地图上的东西如何处理

不是所有东西都必须永远有地图坐标。

例如：

```text
玩家背包里的火把。
NPC 手里的工具。
营地库存里的木头。
箱子里的石头。
已经被吃掉的红果。
尚未生成的生物。
```

这些对象可以没有 `coord`，但必须有明确归属：

```text
location_kind = InInventory / InContainer / Equipped / Nowhere
owner_ref
container_ref
```

这样后续不会再次出现“同伴火把从哪里来”“营地物品是否共享”“制作材料归谁所有”这类混乱。

### 7.5 P44 实现门禁

P44 研发必须满足：

```text
WorldCellRuntime 必须有 coord。
WorldActorRuntime 必须有 coord。
WorldEntityInstance 在 OnMap 状态下必须有 coord。
MoveCommand 必须修改 Actor 坐标。
ProjectionPatch 必须返回坐标字段。
InspectCommand 必须能按 Coordinate 或 Entity 定位目标。
旧 location_key 不能作为开放世界主坐标。
```

测试必须覆盖：

```text
玩家生成后有坐标。
玩家移动后坐标变化。
地图实体生成后有坐标。
移动到阻挡坐标失败。
ProjectionPatch 中 changed_cells / changed_entities 带 x/y/layer_key。
不在地图上的实体可以没有 coord，但必须有 location_kind 和 owner/container。
```

## 8. 新增模块概览

建议新增模块：

```text
backend/include/pathfinder/world_runtime/
backend/src/world_runtime/
backend/tests/unit/world_runtime/
backend/tests/integration/world_runtime/
```

不建议把世界网格直接塞进 `world_command`。

原因：

```text
world_command 负责命令管线和协议。
world_runtime 负责世界状态和空间规则。
两者通过 Handler / Service 接口协作。
```

## 9. 新增核心枚举

### 9.1 WorldLayerPolicy

```cpp
enum class WorldLayerPolicy {
    SurfaceOnly,        // 当前世界只启用 surface
    ExplicitLayerOnly,  // 只能在已生成 layer 内移动
    AllowLayerSpawn     // 未来可动态生成 layer
};
```

中文说明：

```text
世界层策略，用来控制当前版本是否允许地下、空中、水中等层级动态出现。
P44 默认 SurfaceOnly。
```

### 9.2 WorldCellVisibility

```cpp
enum class WorldCellVisibility {
    Unknown,      // 从未见过
    Discovered,   // 见过但当前不可见
    Visible       // 当前可见
};
```

中文说明：

```text
格子可见状态。前端只能显示 Visible 或 Discovered 的安全信息。
Unknown 不能泄露资源和敌人。
```

### 9.3 WorldEntityLocationKind

```cpp
enum class WorldEntityLocationKind {
    Nowhere,      // 未放置或已移除
    OnMap,        // 在地图格子上
    InInventory,  // 在背包中，P45 接管
    InContainer,  // 在容器或营地库存中，P45/P后续接管
    Equipped      // 装备在 Actor 身上，P45/P后续接管
};
```

中文说明：

```text
实体位置类型。P44 只真正处理 OnMap，但必须预留其他位置，避免后续火把归属再次混乱。
```

### 9.4 WorldMoveBlockReason

```cpp
enum class WorldMoveBlockReason {
    None,
    OutOfBounds,
    UnknownLayer,
    NotAdjacent,
    CellBlocked,
    EntityBlocked,
    ActorMissing,
    TargetMissing
};
```

中文说明：

```text
移动失败原因。失败必须进入 WorldCommandResultDto.failure_reason_keys，不能只返回 false。
```

## 10. 新增核心 DTO / 数据结构

### 10.1 WorldRuntimeConfig

```cpp
struct WorldRuntimeConfig {
    std::string world_id;
    uint64_t seed = 0;
    int initial_region_radius = 1;
    int region_size = 16;
    int default_vision_radius = 3;
    WorldLayerPolicy layer_policy = WorldLayerPolicy::SurfaceOnly;
};
```

用途：

```text
定义一个世界如何初始化。
GenerateWorldCommand 可以通过 parameters 覆盖 seed / region_size / vision_radius。
```

### 10.2 WorldCellCoord

```cpp
struct WorldCellCoord {
    int x = 0;
    int y = 0;
    std::string layer_key = "surface";
};
```

要求：

```text
必须能与 WorldCoordinateDto 互转。
必须提供 stable cell_id：layer_key:x:y。
```

### 10.3 WorldCellRuntime

```cpp
struct WorldCellRuntime {
    std::string cell_id;
    WorldCellCoord coord;
    std::string terrain_key;
    std::string region_id;
    bool generated = false;
    bool blocks_movement = false;
    int movement_cost = 1;
    std::vector<std::string> entity_ids;
    std::vector<std::string> tag_keys;
};
```

说明：

```text
terrain_key 只表示内容 key，例如 plain / forest / water / mountain。
P44 不解释 forest 会掉木头，也不解释 water 能不能钓鱼。
```

### 10.4 WorldEntityInstance

```cpp
struct WorldEntityInstance {
    std::string entity_id;
    std::string entity_key;
    std::string display_name_key;
    WorldEntityLocationKind location_kind = WorldEntityLocationKind::Nowhere;
    std::optional<WorldCellCoord> coord;
    std::string owner_ref;
    std::string container_ref;
    bool blocks_movement = false;
    bool visible_by_default = true;
    std::vector<std::string> tag_keys;
};
```

说明：

```text
entity_key 是内容原型 key。
entity_id 是实例 id。
P44 不允许用 entity_key 当实例 id。
```

### 10.5 WorldActorRuntime

```cpp
struct WorldActorRuntime {
    std::string actor_key;
    std::string entity_id;
    WorldCellCoord coord;
    int vision_radius = 3;
    bool is_player_controlled = false;
};
```

说明：

```text
玩家、NPC、野兽都可以有 ActorRuntime。
P44 只需要玩家 Actor，但结构必须允许 NPC 和野兽接入。
```

### 10.6 WorldExplorationState

```cpp
struct WorldExplorationState {
    std::string actor_key;
    std::map<std::string, WorldCellVisibility> cell_visibility_by_id;
};
```

说明：

```text
记录某个 actor 看过哪些格子。
P44 只需要 player 的探索状态。
后续 NPC 自己探索时复用。
```

### 10.7 WorldRuntimeSnapshot

```cpp
struct WorldRuntimeSnapshot {
    std::string world_id;
    uint64_t seed = 0;
    uint64_t world_tick = 0;
    uint64_t state_version = 0;
    std::map<std::string, WorldCellRuntime> cells;
    std::map<std::string, WorldEntityInstance> entities;
    std::map<std::string, WorldActorRuntime> actors;
};
```

说明：

```text
P44 可以先用于测试和调试，不要求完整存档格式。
P30/P31 存档复盘阶段后续再接正式序列化。
```

## 11. 新增核心服务接口

### 11.1 IWorldRuntime

```cpp
class IWorldRuntime {
public:
    virtual ~IWorldRuntime() = default;

    virtual foundation::Result<void> initialize(const WorldRuntimeConfig& config) = 0;
    virtual uint64_t currentWorldTick() const = 0;
    virtual uint64_t stateVersion() const = 0;

    virtual foundation::Result<const WorldCellRuntime*> findCell(const WorldCellCoord& coord) const = 0;
    virtual foundation::Result<const WorldEntityInstance*> findEntity(const std::string& entity_id) const = 0;
    virtual foundation::Result<const WorldActorRuntime*> findActor(const std::string& actor_key) const = 0;

    virtual foundation::Result<void> generateInitialWorld(const WorldRuntimeConfig& config) = 0;
    virtual foundation::Result<MoveActorResult> moveActor(const std::string& actor_key, const WorldCellCoord& target) = 0;
    virtual foundation::Result<InspectWorldResult> inspect(const std::string& actor_key, const WorldCommandTargetDto& target) const = 0;
    virtual foundation::Result<AdvanceWorldTimeResult> advanceWorldTime(uint64_t tick_delta) = 0;

    virtual foundation::Result<WorldRuntimeSnapshot> snapshotForDebug() const = 0;
};
```

说明：

```text
IWorldRuntime 是世界状态权威。
但外部不能直接把它暴露给前端。
Command Handler 可以使用它执行世界变化。
```

### 11.2 WorldGridRuntime

```cpp
class WorldGridRuntime final : public IWorldRuntime {
public:
    foundation::Result<void> initialize(const WorldRuntimeConfig& config) override;
    foundation::Result<void> generateInitialWorld(const WorldRuntimeConfig& config) override;
    foundation::Result<MoveActorResult> moveActor(const std::string& actor_key, const WorldCellCoord& target) override;
    foundation::Result<InspectWorldResult> inspect(const std::string& actor_key, const WorldCommandTargetDto& target) const override;
    foundation::Result<AdvanceWorldTimeResult> advanceWorldTime(uint64_t tick_delta) override;
};
```

职责：

```text
管理 cells / entities / actors / exploration / world_tick / state_version。
校验最小移动规则。
生成最小测试世界。
输出变更结果给 Command Handler。
```

### 11.3 WorldProjectionAdapter

```cpp
class WorldProjectionAdapter {
public:
    foundation::Result<std::vector<WorldCellPatchDto>> buildCellPatches(
        const std::vector<std::string>& changed_cell_ids,
        const std::string& viewer_actor_key,
        const IWorldRuntime& runtime) const;

    foundation::Result<std::vector<WorldEntityPatchDto>> buildEntityPatches(
        const std::vector<std::string>& changed_entity_ids,
        const std::string& viewer_actor_key,
        const IWorldRuntime& runtime) const;
};
```

职责：

```text
把内部 WorldRuntime 状态转换成 WorldProjectionPatchDto 的字段。
过滤不可见信息。
保证前端不读取内部 Runtime。
```

### 11.4 WorldCommandRuntimeBridge

```cpp
class WorldCommandRuntimeBridge {
public:
    explicit WorldCommandRuntimeBridge(IWorldRuntime& runtime);

    foundation::Result<WorldCommandExecutionResult> handleGenerateWorld(const WorldCommandContext& context, const WorldCommandDto& command);
    foundation::Result<WorldCommandExecutionResult> handleMove(const WorldCommandContext& context, const WorldCommandDto& command);
    foundation::Result<WorldCommandExecutionResult> handleInspect(const WorldCommandContext& context, const WorldCommandDto& command);
    foundation::Result<WorldCommandExecutionResult> handleWait(const WorldCommandContext& context, const WorldCommandDto& command);
};
```

说明：

```text
Bridge 不是新的管线。
它只是 Handler 调用 Runtime 的适配层。
后续也可以直接让 Handler 持有 IWorldRuntime&，但必须保持职责清楚。
```

## 12. P43 Handler 接入方式

P44 应新增或替换以下 Handler：

```text
GenerateWorldCommandHandler：从 stub 变成调用 runtime.generateInitialWorld。
MoveCommandHandler：新增，调用 runtime.moveActor。
InspectCommandHandler：增强，调用 runtime.inspect。
WaitCommandHandler：增强，调用 runtime.advanceWorldTime。
```

注册方式：

```cpp
registry.registerHandler(createGenerateWorldCommandHandler(runtime));
registry.registerHandler(createMoveCommandHandler(runtime));
registry.registerHandler(createInspectCommandHandler(runtime));
registry.registerHandler(createWaitCommandHandler(runtime));
registry.bindActionKey("move", WorldCommandKind::Move);
registry.bindActionKey("inspect", WorldCommandKind::Inspect);
registry.bindActionKey("wait", WorldCommandKind::Wait);
registry.bindActionKey("generate_world", WorldCommandKind::GenerateWorld);
```

注意：

```text
不能删除 P43 的无 runtime 最小 handler 构造能力，测试中仍可使用 stub handler。
建议新增带 runtime 的 handler factory，避免破坏 P43 单元测试。
```

## 13. Command 流程设计

### 13.1 GenerateWorldCommand

输入：

```text
command_kind = GenerateWorld
source = WorldGeneration / Test / PlayerInput
parameters.seed
parameters.world_id
parameters.region_size
parameters.vision_radius
```

流程：

```text
Pipeline 接收 command。
Dispatcher 找到 GenerateWorldCommandHandler。
Handler 读取 parameters，生成 WorldRuntimeConfig。
runtime.generateInitialWorld(config)。
创建玩家 actor，放在出生点。
更新玩家视野。
返回 changed_cells / changed_entities / event_feed。
```

输出：

```text
WorldCommandResultKind::Succeeded
WorldEventDto: WorldGenerated
ProjectionPatch.requires_full_refresh = true 或返回初始可见 patch
```

### 13.2 MoveCommand

输入：

```text
command_kind = Move
actor_key = player 或 npc
source = PlayerInput / AgentDecision / BeastDecision
target.target_kind = Coordinate
target.target_coord = x/y/layer_key
```

流程：

```text
校验 actor 存在。
校验目标 cell 存在。
校验同 layer 或未来允许 layer transition。
校验相邻。
校验 cell / entity 是否阻挡。
移动 actor。
更新 exploration visible / discovered。
返回旧格子、新格子、视野变化格子、actor 实体变化。
```

失败原因：

```text
actor_missing
target_cell_missing
unknown_layer
not_adjacent
cell_blocked
entity_blocked
```

### 13.3 InspectCommand

输入：

```text
command_kind = Inspect
actor_key = player
target = Coordinate / Entity
```

流程：

```text
校验目标在已发现或当前可见范围。
读取目标安全投影。
返回 Inspect event。
不泄露隐藏效果。
```

输出：

```text
event_feed 中给出观察结果。
projection_patch 可更新该实体/格子的 inspected 标记。
```

### 13.4 WaitCommand

输入：

```text
command_kind = Wait
actor_key = player 或 system
parameters.tick_delta 可选，默认 1，表示推进多少世界时间步，不表示回合数
```

流程：

```text
runtime.advanceWorldTime。
P44 只增加 world_tick，不执行 NPC AI；world_tick 是模拟时间步，不是回合制。
返回 TimePassed event。
```

后续：

```text
P49/P50 以后 SystemTick 可按时间推进派生 AgentDecision Command。
```

## 14. 投影 Patch 字段规范

P44 必须开始真正填充 P43 已定义的 patch：

### 14.1 WorldCellPatchDto fields

建议字段：

```text
cell_id
x
y
layer_key
terrain_key
region_id
visibility
blocks_movement
movement_cost
tag_keys
```

可见性规则：

```text
Unknown 不发送。
Discovered 可发送 terrain_key，但不发送当前实体列表。
Visible 可发送地形和可见实体引用。
```

### 14.2 WorldEntityPatchDto fields

建议字段：

```text
entity_id
entity_key
display_name_key
x
y
layer_key
location_kind
visible
blocks_movement
tag_keys
```

注意：

```text
entity_key 是内容 key，不是实例 id。
前端可以显示 display_name_key 对应的本地化文本。
前端不能根据 entity_key 推导可采集、可食用、可制作。
```

### 14.3 requires_full_refresh

规则：

```text
初始 GenerateWorld 后可以 true。
普通 Move / Inspect / Wait 应尽量 false。
如果前端 projection_version 与后端不一致，后续阶段可要求 full refresh。
```

## 15. 最小世界生成规则

P44 允许使用一个确定性最小生成器，但必须标注为测试世界生成器，不是长期内容系统。

建议生成：

```text
region_size = 9 或 16。
layer_key = surface。
出生点在 0,0。
周围有 plain / forest / water / mountain 等 terrain_key。
水和山 blocks_movement = true。
放置少量非玩法实体，例如 unknown_bush / loose_stone，只用于 inspect，不做采集。
```

禁止：

```text
不要在 P44 生成红果并实现吃红果。
不要在 P44 生成狼并实现攻击。
不要在 P44 生成火把并实现驱赶。
```

这些属于后续 P47-P49。

## 16. 测试设计

P44 必须新增测试。

### 16.1 WorldRuntime 单元测试

```text
world_runtime_coordinate_id：cell_id 包含 layer/x/y。
world_runtime_generate_deterministic：相同 seed 生成相同初始世界。
world_runtime_find_actor：生成世界后能找到 player。
world_runtime_move_adjacent：玩家可移动到相邻可通行格。
world_runtime_move_blocked：玩家不能移动到 blocked cell。
world_runtime_move_not_adjacent：玩家不能一次跨多格。
world_runtime_visibility_update：移动后 visible/discovered 更新。
world_runtime_time_step：wait 推进 world_tick 和 state_version。
```

### 16.2 Command 集成测试

```text
world_command_generate_world_runtime：GenerateWorld 通过 Pipeline 生成世界。
world_command_move_runtime：Move 通过 Pipeline 改变 actor 坐标。
world_command_move_blocked_runtime：Blocked 结果返回 failure_reason_keys。
world_command_inspect_runtime：Inspect 返回 event_feed。
world_command_projection_patch_runtime：Move 返回 changed_cells / changed_entities。
world_command_no_direct_world_mutation：测试只通过 Pipeline 改世界，不直接调用 moveActor 验收主流程。
```

### 16.3 架构保护测试

```text
world_runtime_no_playbook_rules：world_runtime 核心不得出现 red_berry / wolf / torch / axe 等玩法 key。
world_command_runtime_bridge_no_frontend_dependency：runtime / handler 不依赖 H5 目录。
world_projection_no_hidden_unknown_cells：Unknown 格子不出现在 patch。
```

## 17. 验收标准

P44 验收必须满足：

```text
构建通过。
P43 world_command 测试仍全部通过。
新增 P44 world_runtime 测试全部通过。
GenerateWorld / Move / Inspect / Wait 可以通过 WorldCommandPipeline 执行。
WorldProjectionPatchDto 中 changed_cells / changed_entities 有真实内容。
玩家移动后坐标变化、视野变化、patch 变化一致。
阻挡格移动失败，并返回明确 failure_reason_keys。
world_runtime 核心没有写死红果、狼、火把、斧头等具体玩法规则。
```

不要求：

```text
不要求 H5 能画地图。
不要求采集制作。
不要求 NPC 自主行动。
不要求真实存档。
```

## 18. 实现建议顺序

建议研发按以下顺序小步实现：

```text
1. 新建 world_runtime 模块和 CMake target。
2. 实现 WorldCellCoord / cell_id / 坐标转换。
3. 实现 WorldRuntimeConfig / WorldCellRuntime / WorldEntityInstance / WorldActorRuntime。
4. 实现 WorldGridRuntime::generateInitialWorld。
5. 实现 findCell / findActor / findEntity。
6. 实现 moveActor 最小校验。
7. 实现 exploration 视野更新。
8. 实现 WorldProjectionAdapter。
9. 新增 runtime-aware GenerateWorld / Move / Inspect / Wait Handler。
10. 接入 P43 registry 测试。
11. 补单元测试和集成测试。
12. 写审核报告。
```

## 19. 关键注意事项

### 19.1 不要把 P44 做成旧 H5 的另一个版本

P44 的产物是后端世界状态和投影，不是页面按钮。

### 19.2 不要为了最小移动破坏未来背包

实体必须区分：

```text
实例 id
内容 key
位置类型
所有者/容器预留
```

否则后续火把、材料、掉落物、营地库存会再次混乱。

### 19.3 不要让前端推规则

前端可以知道：

```text
这个格子可见。
这个实体在这里。
这个 command 可以点。
```

前端不能自己判断：

```text
这棵树可以砍。
这个果子能吃。
这个狼怕火。
```

### 19.4 不要让 Runtime 解释内容玩法

Runtime 可以知道：

```text
terrain_key = water
blocks_movement = true
```

Runtime 不应该知道：

```text
water 可以喝。
forest 可以掉木头。
wolf 怕 torch。
```

这些属于内容、反应、知识、效果系统。

### 19.5 不要一次实现复杂寻路

P44 只需要相邻移动。

未来寻路可以建立在：

```text
WorldRuntime 查询邻居。
Command 序列执行移动。
每一步仍走 Pipeline。
```

### 19.6 不要忽视失败反馈

移动失败不是静默失败。

必须返回：

```text
WorldCommandResultKind::Blocked
failure_reason_keys
可选 frontend_hints
```

否则前端和玩家不知道为什么不能走。

## 20. 与后续阶段关系

P44 完成后，后续阶段应这样接：

```text
P45：背包、容器、掉落物、物品实例归属。
P46：available_commands 根据地图、实体、知识、距离生成。
P47：采集、砍伐、挖掘接入地图实体。
P48：制作、反应链、区域效果接入 Runtime。
P49：经验和知识形成接入世界 Command。
P50：教学和 NPC 知识接入地图距离。
P51：Agent 在地图上自主移动和执行目标链。
P52：野兽、危险、外界打断地图化。
P53：H5 V2 地图界面只消费 ProjectionPatch。
```

## 21. 研发门禁

进入实现前，研发必须确认：

```text
已阅读 P43 设计和复审报告。
不会绕开 WorldCommandPipeline。
不会把 P44 写成只为当前测试服务的假地图。
不会把红果、狼、火把等具体玩法写进 world_runtime 核心。
会保留 layer_key。
会保留 entity_id 和 entity_key 的区别。
会为移动失败写清楚 failure_reason_keys。
会写测试证明 ProjectionPatch 真实返回变化。
```

## 22. 最终结论

P44 是 V2.0 能否成立的关键地基。

它不追求内容丰富，而追求空间规则正确：

```text
世界有格子。
实体有位置。
玩家能移动。
视野会变化。
投影能更新。
所有行为都走 Command。
```

只要 P44 做稳，后续采集、背包、制作、知识、NPC、野兽、区域事件、H5 地图都可以沿着同一条架构接入。

P44 通过后，项目才真正从“文本规则原型”进入“开放世界后端原型”。
