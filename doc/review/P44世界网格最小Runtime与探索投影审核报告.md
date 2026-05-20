# P44 世界网格最小 Runtime 与探索投影审核报告

审核日期：2026-05-20

## 1. 审核结论

结论：P44 当前不能验收，需要修复 2 个 P0 阻塞问题和 2 个 P1 问题后复审。

本次实现已经完成了大量 P44 基础骨架：

```text
world_runtime 模块。
WorldCellCoord / WorldCellRuntime / WorldEntityInstance / WorldActorRuntime。
WorldGridRuntime。
WorldProjectionAdapter。
WorldCommandRuntimeBridge。
runtime-aware GenerateWorld / Move / Inspect / Wait handlers。
P44 单元测试与集成测试。
```

并且专项测试全部通过：

```text
world_command / world_runtime 相关 41/41 tests passed。
```

但是 P44 的目标不是“测试通过”本身，而是要建立 V2 世界网格的正确架构边界。当前存在两个会影响后续长期扩展的核心问题：

```text
P0-1：world_command 与 world_runtime 形成双向依赖，破坏 P43 统一 Command 管线的基础边界。
P0-2：WorldCommandPipeline 返回的 projection_patch 仍然是空壳，runtime 的 changed_cells / changed_entities 没有进入真正的 Command 响应。
```

因此本次审核结论为：

```text
暂不验收。
必须修复 P0 后复审。
```

## 2. 审核范围

代码范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/world_runtime/
backend/src/world_runtime/
backend/include/pathfinder/world_command/world_command_handlers.h
backend/src/world_command/handlers/generate_world_command_handler.cpp
backend/src/world_command/handlers/move_command_handler.cpp
backend/src/world_command/handlers/inspect_command_handler.cpp
backend/src/world_command/handlers/wait_command_handler.cpp
backend/tests/unit/world_runtime/
backend/tests/integration/world_runtime/
```

设计依据：

```text
doc/74_P44世界网格最小Runtime与探索投影设计.md
doc/73_P43统一世界Command管线前置设计.md
doc/review/P43统一世界Command管线前置复审报告.md
```

## 3. 已执行验证

执行命令：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_runtime pathfinder_tests_world_runtime_integration pathfinder_tests_world_command pathfinder_tests_world_command_integration -j 2
ctest --test-dir build/backend -R '^(world_runtime|world_command_)' --output-on-failure
```

结果：

```text
41/41 tests passed
0 failed
```

说明：

```text
当前代码可构建。
当前 P43/P44 专项测试可通过。
但测试没有覆盖两个最关键的架构门禁，所以不能仅凭测试通过验收。
```

## 4. 符合设计的部分

### 4.1 坐标模型已开始落地

已实现：

```text
WorldCellCoord：x / y / layer_key。
WorldCellRuntime.coord。
WorldEntityInstance.coord。
WorldActorRuntime.coord。
WorldCoordinateDto 与 Runtime 坐标可转换。
```

这符合 P44 “从协议坐标进入运行时坐标权威”的目标。

### 4.2 世界 Runtime 骨架基本成立

已实现：

```text
IWorldRuntime。
WorldGridRuntime。
generateInitialWorld。
findCell / findEntity / findActor。
moveActor。
inspect。
advanceWorldTime。
snapshotForDebug。
```

这符合 P44 最小 Runtime 的结构要求。

### 4.3 world_tick 语义没有被写成回合制

实现中使用：

```text
currentWorldTick
advanceWorldTime
tick_delta
world_tick
```

未发现 `turn / round / 回合` 作为核心运行时概念。

这符合修正版 P44 设计。

### 4.4 基础移动、阻挡、视野测试存在

已覆盖：

```text
相邻移动。
阻挡格失败。
越界失败。
非相邻失败。
移动后 Actor 坐标变化。
视野 visible/discovered 更新。
时间步推进。
```

### 4.5 未发现红果、狼、火把、斧头等具体玩法写入 world_runtime 核心

扫描：

```text
backend/include/pathfinder/world_runtime
backend/src/world_runtime
backend/include/pathfinder/world_command
backend/src/world_command
```

未发现：

```text
red_berry / wolf / torch / axe / poison_mushroom / camp_fire
```

当前有：

```text
plain / forest / water / mountain
unknown_bush / loose_stone
```

这些是 P44 设计允许的最小测试地形和 inspect-only 实体，但后续必须接入 P42 JSON，不应长期作为真实内容来源。

## 5. P0 阻塞问题

