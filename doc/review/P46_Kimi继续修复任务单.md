# P46 Kimi 继续修复任务单

## 1. 文档用途

本文档是给 Kimi 的 P46 代码修复任务单。

后续本项目与 Kimi 协作时，必须优先使用文档传递任务，而不是只靠长提示口头说明。

Kimi 必须阅读：

```text
doc/76_P46世界生成器与资源分布任务卡设计.md
doc/76A_P46世界生成器后续扩展风险头脑风暴.md
doc/76B_P46世界生成器测试文档.md
doc/76C_P46世界生成器修复文档.md
doc/review/P46_Kimi继续修复任务单.md
```

## 2. 当前状态

Kimi 已经部分修复了 P46：

```text
1. TerrainGenerator 已开始使用 region_coord + region_size 计算世界坐标。
2. WorldRuntime 新增了 generated cell、resource node、region generated tracking 的接口雏形。
3. spawnEntityOnMap 开始拒绝缺失 cell，避免幽灵地面物品。
4. generation_timestamp 已改成确定性 0。
5. WorldGenerationService 已去掉 generate() 末尾 markRegionGenerated。
```

但是当前代码还没有通过编译，也没有完成全部 P46 审核门禁。

## 3. 当前编译失败

我运行：

```bash
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_generation pathfinder_tests_world_generation_integration -j 2
```

失败：

```text
backend/include/pathfinder/world_runtime/world_grid_runtime.h:77:10: error: ‘set’ in namespace ‘std’ does not name a template type
提示：需要 #include <set>

backend/src/world_runtime/world_grid_runtime.cpp:612:34: error: ‘runtime_error’ is not a member of ‘pathfinder::foundation::ErrorCode’
```

修复要求：

```text
1. world_grid_runtime.h 加入正确 include。
2. ErrorCode 使用项目已有枚举值，不要自造 runtime_error。
3. 修复后必须能编译 pathfinder_tests_world_generation 和 pathfinder_tests_world_generation_integration。
```

## 4. 必须继续修复的 P0 问题

### 4.1 P0：applier 必须真正创建 cell

现状风险：之前 `WorldGenerationApplier::applyCells` 只写 state_delta，不写 runtime。

必须确认并完成：

```text
applyCells 必须调用 IWorldRuntime::createOrUpdateGeneratedCell。
apply 后 world_runtime.findCell(generated_coord).is_ok() 必须成立。
cell.terrain_key、cell.region_id、cell.coord.layer_key 必须与 draft 一致。
```

必须补测试：

```text
world_generation_apply_creates_runtime_cells
```

### 4.2 P0：地面物品不能成为幽灵实体

必须确认并完成：

```text
P46 applier 必须先 apply cells，再 apply entities。
spawnEntityOnMap 如果 cell 缺失必须失败。
apply 后 cell.entity_ids 必须包含 generated entity id。
```

必须补测试：

```text
world_generation_ground_item_attached_to_runtime_cell
world_generation_spawn_entity_missing_cell_fails
```

### 4.3 P0：ResourceNode 必须落地到 runtime 权威存储

必须确认并完成：

```text
WorldRuntime 层必须有 WorldResourceNodeRuntime。
IWorldRuntime 必须提供 upsertGeneratedResourceNode / findResourceNode 或等价接口。
WorldGenerationApplier 必须把 GeneratedResourceNodeDraft 落地到 runtime。
ResourceNode 不能出现在 WorldEntityInstance 普通实体列表里。
```

必须补测试：

```text
world_generation_resource_nodes_applied_to_runtime
world_generation_resource_node_not_ground_entity
```

### 4.4 P0：重复 apply 同一区域不能覆盖世界

generate() 必须是纯生成，不能标记 region generated。

重复控制必须在 runtime/applier 层处理：

```text
第一次 apply region 成功，并 markRegionGenerated。
第二次 apply 同 region 必须返回 RegionAlreadyGenerated，或严格幂等无变更。
不能重复生成实体。
不能覆盖玩家改动。
```

建议本阶段选择：第二次返回 RegionAlreadyGenerated。

必须补测试：

```text
world_generation_service_same_request_repeat_ok
world_generation_apply_same_region_twice_blocked
```

### 4.5 P0：manifest 必须可复盘

必须确认：

```text
generation_timestamp 不使用 std::time(nullptr)。
同一个 service 或两个 service，同 request 生成结果的 manifest 关键字段一致。
必须比较 generated_cell_ids、generated_entity_ids、generated_resource_node_ids、trace_roll_keys、generation_timestamp。
```

必须补强测试：

```text
world_generation_same_seed_manifest_full_stable
```

## 5. 必须继续修复的 P1 问题

### 5.1 SpawnSafetyPlanner 不能写死具体红果/木头/石头

当前仍可能写死：

```text
berry_bush
red_berry
fallen_branch
loose_stone
strange_tracks
```

修复方向：

```text
built-in profile 可以配置这些 key。
SpawnSafetyPlanner 逻辑不能固定这些 key。
出生保障必须从 profile.resource_rules 和 profile.spawn_safety.guaranteed_resource_keys / tags 中选择候选。
如果没有候选，返回可诊断失败或不补，不允许硬编码补 berry_bush。
```

测试要求：

```text
world_generation_spawn_safety_uses_profile_rules_not_fixed_keys
```

### 5.2 GenerateWorld command handler 参数必须安全

当前风险：

```text
std::stoull / std::stoi 未捕获异常。
没有解析 region_x / region_y。
layer_keys 只当一个整体字符串。
```

修复要求：

```text
非法 world_seed / region_size / region_x / region_y 返回 Failed，不崩溃。
支持 region_x、region_y。
支持 layer_keys 单值或逗号分隔。
```

测试要求：

```text
world_command_generate_world_invalid_numeric_parameter_fails
world_command_generate_world_region_coord_parameters
world_command_generate_world_layer_keys_csv
```

### 5.3 command_uses_service 必须走真实 command handler

当前 `world_generation_command_uses_service` 测试如果只是直接调用 service，不算有效。

必须补真实集成测试：

```text
创建 WorldGridRuntime。
创建 WorldGenerationService。
创建 GenerateWorldCommandHandler。
执行 WorldCommandDto kind=GenerateWorld。
断言 result succeeded。
断言 changed_cells / changed_entities / state_deltas / events 存在。
断言 runtime.findCell 成功。
断言 runtime.findResourceNode 成功。
断言 ground item 在 OnMap，且 cell.entity_ids 包含它。
```

建议测试名：

```text
world_command_generate_world_real_handler_applies_runtime
```

## 6. 架构硬门禁

必须继续遵守：

```text
P46/P47 生成或采集物品时，必须通过 P45 的归属接口建立 OnMap 或 InInventory 状态，不能绕过 runtime 直接改数组。
P46 不允许创建玩家背包物品。
ResourceNode 不是普通 GroundItem。
前端不能读 worldgen JSON。
WorldCommand 不能直接依赖 world_generation。
```

注意：

```text
world_generation 可以依赖 world_runtime/world_inventory。
world_command 基础模块不能反向依赖 world_generation。
GenerateWorld 的 P46 runtime-aware handler 应留在 world_generation 模块或独立适配模块。
```

## 7. 本轮完成标准

Kimi 本轮必须做到：

```text
1. 编译通过。
2. P46 所有 world_generation 测试通过。
3. 新增测试覆盖本文档列出的关键场景。
4. 不提交、不推送。
5. 最终回复列出改动文件、测试名、建议运行命令。
```

建议运行命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_generation pathfinder_tests_world_generation_integration -j 2
ctest --test-dir build/backend -R '^world_generation_' --output-on-failure
```
