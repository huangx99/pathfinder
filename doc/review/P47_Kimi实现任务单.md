# P47 Kimi 实现任务单

## 1. 任务

请 Kimi 实现 P47：资源采集、砍伐、挖掘。

只实现 P47，不提交，不推送。

## 2. 必须阅读

```text
doc/77_P47资源采集砍伐挖掘任务卡设计.md
doc/77A_P47资源采集砍伐挖掘风险头脑风暴.md
doc/77B_P47资源采集砍伐挖掘测试文档.md
doc/76_P46世界生成器与资源分布任务卡设计.md
doc/75_P45背包容器与物品归属系统设计.md
```

## 3. 实现范围

新增建议模块：

```text
backend/include/pathfinder/world_harvest/
backend/src/world_harvest/
backend/tests/unit/world_harvest/
backend/tests/integration/world_harvest/
```

实现：

```text
ResourceHarvestKind / FailureKind / OutputLocationPolicy。
ResourceHarvestRequest / Draft / Result / OutputDraft。
ResourceHarvestService validate + apply。
Gather/Chop/Mine/Dig Command handler。
Projection/events/state_deltas。
CMake target 和测试注册。
```

## 4. 硬性门禁

```text
1. 产物必须通过 P45 IWorldEntityLocationPort::spawnEntityOnMap 生成 OnMap。
2. P47 不允许直接写 inventory entries。
3. P47 不允许直接把产物塞进背包。
4. ResourceNode 不能变成普通 GroundItem。
5. 失败不扣 charge，不生成产物。
6. output id 必须确定性。
7. 不使用真实时间或随机数。
8. 前端不能读 JSON 或自己推导结果。
```

## 5. 本轮完成标准

```text
1. 编译通过。
2. P47 专项测试通过。
3. P46 -> P47 端到端测试通过。
4. P45 归属测试通过。
5. 不提交、不推送。
6. 最终回复改动文件、测试名、建议运行命令。
```

建议运行：

```bash
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_harvest pathfinder_tests_world_harvest_integration -j 2
ctest --test-dir build/backend -R '^(world_harvest_|world_command_(gather|chop|mine|dig|harvest)|world_generation_)' --output-on-failure
```
