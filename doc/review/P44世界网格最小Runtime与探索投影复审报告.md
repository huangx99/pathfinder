# P44 世界网格最小 Runtime 与探索投影复审报告

复审日期：2026-05-20

## 1. 复审结论

结论：P44 本次复审通过，可以验收进入下一阶段。

上次审核提出的核心问题：

```text
P0-1：world_command 与 world_runtime 形成双向依赖，破坏 P43/P44 架构边界。
P0-2：WorldCommandPipeline 返回的 projection_patch 是空壳，Runtime changed_cells / changed_entities 没有进入真正 Command 响应。
P1-1：实体 ID 使用 static counter，同 seed 不保证稳定复盘。
P1-2：初始玩家实体写入 origin cell 时读取 moved-from 的 player.entity_id。
```

本次复审结果：

```text
P0-1 已修复。
P0-2 已修复。
P1-1 已修复。
P1-2 已修复。
```

P44 当前已经达到本阶段目标：

```text
建立二维世界网格 Runtime。
让玩家、实体、格子拥有 x / y / layer_key 坐标。
让 GenerateWorld / Move / Inspect / Wait 接入 Runtime。
让 WorldCommandPipeline 响应携带真实 ProjectionPatch。
保持 P43 world_command 基础层不反向依赖 P44 runtime。
```

最终复审意见：

```text
通过。
可以进入 P45 / 背包、容器、掉落物、物品实例归属阶段。
```

## 2. 复审范围

代码范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/world_command/world_command_handlers.h
backend/include/pathfinder/world_command/world_command_types.h
backend/src/world_command/world_command_pipeline.cpp
backend/include/pathfinder/world_runtime/
backend/src/world_runtime/
backend/tests/unit/world_runtime/
backend/tests/integration/world_runtime/
```

设计依据：

```text
doc/74_P44世界网格最小Runtime与探索投影设计.md
doc/review/P44世界网格最小Runtime与探索投影审核报告.md
```

## 3. 已执行验证

执行命令：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_runtime pathfinder_tests_world_runtime_integration pathfinder_tests_world_command pathfinder_tests_world_command_integration -j 2
ctest --test-dir build/backend -R '^(world_runtime|world_command_|architecture_world_command_no_runtime_dependency)' --output-on-failure
```

结果：

```text
46/46 tests passed
0 failed
```

新增/关键通过项：

```text
world_command_pipeline_projection_patch_runtime
world_runtime_deterministic_entity_ids
world_runtime_origin_contains_player_entity
world_command_runtime_invalid_parameters
architecture_world_command_no_runtime_dependency
```

## 4. 上次问题复审

### 4.1 P0-1：world_command 与 world_runtime 双向依赖

复审结论：已修复。

当前 CMake 结构：

```text
pathfinder_world_command：P43 基础命令管线，只依赖 pathfinder_foundation。
pathfinder_world_runtime：P44 世界 Runtime，依赖 pathfinder_foundation + pathfinder_world_command。
pathfinder_world_command_runtime：P44 runtime-aware handlers，依赖 pathfinder_world_command + pathfinder_world_runtime。
```

当前 `pathfinder_world_command` 不再链接 `pathfinder_world_runtime`。

当前 `backend/include/pathfinder/world_command/world_command_handlers.h` 只声明 P43 stub handlers：

```text
createNoopCommandHandler
createWaitCommandHandler
createInspectCommandHandler
createSystemTickCommandHandler
createGenerateWorldCommandHandler
```

runtime-aware handlers 已移动到：

```text
backend/include/pathfinder/world_runtime/world_command_runtime_handlers.h
backend/src/world_runtime/world_runtime_command_handlers.cpp
```

新增架构保护测试：

```text
architecture_world_command_no_runtime_dependency
```

验证：

```text
backend/src/world_command
backend/include/pathfinder/world_command
```

不得 include：

```text
pathfinder/world_runtime
```

该测试通过。

结论：

```text
P43 world_command 基础层重新恢复独立性。
P44 runtime 通过独立 adapter/handler target 接入 Command 管线。
后续 P45/P47/P48 不应再把 runtime 依赖塞回 world_command target。
```

### 4.2 P0-2：Pipeline 响应没有真实 ProjectionPatch

复审结论：已修复。

当前 `WorldCommandExecutionResult` 已新增：

```cpp
std::optional<WorldProjectionPatchDto> projection_patch_override;
std::vector<std::string> changed_cell_ids;
std::vector<std::string> changed_entity_ids;
bool requires_full_refresh = false;
```

当前 `WorldCommandPipeline::execute()` 在 BuildProjectionPatch 阶段会处理：

```text
如果 execution.projection_patch_override 有值，写入 response.projection_patch，并合并 projection version。
否则继续使用默认 ProjectionPatchBuilder。
```

当前 `WorldCommandRuntimeBridge` 已在：

```text
handleGenerateWorld
handleMove
```

中生成 `projection_patch_override`。

新增测试：

```text
world_command_pipeline_projection_patch_runtime
```

验证内容：

```text
通过 WorldCommandPipeline 执行 GenerateWorld。
response.projection_patch.requires_full_refresh == true。
response.projection_patch.changed_cells 非空。
response.projection_patch.changed_entities 非空。
changed_cells 包含 x / y / layer_key。
通过 WorldCommandPipeline 执行 Move。
Move response.projection_patch.changed_cells 非空。
Move response.projection_patch.changed_entities 非空。
changed_entities 包含 player entity。
player entity patch 包含 x / y / layer_key。
```

