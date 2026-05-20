# P43 统一世界 Command 管线前置审核报告

审核日期：2026-05-20

## 1. 审核结论

结论：方向正确，但不建议直接验收通过，需要先修复 1 个核心阻塞问题和 2 个结构性问题。

当前 P43 实现已经完成了设计文档要求的大部分最小骨架：

```text
pathfinder_world_command target。
WorldCommandKind / Source / ResultKind / TargetKind / PatchOp / FrontendHintKind / AreaShapeKind。
WorldCommandDto / WorldCommandResponseDto / ProjectionPatch / Event / Experience / Option DTO。
WorldCommandPipeline::execute。
WorldCommandDispatcher。
WorldCommandHandlerRegistry。
IWorldCommandHandler。
Noop / Wait / Inspect / SystemTick / GenerateWorld stub handlers。
ProjectionPatchBuilder / AvailableCommandBuilder / FrontendHintBuilder。
CommandTraceRecorder。
P43 单元测试和集成测试。
```

专项测试全部通过，说明代码能构建、能执行最小命令、能通过注册式 Handler 分发。

但是 P43 的目标不是“测试跑过就行”，而是建立后续 V2 世界的统一命令入口。当前存在一个会影响复盘和调试正确性的核心问题：`CommandTraceRecorder` 是 Pipeline 成员，但每次 `execute()` 开始时没有清空，导致多个命令的 trace 会互相污染。

因此本次审核结论为：

```text
暂不验收。
修复 P0-1 后可复审。
```

## 2. 已检查范围

代码范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/world_command/world_command_types.h
backend/include/pathfinder/world_command/world_command_handler.h
backend/include/pathfinder/world_command/world_command_handlers.h
backend/include/pathfinder/world_command/world_command_registry.h
backend/include/pathfinder/world_command/world_command_dispatcher.h
backend/include/pathfinder/world_command/world_command_pipeline.h
backend/include/pathfinder/world_command/world_command_projection.h
backend/include/pathfinder/world_command/world_command_trace.h
backend/src/world_command/world_command_types.cpp
backend/src/world_command/world_command_registry.cpp
backend/src/world_command/world_command_dispatcher.cpp
backend/src/world_command/world_command_pipeline.cpp
backend/src/world_command/world_command_projection.cpp
backend/src/world_command/world_command_trace.cpp
backend/src/world_command/handlers/noop_command_handler.cpp
backend/src/world_command/handlers/wait_command_handler.cpp
backend/src/world_command/handlers/inspect_command_handler.cpp
backend/src/world_command/handlers/system_tick_command_handler.cpp
backend/src/world_command/handlers/generate_world_command_handler.cpp
backend/tests/unit/world_command/world_command_test.cpp
backend/tests/unit/world_command/world_command_dispatcher_test.cpp
backend/tests/integration/world_command/world_command_flow_test.cpp
```

对照文档：

```text
doc/73_P43统一世界Command管线前置设计.md
```

## 3. 已执行验证

### 3.1 构建验证

执行：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_command pathfinder_tests_world_command_integration -j 2
```

结果：通过。

### 3.2 P43 单元与集成测试

执行：

```text
./build/backend/tests/pathfinder_tests_world_command enum
./build/backend/tests/pathfinder_tests_world_command dto_validation
./build/backend/tests/pathfinder_tests_world_command pipeline
./build/backend/tests/pathfinder_tests_world_command registry
./build/backend/tests/pathfinder_tests_world_command trace
./build/backend/tests/pathfinder_tests_world_command source_distinguish
./build/backend/tests/pathfinder_tests_world_command lookup
./build/backend/tests/pathfinder_tests_world_command no_playbook_rules
./build/backend/tests/pathfinder_tests_world_command_integration bootstrap
./build/backend/tests/pathfinder_tests_world_command_integration sequence
./build/backend/tests/pathfinder_tests_world_command_integration response_completeness
./build/backend/tests/pathfinder_tests_world_command_integration invalid_command
./build/backend/tests/pathfinder_tests_world_command_integration trace_integrity
ctest --test-dir build/backend -R '^world_command_' --output-on-failure
```

结果：全部通过。

CTest 结果：

```text
13/13 tests passed
0 failed
```

## 4. 符合设计的部分

### 4.1 模块边界基本正确

已新增独立 target：

```text
pathfinder_world_command
```

当前依赖：

```text
pathfinder_world_command -> pathfinder_foundation
```

