# P48 Kimi 实现任务单

阶段：P48 制作与世界反应接入
状态：待实现

## 1. 必须阅读

请先阅读以下文档，再写代码：

```text
doc/78_P48制作与世界反应接入任务卡设计.md
doc/78A_P48制作与世界反应风险头脑风暴.md
doc/78B_P48制作与世界反应测试文档.md
doc/75_P45背包容器与物品归属系统设计.md
doc/77_P47资源采集砍伐挖掘任务卡设计.md
doc/review/P47资源采集砍伐挖掘系统最终审核报告.md
```

## 2. 实现范围

实现 P48：

```text
Craft command 执行指定 JSON reaction。
材料从 actor inventory / 附近 OnMap entity 中解析。
材料消耗走 P45 权威接口。
产物生成进入 actor inventory 或 OnMap。
返回 events / deltas / projection patch / experience。
```

## 3. 必须新增模块

建议新增：

```text
backend/include/pathfinder/world_reaction/
backend/src/world_reaction/
backend/tests/unit/world_reaction/
backend/tests/integration/world_reaction/
```

建议新增库：

```text
pathfinder_world_reaction
```

## 4. 必须补强 P45

当前 `InventoryTransferKind` 已有预留，但 runtime 未实现。

P48 至少要实现：

```text
ConsumeToNowhere
SpawnToInventory
```

要求：

```text
不能让 world_reaction 直接改 inventory entries。
不能让 world_reaction 直接改 item_locations_。
```

## 5. 硬性门禁

不允许：

```text
不写死 wood_plus_fire_make_torch / torch / wood / camp_fire 的主流程。
不绕过 P45 inventory / location port。
不使用真实时间或随机数。
不由前端传 outputs。
不直接形成知识。
不实现 P49/P50/P51。
不修改 H5。
不提交，不推送。
```

允许：

```text
测试中使用 wood_plus_fire_make_torch 作为样例。
测试中构造 processed wood。
为 P45 transfer 增加必要 helper。
```

## 6. 必须测试

至少实现并注册这些测试：

```text
world_inventory_consume_to_nowhere_reduces_stack
world_inventory_consume_to_nowhere_removes_stack_at_zero
world_inventory_consume_to_nowhere_insufficient_fails
world_inventory_spawn_to_inventory_adds_item
world_inventory_spawn_to_inventory_merges_stack
world_reaction_apply_craft_torch_to_inventory
world_reaction_validate_input_missing
world_reaction_validate_state_mismatch
world_reaction_validate_quantity_insufficient
world_reaction_validate_catalyst_not_consumed
world_reaction_failure_does_not_pollute_state
world_command_craft_reaction_key_from_target_recipe
world_command_craft_events_use_context_tick
world_generation_harvest_pickup_then_craft_torch
```

## 7. 必须运行

```bash
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_reaction pathfinder_tests_world_reaction_integration -j 2
ctest --test-dir build/backend -R '^(world_reaction_|world_command_craft_|world_inventory_(consume|spawn)|world_generation_|world_harvest_)' --output-on-failure
```

建议再运行：

```bash
cmake --build build/backend -j 2
ctest --test-dir build/backend -R '^(architecture_world_command_no_runtime_dependency|architecture_world_command_no_inventory_dependency|world_command_|world_runtime_|world_inventory_|world_generation_|world_harvest_|world_reaction_)' --output-on-failure
```

## 8. 完成汇报

完成后请汇报：

```text
改动文件。
新增测试名。
实际运行测试命令和结果。
是否有偏离文档。
是否有保留风险。
```

不要提交，不要推送。