## P0-1：world_command 与 world_runtime 形成双向依赖，破坏架构边界

位置：

```text
backend/CMakeLists.txt
backend/include/pathfinder/world_command/world_command_handlers.h
backend/src/world_command/handlers/generate_world_command_handler.cpp
backend/src/world_command/handlers/inspect_command_handler.cpp
backend/src/world_command/handlers/wait_command_handler.cpp
backend/src/world_command/handlers/move_command_handler.cpp
backend/include/pathfinder/world_runtime/iworld_runtime.h
backend/include/pathfinder/world_runtime/world_command_runtime_bridge.h
```

现状：

```cmake
pathfinder_world_runtime -> pathfinder_world_command
pathfinder_world_command -> pathfinder_world_runtime
```

并且：

```cpp
world_command_handlers.h include pathfinder/world_runtime/iworld_runtime.h
world_command handler cpp include pathfinder/world_runtime/world_command_runtime_bridge.h
world_runtime/iworld_runtime.h include pathfinder/world_command/world_command_types.h
```

问题：

```text
P43 设计要求 world_command 是统一命令管线基础层。
P44 设计要求 world_runtime 通过 Handler / Bridge 接入 P43，而不是让 P43 反向依赖 P44。
当前实现把 runtime-aware handler 放进 pathfinder_world_command target，导致 world_command 反过来依赖 world_runtime。
这会使 P43 不再是独立底座，而是和 P44 Runtime 互相纠缠。
```

风险：

```text
后续 P45 背包、P47 采集、P48 反应、P49 知识如果继续往 world_command handler 里塞 runtime 依赖，world_command 会变成巨型玩法聚合层。
Dispatcher/HandlerRegistry 的抽象会被破坏。
低上下文 AI 后续很容易继续把各阶段 runtime 直接 include 进 world_command。
模块无法独立测试和复用。
```

修复建议：

```text
保留 pathfinder_world_command 只包含 P43 协议、Pipeline、Dispatcher、Registry、基础 stub handler。
新增 pathfinder_world_runtime_command 或 pathfinder_world_runtime_handlers target。
把 runtime-aware GenerateWorld / Move / Inspect / Wait handlers 移到新 target。
该新 target 可以依赖 pathfinder_world_command 和 pathfinder_world_runtime。
world_runtime 可以依赖 world_command DTO，或进一步拆出 world_command_types 小 target。
但 pathfinder_world_command 不能依赖 pathfinder_world_runtime。
```

建议目标依赖：

```text
pathfinder_foundation
  -> pathfinder_world_command
  -> pathfinder_world_runtime
  -> pathfinder_world_runtime_command
```

或更干净：

```text
pathfinder_foundation
  -> pathfinder_world_command_types
  -> pathfinder_world_command
  -> pathfinder_world_runtime
  -> pathfinder_world_runtime_command
```

P44 最小修复可采用第一种，不必现在拆 command_types。

必须补测试/门禁：

```text
CMake / 架构扫描：pathfinder_world_command 不得 include pathfinder/world_runtime。
源码扫描：backend/src/world_command 不得 include world_runtime。
```

## P0-2：Pipeline 响应没有真正返回 Runtime ProjectionPatch

位置：

```text
backend/src/world_command/world_command_pipeline.cpp
backend/src/world_runtime/world_command_runtime_bridge.cpp
backend/tests/integration/world_runtime/world_runtime_command_flow_test.cpp
```

现状：

`WorldCommandRuntimeBridge` 有能力构建 patch：

```cpp
WorldCommandRuntimeBridge::buildProjectionPatch(...)
```

但 `WorldCommandPipeline::execute()` 并没有使用 runtime bridge 的 patch。

Pipeline 仍然只执行：

```cpp
auto patch_result = patch_builder_.setVersions(projection_version_, new_version).build();
response.projection_patch = patch_result.value();
```

而 `ProjectionPatchBuilder` 当前只设置：

```text
base_projection_version
new_projection_version
requires_full_refresh
```

不会填充：

```text
changed_cells
changed_entities
changed_inventories
changed_knowledge
changed_area_effects
```

设计要求：

```text
GenerateWorld / Move / Inspect / Wait 可以通过 WorldCommandPipeline 执行。
WorldProjectionPatchDto 中 changed_cells / changed_entities 有真实内容。
玩家移动后坐标变化、视野变化、patch 变化一致。
```

当前测试问题：

```text
world_command_projection_patch_runtime 是手动调用 bridge.buildProjectionPatch。
world_command_pipeline_with_runtime 只断言 pipeline.execute 成功，没有断言 pipeline response.projection_patch.changed_cells / changed_entities。
```