这符合 P43 设计中“world_command 不反向依赖 H5、不依赖旧 h5_dialog、不使用旧 DialogScenario”的要求。

### 4.2 注册式 Handler 分发方向正确

当前已有：

```text
IWorldCommandHandler
WorldCommandHandlerRegistry
WorldCommandDispatcher
NoopCommandHandler
WaitCommandHandler
InspectCommandHandler
SystemTickCommandHandler
GenerateWorldCommandHandler
```

`WorldCommandDispatcher` 通过 `registry.findByKind(command.command_kind)` 查找 handler，不是在 dispatcher 里写大段玩法 if/else。

这符合“Command 不是无数 if，而是注册式 Handler”的设计方向。

### 4.3 最小 Handler 没有写内容特例

扫描范围：

```text
backend/include/pathfinder/world_command
backend/src/world_command
backend/tests/unit/world_command
backend/tests/integration/world_command
```

未发现红果、狼、火把、斧头、木头等玩法特例写入核心模块。

唯一命中是测试注释：

```text
backend/tests/unit/world_command/world_command_dispatcher_test.cpp
```

结论：当前没有明显把具体内容写死到 P43 核心。

### 4.4 Response 结构基本完整

`WorldCommandResponseDto` 已包含：

```text
result
projection_patch
event_feed
experiences
frontend_hints
available_commands
warning_keys
debug_trace_keys
```

这符合 P43 设计中“Command 必须返回前端可消费变化”的要求。

### 4.5 P43 最小行为已覆盖

当前测试覆盖：

```text
Noop -> Noop event。
Wait -> TimePassed event。
Inspect -> Inspect event。
SystemTick source 可进入管线。
GenerateWorld handler stub 可注册。
Unknown command 返回 Failed / unknown_command。
重复注册同一 handler kind 会失败。
投影版本随命令递增。
```

这符合 P43 最小实现范围。

## 5. 阻塞问题

## P0-1：CommandTraceRecorder 没有按命令清空，trace 会跨命令污染

位置：

```text
backend/src/world_command/world_command_pipeline.cpp
```

现状：

```text
WorldCommandPipeline 持有成员 trace_recorder_。
execute() 开始时直接 recordCommandReceived(command)。
execute() 结束时 response.debug_trace_keys = trace_recorder_.getTrace()。
但 execute() 开始没有 trace_recorder_.clear()。
```

相关代码行为：

```text
第一次执行 Wait：trace 包含第一次命令。
第二次执行 Wait：trace 会包含第一次命令 + 第二次命令。
第三次执行 Wait：trace 会继续累计。
```

风险：

```text
每个 Command 的 debug_trace 不再只描述自己。
复盘和调试会把上一个命令的阶段误认为当前命令的一部分。
WorldCommandContext 初始化时会把污染后的 trace 全部复制进去。
后续 Agent 调试、World Replay、失败定位都会被误导。
```

为什么这是 P0：

P43 的核心目标之一就是建立统一 Command trace，为后续复盘、NPC 行为调试和世界生成调试打基础。trace 污染会直接破坏这个目标。

修复建议：

```cpp
Result<WorldCommandResponseDto> WorldCommandPipeline::execute(...) {
    trace_recorder_.clear();
    ...
}
```

同时补测试：

```text
连续执行两个不同 command。
第二个 response.debug_trace_keys 不应包含第一个 command_id / command_key / result。
trace 的 receive 记录数量应为 1。
```

## 6. 结构性问题

## P1-1：HandlerRegistry 预留了 action_registry_，但没有注册 action_key 的接口

位置：

```text
backend/include/pathfinder/world_command/world_command_registry.h
backend/src/world_command/world_command_registry.cpp
```

现状：

```text
WorldCommandHandlerRegistry 有 action_registry_。
有 findByActionKey(action_key)。
但没有 registerActionHandler / bindActionKey / registerActionKeyMapping 之类接口。
```

风险：

P43 设计明确后续要通过 content action 接入 `WorldCommandOptionDto` 和 Handler：

```text
content action -> action_key -> handler
```

如果 registry 不能注册 action_key 映射，后续 P44-P48 很容易退回：

```text
if action_key == "gather" then GatherHandler
if action_key == "craft" then CraftHandler
```

这正是 P43 要避免的问题。

修复建议：

新增接口，例如：

```cpp
foundation::Result<void> bindActionKey(
    const std::string& action_key,
    WorldCommandKind handler_kind);
```

