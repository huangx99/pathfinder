# P43 统一世界 Command 管线前置复审报告

复审日期：2026-05-20

## 1. 复审结论

结论：P43 本次复审通过，可以验收进入下一阶段。

上次审核提出的问题中：

```text
P0-1：CommandTraceRecorder 跨命令污染。
P1-1：HandlerRegistry 缺少 action_key 绑定接口。
P1-2：无内容特例测试保护力不足。
```

本次复审结果：

```text
P0-1 已修复。
P1-1 已修复。
P1-2 已增强，当前可接受；后续仍建议在更完整 CI 中加入源码级架构扫描。
```

P43 当前已经达到本阶段目标：

```text
建立 V2 世界行为的统一 Command 入口。
建立 Pipeline + Dispatcher + HandlerRegistry 的注册式分发骨架。
保持 world_command 与旧 H5、旧 DialogScenario、具体玩法内容解耦。
为后续世界网格、背包、采集、制作、NPC、自主行动、区域事件、前端投影打基础。
```

最终复审意见：

```text
通过。
可以进入 P44 / V2 世界网格相关阶段。
```

## 2. 复审范围

代码范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/world_command/
backend/src/world_command/
backend/tests/unit/world_command/
backend/tests/integration/world_command/
```

设计依据：

```text
doc/73_P43统一世界Command管线前置设计.md
doc/72_V2_0开放世界网格生存与文明启蒙整体架构设计.md
doc/程序设计计划.md
```

上次审核依据：

```text
doc/review/P43统一世界Command管线前置审核报告.md
```

## 3. 已执行验证

执行命令：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_command pathfinder_tests_world_command_integration -j 2
ctest --test-dir build/backend -R '^world_command_' --output-on-failure
```

结果：

```text
14/14 tests passed
0 failed
```

通过测试：

```text
world_command_enum
world_command_dto_validation
world_command_pipeline
world_command_registry
world_command_trace
world_command_trace_isolation
world_command_source_distinguish
world_command_dispatcher_lookup
world_command_dispatcher_no_playbook_rules
world_command_flow_bootstrap
world_command_flow_sequence
world_command_flow_response_completeness
world_command_flow_invalid_command
world_command_flow_trace_integrity
```

## 4. 上次问题复审

### 4.1 P0-1：CommandTraceRecorder 跨命令污染

复审结论：已修复。

位置：

```text
backend/src/world_command/world_command_pipeline.cpp
```

当前 `WorldCommandPipeline::execute()` 开始处已经调用：

```cpp
trace_recorder_.clear();
```

这保证每次 Command 的 debug trace 只属于当前命令，不再把上一次命令的 Receive / Stage / Result 混入本次响应。

新增验证：

```text
backend/tests/unit/world_command/world_command_test.cpp
run_trace_isolation_tests
```

验证内容：

```text
连续执行 cmd_first / wait 与 cmd_second / noop。
第二次 response.debug_trace_keys 不包含 wait。
第二次 trace 中 receive 记录数量等于 1。
第二次 trace 中包含 noop。
```

这能直接防止上次发现的 trace 累积问题回归。

### 4.2 P1-1：HandlerRegistry 缺少 action_key 绑定接口

复审结论：已修复。

位置：

```text
backend/include/pathfinder/world_command/world_command_registry.h
backend/src/world_command/world_command_registry.cpp
```

当前已新增：

```cpp
pathfinder::foundation::Result<void> bindActionKey(
    const std::string& action_key,
    WorldCommandKind handler_kind);
```

当前行为：

```text
action_key 不能为空。
不能绑定到 Unknown。
不能绑定到未注册的 handler kind。
不能重复绑定同一个 action_key。
绑定成功后 findByActionKey 能返回对应 handler。
```

测试覆盖：

```text
backend/tests/unit/world_command/world_command_test.cpp
run_registry_tests
```

这对后续 JSON content action 接入很重要。后续可以把：

```text
content action_key -> command handler kind
```

注册到同一套 Registry，而不是在 Dispatcher 中写大量 if/else。

### 4.3 P1-2：无内容特例测试保护力不足

复审结论：已增强，当前 P43 可接受。

位置：

```text
backend/tests/unit/world_command/world_command_dispatcher_test.cpp
```

当前 `run_dispatcher_no_playbook_rules_tests` 已不再只验证 Wait 成功，而是覆盖：

```text
Noop
Wait
Inspect
SystemTick
GenerateWorld
```