这意味着：

```text
真正前端调用 Pipeline 时，拿不到 P44 地图变化。
P44 的投影能力存在于 Bridge 的旁路 helper 中，没有接入统一 Command 响应。
```

风险：

```text
前端无法只消费 WorldCommandResponseDto 来画地图。
后续 H5 V2 会被迫绕过 Pipeline 去调用 Bridge 或 Runtime。
这正好违背 P43/P44 的核心目标。
```

修复建议：

方案 A，推荐：扩展 `WorldCommandExecutionResult`：

```cpp
std::optional<WorldProjectionPatchDto> projection_patch_override;
std::vector<std::string> changed_cell_ids;
std::vector<std::string> changed_entity_ids;
bool requires_full_refresh = false;
```

然后 Pipeline 在 BuildProjectionPatch 阶段：

```text
如果 execution.projection_patch_override 有值，合并版本号后写入 response.projection_patch。
否则用 ProjectionPatchBuilder 默认空 patch。
```

方案 B：让 Handler 返回 changed refs，Pipeline 持有 ProjectionAdapter：

```text
不推荐 P44 直接这么做，因为会进一步让 world_command 依赖 world_runtime。
```

因此 P44 应优先采用方案 A：Handler/Bridge 生成或携带 patch 结果，Pipeline 只负责拷贝/合并，不理解 runtime。

必须补测试：

```text
通过 WorldCommandPipeline 执行 GenerateWorld 后，response.projection_patch.changed_cells 非空。
通过 WorldCommandPipeline 执行 Move 后，response.projection_patch.changed_cells 非空。
通过 WorldCommandPipeline 执行 Move 后，response.projection_patch.changed_entities 包含 player entity。
projection_patch 中 cell/entity fields 必须包含 x / y / layer_key。
```

## 6. P1 结构性问题

## P1-1：初始世界实体 ID 不是严格确定性的，影响复盘和重放

位置：

```text
backend/src/world_runtime/world_grid_runtime.cpp
```

现状：

```cpp
std::string WorldGridRuntime::generateEntityId(const std::string& entity_key) {
    static uint64_t counter = 0;
    return entity_key + "_" + std::to_string(++counter) + "_" + std::to_string(world_tick_);
}
```

问题：

```text
counter 是函数静态变量，跨 WorldGridRuntime 实例共享。
同一进程中连续生成两个相同 seed 的世界，实体 ID 会不同。
当前 deterministic 测试只比较 cells size、entities size、actors size 和 terrain_key，没有比较 entity_id / entity_key / entity coord 的一致性。
```

风险：

```text
Command Replay 依赖稳定实体 id。
如果同 seed 生成的实体 id 不一致，后续 target_entity_id 命令无法稳定重放。
存档、复盘、测试基线都会受影响。
```

修复建议：

```text
不要使用 static counter。
把 entity_id 生成改成与 world_id / seed / coord / entity_key / local_index 有关的稳定 ID。
例如：entity_key + "_" + coord.cellId() + "_" + local_index。
或者在 WorldGridRuntime 内部维护实例 counter，并在 generateInitialWorld 时重置。
```

推荐 P44 直接做稳定坐标派生 ID：

```text
unknown_bush_surface_1_2_0
player_surface_0_0
```

必须补测试：

```text
相同 seed 连续生成两个 runtime，snapshot.entities 的 key 集合完全一致。
player entity_id 完全一致。
同一实体坐标和 entity_key 完全一致。
```

## P1-2：生成世界时把 moved-from player.entity_id 写入 origin cell，存在隐性错误

位置：

```text
backend/src/world_runtime/world_grid_runtime.cpp
```

现状：

```cpp
std::string player_entity_id = player.entity_id;
WorldCellCoord player_coord = player.coord;
actors_[player.actor_key] = std::move(player);
...
origin_it->second.entity_ids.push_back(player.entity_id);
```

问题：

```text
player 已经 std::move 进 actors_。
之后再读取 player.entity_id 属于 moved-from 对象读取。
std::string moved-from 后通常为空，但标准只保证有效未指定状态。
```

风险：

```text
origin cell 可能没有正确包含 player entity。
初始 projection 的 entity_ids 可能缺 player。
后续移动时旧 cell 删除 actor.entity_id 删除不到初始错误值。
```

修复建议：

```cpp
origin_it->second.entity_ids.push_back(player_entity_id);
```

必须补测试：