或：

```cpp
foundation::Result<void> registerActionHandler(
    const std::string& action_key,
    std::shared_ptr<IWorldCommandHandler> handler);
```

并补测试：

```text
action_key 能映射到已注册 handler。
重复 action_key 绑定失败。
未知 action_key 返回 nullptr 或 unknown_command。
```

## P1-2：无内容特例测试过弱，只验证 Wait 成功，没有真正防止内容规则混入

位置：

```text
backend/tests/unit/world_command/world_command_dispatcher_test.cpp
```

现状：

测试名为：

```text
run_dispatcher_no_playbook_rules_tests
```

但实际只执行 Wait handler 并断言成功。

这不能证明：

```text
Pipeline 不包含 red_berry / wolf / torch / axe 等内容特例。
Handler 不包含内容特例。
Dispatcher 不包含内容特例。
```

当前人工扫描未发现内容特例，所以这不是当前代码缺陷；但测试保护力不足。

建议：

短期可以增加源文件文本扫描测试或更明确的结构测试：

```text
world_command 核心源码中不得出现 red_berry、wolf、torch、axe、poison_mushroom 等内容 key。
```

长期应通过架构测试保证：

```text
Dispatcher 只能依赖 HandlerRegistry。
Handler 不读取 UI 层。
前端按钮只能来自 available_commands。
```

## 7. 可延期事项

以下事项不阻塞 P43 最小验收，但必须在后续阶段接入：

```text
EffectOperationRegistry / IEffectOperationApplier 尚未实现，可在 P47/P48 接入真实 effect 时补。
ContentActionAdapter 尚未实现，可在 content actions 接入 available_commands 时补。
AvailableCommandBuilder 当前返回空数组，P43 允许，P44/P47/P53 必须逐步实现。
ValidateLayer / ValidateRange / ValidateKnowledgeGate / ValidateConditions / ValidateMaterials 目前是 stub，P44-P49 必须逐步接入真实模块。
ProjectionPatch 当前只有版本号，P44 以后必须填充 changed_cells / changed_entities / inventories 等。
```

## 8. 设计符合度对照

| 设计要求 | 当前状态 | 结论 |
|---|---|---|
| 独立 world_command target | 已实现 | 通过 |
| 不依赖旧 H5 / DialogScenario | 当前未依赖 | 通过 |
| WorldCommand DTO / Response DTO | 已实现 | 通过 |
| Pipeline 统一入口 | 已实现 | 通过 |
| Dispatcher + HandlerRegistry | 已实现 | 基础通过 |
| Noop / Wait / Inspect / SystemTick / GenerateWorld stub | 已实现 | 通过 |
| available_commands 字段存在 | 已实现，当前为空 | P43 通过，后续补 |
| ProjectionPatch 字段存在 | 已实现，当前版本号 | P43 通过，后续补 |
| CommandTrace | 已实现但跨命令污染 | 不通过，P0 |
| action_key 注册式分发 | 有 find，无注册接口 | P1 |
| 不写玩法特例 | 人工扫描通过 | 通过，但测试弱 |
| 测试覆盖 | 13 项通过 | 基础通过 |

## 9. 建议修复清单

### 必须修复后再验收

```text
P0-1：WorldCommandPipeline::execute() 开始清空 trace_recorder_，并补连续命令 trace 不污染测试。
```

### 建议本阶段一起修复

```text
P1-1：给 WorldCommandHandlerRegistry 增加 action_key 绑定接口和测试。
P1-2：增强 no_playbook_rules 测试，不要只断言 Wait 成功。
```

### 后续阶段继续实现

```text
P44：ProjectionPatch changed_cells / changed_entities。
P45：Inventory / ObjectInstance 相关 patch。
P47：Gather / Chop / Mine / Dig handlers。
P48：Craft / Reaction / EffectOperationRegistry。
P49：ExperienceEmitter 与知识模板接入。
P53：available_commands 驱动 H5 V2。
```

## 10. 最终意见

当前 P43 实现的方向是对的：

```text
没有另起炉灶。
没有把旧 H5 接入 world_command。
没有写内容特例。
已经建立了统一 Pipeline + Dispatcher + HandlerRegistry 骨架。
```

但由于 trace 污染会直接破坏 P43 的复盘和调试基础，本阶段不建议直接验收。

最终审核结论：

```text
需修复后复审。
```