并检查 handler 输出中没有出现以下内容特例 key：

```text
red_berry
wolf
torch
axe
poison_mushroom
wood
tree
fire
camp_fire
```

人工源码扫描结果：

```text
backend/include/pathfinder/world_command
backend/src/world_command
```

未发现上述具体玩法内容 key。

说明：

当前测试已经比上次强，能防止 handler 输出偷偷混入具体内容规则。但它还不是完整源码级架构测试，因为 forbidden key 只在测试运行结果中检查，不会自动扫描所有源码文本。

本阶段可以接受，原因：

```text
P43 仍是 Command 管线前置骨架阶段。
当前人工扫描确认核心源码无红果、狼、火把、斧头等内容特例。
核心分发仍走 Registry + Handler，没有退回 if/else 玩法分发。
```

后续建议：

```text
在 CI 或架构测试中加入 world_command 核心源码 forbidden content key 扫描。
这不是 P43 阻塞项，可在 P44/P45 或测试基础设施阶段补。
```

## 5. 架构符合度复审

### 5.1 统一入口

当前具备：

```text
WorldCommandPipeline::execute(session_id, command)
```

后续玩家输入、NPC 决策、系统 Tick、区域事件、世界生成、复盘测试都可以统一提交 Command。

结论：通过。

### 5.2 注册式分发

当前具备：

```text
IWorldCommandHandler
WorldCommandHandlerRegistry
WorldCommandDispatcher
bindActionKey
findByKind
findByActionKey
```

Dispatcher 通过 Registry 找 Handler，不直接写具体玩法判断。

结论：通过。

### 5.3 前端响应结构

当前 `WorldCommandResponseDto` 保留：

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

这符合后续 V2 H5 只消费后端投影和可用命令的设计方向。

结论：通过。

### 5.4 与旧 H5 解耦

复审未发现 `world_command` 依赖旧 H5 dialog、旧页面逻辑或具体 H5 可玩原型。

结论：通过。

### 5.5 与具体玩法内容解耦

复审未发现 `world_command` 核心源码写入红果、狼、火把、斧头、木头等内容特例。

结论：通过。

## 6. 当前仍是 Stub 的部分

以下不是 P43 问题，而是后续阶段必须逐步接入：

```text
ValidateLayer：等待 V2 世界网格 / layer_key 接入。
ValidateRange：等待位置、距离、视野接入。
ValidateKnowledgeGate：等待知识门槛与 Agent 认知接入。
ValidateConditions：等待表达式 / 条件系统接入。
ValidateMaterials：等待背包、库存、材料系统接入。
ProjectionPatch：当前只有版本号，后续要加入 changed_cells / changed_entities / inventories。
AvailableCommandBuilder：当前可为空，后续要由世界状态和知识状态生成按钮。
GenerateWorldHandler：当前是 stub，后续接世界生成器。
EffectOperationRegistry：尚未接入，后续接真实世界效果。
```

这些都符合 P43 的阶段边界：P43 只建立管线，不一次实现完整开放世界。

## 7. 风险与后续要求

### 7.1 必须继续遵守的架构约束

后续阶段必须遵守：

```text
不能让前端直接调用 movePlayer / addItem / craftItem 等内部函数。
不能在 Dispatcher 里写大量 action_key if/else。
不能把 red_berry / wolf / torch / axe 等内容写死进 world_command 核心。
不能让 NPC 自主行动绕过 WorldCommandPipeline。
不能让世界生成、区域事件、区域魔法绕过 WorldCommandPipeline。
```

### 7.2 P44 以后建议优先接入

建议顺序：

```text
P44：世界网格最小 Runtime 与 ProjectionPatch。
P45：实体位置、Actor、ObjectInstance、Inventory Patch。
P46：Command 可用性生成 available_commands。
P47：采集 / 移动 / 观察等真实 Handler。
P48：制作 / 反应 / EffectOperationRegistry。
P49：ExperienceEmitter 与知识系统接入。
```

这样能保持“前端只发 Command，后端统一返回变化”的 V2 方向。

## 8. 最终复审意见

P43 本次修复解决了上次最重要的 trace 污染问题，也补上了 action_key 注册式映射能力。

当前代码没有发现为了通过 P43 而写死具体玩法内容的情况，仍然是面向后续开放世界扩展的底层 Command 管线。

最终结论：

```text
P43 复审通过。
可以验收。
可以进入下一阶段。
```