```text
生成世界后 origin cell 的 entity_ids 包含 player actor 的 entity_id。
初始 ProjectionPatch 的 origin cell entity_ids 包含 player entity。
```

## 7. P2 可改进问题

### 7.1 ProjectionAdapter 中存在无效/调试残留代码

位置：

```text
backend/src/world_runtime/world_projection_adapter.cpp
```

现状：

```cpp
for (const auto& cell_id : changed_cell_ids) {
    auto cell_result = runtime.findCell(WorldCellCoord{}); // dummy
    auto it = grid->snapshotForDebug().value().cells.find(cell_id);
}
```

这段循环不产生任何结果，只是调试残留。

风险：

```text
每个 cell_id 都会额外调用 findCell(surface:0:0) 和 snapshotForDebug。
如果 origin 不存在，虽然当前不报错，但逻辑脏。
未来 snapshotForDebug 变重后会浪费性能。
```

修复建议：

```text
删除第一段无效循环。
保留后面 parse cell_id 的真实逻辑。
```

### 7.2 参数解析没有错误保护

位置：

```text
backend/src/world_runtime/world_command_runtime_bridge.cpp
```

现状：

```cpp
std::stoull(command.parameters.at("seed"))
std::stoi(command.parameters.at("region_size"))
std::stoi(command.parameters.at("vision_radius"))
std::stoull(command.parameters.at("tick_delta"))
```

如果前端传入非数字字符串，可能抛异常并中断。

修复建议：

```text
用安全 parse helper，失败时返回 Failed / invalid_parameter。
```

P44 可作为 P1 或 P2 修，建议一起修，避免 H5 V2 接入时崩溃。

## 8. 测试覆盖评价

当前测试优点：

```text
数量较多，覆盖基础坐标、移动、阻挡、视野、时间步、投影 adapter、pipeline smoke。
P43 测试仍通过。
```

当前测试缺口：

```text
没有验证 world_command target 不依赖 world_runtime。
没有验证 pipeline response 真正包含 changed_cells / changed_entities。
没有验证 deterministic entity id。
没有验证 origin cell 包含 player entity。
没有验证参数解析异常。
```

必须新增的复审测试：

```text
world_command_pipeline_projection_patch_runtime。
world_runtime_deterministic_entity_ids。
world_runtime_origin_contains_player_entity。
world_command_runtime_invalid_parameters。
architecture_world_command_no_runtime_dependency。
```

## 9. 设计符合度对照

| 设计要求 | 当前状态 | 结论 |
|---|---|---|
| world_runtime 模块 | 已实现 | 通过 |
| 坐标 x/y/layer_key | 已实现 | 通过 |
| Actor / Entity / Cell 坐标 | 已实现 | 基础通过 |
| world_tick 非回合制 | 已实现 | 通过 |
| MoveCommand 修改坐标 | 已实现 | 通过 |
| InspectCommand 可观察 | 已实现 | 基础通过 |
| Wait 推进世界时间 | 已实现 | 通过 |
| ProjectionAdapter | 已实现 | 但未接入 Pipeline |
| Pipeline response 返回真实 patch | 未实现 | P0 不通过 |
| P43 world_command 不依赖 P44 runtime | 未满足 | P0 不通过 |
| 同 seed 世界可复盘 | 部分满足 | P1 |
| 初始 player entity 正确挂到 cell | 有隐患 | P1 |
| 无红果/狼/火把/斧头硬编码 | 当前通过 | 通过 |

## 10. 修复清单

### 必须修复后再验收

```text
P0-1：拆掉 world_command <-> world_runtime 双向依赖。
P0-2：让 Pipeline response.projection_patch 真正包含 Runtime changed_cells / changed_entities。
```

### 建议本阶段一起修复

```text
P1-1：实体 ID 改为确定性，不使用 static counter。
P1-2：origin cell 使用 player_entity_id，不读取 moved-from player。
```

### 可顺手修复

```text
删除 ProjectionAdapter 无效循环。
参数解析加 invalid_parameter 防护。
```

## 11. 最终意见

P44 的方向是对的，已经把坐标、网格、Actor、Entity、移动、视野、时间步做出了最小形态。

但当前实现还没有真正完成 P44 最关键的架构闭环：

```text
前端提交 Command -> Pipeline 执行 Runtime -> Response 直接返回地图 ProjectionPatch。
```

并且当前模块依赖方向会让 P43 基础层被 P44 运行时反向污染。

最终审核结论：

```text
P44 暂不验收。
修复 P0-1 / P0-2 后复审。
```
