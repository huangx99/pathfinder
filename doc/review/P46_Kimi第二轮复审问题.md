# P46 Kimi 第二轮复审问题

## 1. 文档用途

这是 P46 第一轮 Kimi 修复后的第二轮复审问题单。

Kimi 必须继续使用同一个 P46 阶段上下文，只修 P46，不提交，不推送。

必须先阅读：

```text
doc/76_P46世界生成器与资源分布任务卡设计.md
doc/76B_P46世界生成器测试文档.md
doc/76C_P46世界生成器修复文档.md
doc/review/P46_Kimi继续修复任务单.md
doc/review/P46_Kimi第二轮复审问题.md
```

## 2. 当前已通过的测试

我独立运行过：

```bash
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_generation pathfinder_tests_world_generation_integration -j 2
ctest --test-dir build/backend -R '^world_generation_' --output-on-failure
```

结果：35/35 通过。

但是测试通过不代表可以验收。复审发现仍有架构问题和空跑测试。

## 3. P0：地面物品生成测试是空跑

当前 `SpawnSafetyPlanner` 移除了硬编码 `strange_tracks` 后，P46 默认 first_world 很可能不再生成任何 `GeneratedEntityDraft`。

这导致这些测试可能空跑：

```text
world_generation_ground_items_are_on_map_only
world_generation_ground_items_have_quantity_stack_key
world_generation_ground_items_on_map_after_apply
```

它们只遍历 `entity_drafts`，但没有断言 `entity_drafts` 非空。

这会破坏 P46 对 P45 的关键验收：

```text
P46/P47 生成或采集物品时，必须通过 P45 的归属接口建立 OnMap 或 InInventory 状态，不能绕过 runtime 直接改数组。
```

如果 P46 根本没有生成地面物品，P45 OnMap 归属路径没有被真实测试。

### 修复要求

必须新增配置化的地面物品生成能力，不能回到写死 `strange_tracks`。

推荐方案：

```cpp
struct GroundItemPlacementRule {
    std::string entity_key;
    std::string display_name_key;
    std::vector<std::string> allowed_terrain_tags;
    double density = 0.0;
    int min_distance_from_spawn = 0;
    int max_distance_from_spawn = -1;
    int quantity = 1;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> tags;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
};
```

`WorldgenProfile` 增加：

```cpp
std::vector<GroundItemPlacementRule> ground_item_rules;
```

生成逻辑：

```text
1. 由 profile.ground_item_rules 生成 GeneratedEntityDraft。
2. stable entity_id 必须使用 entity_key + coord + local_index。
3. 生成出的 ground item 必须通过 WorldGenerationApplier::applyEntities 调用 P45 location port spawnEntityOnMap 建立 OnMap。
4. built-in first_world 可以配置一个 hint 类地面物品，例如 strange_tracks，但只能在 profile 配置里写，不能在 SpawnSafetyPlanner 逻辑里写死。
5. SpawnSafetyPlanner 若需要 hint，只能从 ground_item_rules 或 resource_rules 里按 tag 查找候选。
```

### 必须补强测试

```text
world_generation_ground_items_non_empty_for_first_world
world_generation_ground_items_are_on_map_only
world_generation_ground_items_have_quantity_stack_key
world_generation_ground_item_attached_to_runtime_cell
```

测试必须断言：

```text
entity_drafts 非空。
apply 后 runtime.findEntity(entity_id).is_ok()。
entity.location_kind == OnMap。
entity.coord.has_value()。
对应 cell.entity_ids 包含 entity_id。
findInventory(player) 不包含这些 entity。
```

## 4. P0：ResourceNode 可以成为幽灵节点

当前 `WorldGenerationApplier::applyResourceNodes` 直接 `upsertGeneratedResourceNode`，没有检查 node.coord 对应 cell 是否存在。

如果传入错误 draft，resource node 会落到不存在的 cell，后续 P47 采集会找不到地图上下文。

### 修复要求

```text
1. applyResourceNodes 在 upsert 前必须 world_runtime_.findCell(draft.coord)。
2. 如果 cell 不存在，返回 ApplyFailed。
3. WorldGridRuntime::upsertGeneratedResourceNode 也建议防御性检查 cell 存在；如果做不到，至少 applier 必须检查。
```

### 必须补测试

```text
world_generation_resource_node_missing_cell_fails
world_generation_resource_nodes_applied_to_existing_cells
```

测试必须断言：

```text
资源节点 apply 后 runtime.findResourceNode 成功。
runtime.findCell(node.coord) 成功。
节点不出现在 runtime.entities。
```

## 5. P1：GenerateWorld region_x / region_y 测试不够强

当前 `world_generation_command_handler_region_xy` 只断言成功和 changed_cells 非空，没有断言生成坐标真的落在 region_x/region_y 对应区域。

### 修复要求

补强测试：

```text
world_generation_command_handler_region_xy
```

断言：

```text
region_x=1, region_y=-1, region_size=16 时，changed_cells 或 runtime snapshot 中至少一个 cell x 在 [8,23] 或按当前 region 算法范围，y 在 [-24,-9]。
不能仍然生成 origin 附近 [-8,7]。
```

如果你修改了 region 坐标算法，请在测试里按算法明确断言范围。

## 6. P1：enabled_layer_keys 不能误导 manifest

当前 handler 能解析 `layer_keys=surface,underground`，但生成器仍只按 profile.default_layer 生成 surface。

这可以暂时接受，但不能让 manifest 看起来像已经生成多个 layer。

### 两种可接受修复方式，任选一种

方案 A：P46 最小版只支持 default_layer。

```text
如果 enabled_layer_keys 多于一个或不等于 profile.default_layer，generate 返回 InvalidLayer。
测试断言 multi layer 当前失败。
```

方案 B：真正按 enabled_layer_keys 生成多层。

```text
TerrainGenerator 对每个 enabled layer 都生成 cells。
ResourceNode/GroundItem 至少先只放 default_layer，或者按配置 layer_filter。
测试断言 underground cells 确实存在。
```

建议本阶段采用方案 A，避免半实现。

## 7. 本轮完成标准

Kimi 完成后必须：

```text
1. 编译通过。
2. world_generation 测试全部通过。
3. 地面物品相关测试不再空跑。
4. ResourceNode 不能落到不存在 cell。
5. region_x/region_y 测试能证明坐标真的偏移。
6. layer_keys 行为不再误导 manifest。
7. 不提交、不推送。
```

建议运行：

```bash
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_generation pathfinder_tests_world_generation_integration -j 2
ctest --test-dir build/backend -R '^world_generation_' --output-on-failure
```