结论：

```text
P44 现在完成了真正闭环：前端提交 Command -> Pipeline 执行 Runtime -> Response 直接返回地图 ProjectionPatch。
```

### 4.3 P1-1：实体 ID 非确定性

复审结论：已修复。

当前已移除 `static counter` 方案，改为稳定 ID：

```cpp
makeStableEntityId(entity_key, coord, local_index)
```

当前实体 ID 由：

```text
entity_key
layer_key
x
y
local_index
```

组成。

新增测试：

```text
world_runtime_deterministic_entity_ids
```

验证内容：

```text
相同 seed 连续生成两个 runtime。
entities size 一致。
每个 entity_id 在另一个 runtime 中都能找到。
entity_key 一致。
coord 一致。
player entity_id 一致，且为 player_surface_0_0。
```

结论：

```text
同 seed 世界生成现在具备可复盘基础。
```

### 4.4 P1-2：origin cell 读取 moved-from player.entity_id

复审结论：已修复。

当前代码使用：

```cpp
origin_it->second.entity_ids.push_back(player_entity_id);
```

不再读取 moved-from `player.entity_id`。

新增测试：

```text
world_runtime_origin_contains_player_entity
```

验证内容：

```text
生成世界后找到 player actor。
读取 player actor 的 entity_id。
读取 origin cell surface:0:0。
origin cell.entity_ids 必须包含 player entity_id。
```

结论：

```text
玩家初始实体已正确挂到出生格。
```

## 5. 其他修复与观察

### 5.1 ProjectionAdapter 调试残留已清理

上次审核指出的 dummy findCell / snapshotForDebug 残留循环已删除。

当前 `WorldProjectionAdapter::buildCellPatches` 直接：

```text
按 cell_id 解析 layer/x/y。
校验 viewer 可见性。
读取 cell。
输出 x / y / layer_key / terrain_key / visibility / movement 等字段。
```

### 5.2 参数解析已避免异常崩溃

当前新增 `safeParse`，用于：

```text
seed
region_size
vision_radius
tick_delta
```

测试：

```text
world_command_runtime_invalid_parameters
```

当前策略是：

```text
非法数字参数不崩溃，使用默认值继续执行。
```

说明：

这已经修复“前端传错参数导致崩溃”的风险。

后续如果需要更严格的数据治理，可以在 P45/P46 之后改成：

```text
非法参数返回 Failed / invalid_parameter。
```

但这不是 P44 验收阻塞。

### 5.3 player entity_id 生成顺序可读性建议

当前代码：

```cpp
player.entity_id = makeStableEntityId("player", player.coord);
player.coord = WorldCellCoord{0, 0, "surface"};
```

由于 `WorldCellCoord` 默认值正好是 `0,0,surface`，结果正确，并且测试覆盖通过。

建议后续顺手改成：

```cpp
player.coord = WorldCellCoord{0, 0, "surface"};
player.entity_id = makeStableEntityId("player", player.coord);
```

这只是可读性改进，不影响当前验收。

## 6. 架构符合度复审

| 设计要求 | 当前状态 | 结论 |
|---|---|---|
| world_runtime 模块 | 已实现 | 通过 |
| x / y / layer_key 坐标 | 已实现 | 通过 |
| Actor / Entity / Cell 坐标 | 已实现 | 通过 |
| world_tick 非回合制 | 已实现 | 通过 |
| MoveCommand 修改坐标 | 已实现 | 通过 |
| GenerateWorld 生成世界 | 已实现 | 通过 |
| InspectCommand 可观察 | 已实现 | 通过 |
| Wait 推进世界时间 | 已实现 | 通过 |
| Pipeline 返回真实 ProjectionPatch | 已修复 | 通过 |
| world_command 不反向依赖 runtime | 已修复 | 通过 |
| 同 seed 实体 ID 稳定 | 已修复 | 通过 |
| 初始 player entity 在 origin cell | 已修复 | 通过 |
| 无红果/狼/火把/斧头核心硬编码 | 当前未发现 | 通过 |

## 7. 后续注意事项

P44 通过后，后续阶段必须继续遵守：

```text
不要让 pathfinder_world_command 重新依赖 world_runtime。
不要让前端绕过 WorldCommandPipeline 直接读 WorldGridRuntime。
不要让 H5 自己推导地图规则。
不要把真实玩法内容长期写死在 WorldGridRuntime。
不要把背包、容器、装备里的物品强行放到地图坐标上。
```

P45 建议重点：

```text
Inventory / Container / Equipped / OnMap / Nowhere 的统一归属模型。
地图实体和背包实体的迁移。
Pickup / Drop Command。
ProjectionPatch.changed_inventories。
避免“同伴火把从哪里来”“营地物品是否共享”再次混乱。
```

## 8. 最终意见

P44 本次修复解决了上次两个架构级 P0 问题，也补上了确定性实体 ID 和玩家出生格挂载问题。

当前 P44 已经形成后端开放世界最小闭环：

```text
GenerateWorld 生成世界。
玩家有坐标。
实体有坐标。
Move 通过 Command 管线改变坐标。
Inspect 可以读取可见目标。
Wait 推进世界时间步。
Pipeline response 返回真实地图 ProjectionPatch。
```

最终复审结论：

```text
P44 复审通过。
可以验收。
可以进入 P45。
```
