# P42 JSON 配置内容目录与加载管线最终复审报告

复审日期：2026-05-20

## 1. 复审结论

结论：P42 可以作为阶段通过，允许进入 V2.0 / P43 统一世界 Command 管线设计与实现。

本次复审不是简单看测试是否通过，而是按 P42 设计目标检查了：

```text
JSON 内容目录是否存在。
manifest 是否能加载。
ContentRegistry 是否能注册核心内容。
结构校验和引用校验是否能阻止错误配置。
Strict 模式下错误内容是否会阻断 registry 构建。
Scenario 初始对象数量、numeric 状态、欢迎文案、快捷动作、推荐动作、危险知识模板是否能进入 H5 场景。
EffectExecutionSpec / EffectSemantics 是否能从 JSON effect 定义生成。
P42 专项测试是否稳定。
H5 关键闭环是否未被破坏。
```

综合判断：

```text
P42 的核心目标已经达成。
P42 不再是“只有几个 JSON 文件的空壳”。
P42 已经具备作为后续 V2 内容配置基础的能力。
```

但仍有 3 个后续架构债务，必须在 V2/P43-P48 路线里继续收束，不能遗忘。

## 2. 已检查范围

代码范围：

```text
backend/CMakeLists.txt
backend/include/pathfinder/content/content_types.h
backend/include/pathfinder/content/content_registry.h
backend/include/pathfinder/content/content_runtime_adapter.h
backend/include/pathfinder/content/content_validation.h
backend/include/pathfinder/content/json_content_loader.h
backend/src/content/content_types.cpp
backend/src/content/content_registry.cpp
backend/src/content/content_runtime_adapter.cpp
backend/src/content/content_validation.cpp
backend/src/content/json_content_loader.cpp
backend/src/h5_dialog/dialog_scenario.cpp
backend/src/h5_dialog/dialog_types.cpp
backend/tests/CMakeLists.txt
backend/tests/unit/content/*
backend/tests/integration/p42/json_content_h5_flow_test.cpp
content/core/*
content/dev/manifest.json
backend/third_party/nlohmann/json.hpp
```

文档范围：

```text
doc/71_P42_JSON配置内容目录与加载管线任务卡设计.md
doc/review/P42_JSON配置内容目录与加载管线审核报告.md
doc/review/P42_JSON配置内容目录与加载管线复审报告.md
doc/review/P42_JSON配置内容目录与加载管线再次复审报告.md
doc/72_V2_0开放世界网格生存与文明启蒙整体架构设计.md
doc/73_P43统一世界Command管线前置设计.md
```

## 3. 已执行验证

### 3.1 构建验证

执行：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_content pathfinder_tests_content_integration pathfinder_tests_h5_playable -j 2
```

结果：通过。

说明：

```text
P42 内容模块、内容运行时适配测试、P42 集成测试、H5 可玩测试目标均可构建。
```

### 3.2 P42 单元测试

执行：

```text
./build/backend/tests/pathfinder_tests_content loader
./build/backend/tests/pathfinder_tests_content registry
./build/backend/tests/pathfinder_tests_content validation
./build/backend/tests/pathfinder_tests_content runtime_adapter
```

结果：通过。

覆盖点：

```text
路径安全。
manifest 解析。
objects / effects / feedbacks / reactions / threats / locales / agents / scenarios 解析。
非法 JSON 失败。
Strict 模式下错误配置跳过 registry 构建。
registry 查询。
引用校验。
Runtime Adapter 构建 scenario 和 effect 运行时结构。
```

### 3.3 P42 集成测试

执行：

```text
./build/backend/tests/pathfinder_tests_content_integration flow
```

结果：通过。

本项验证了之前的路径依赖问题已经修复：

```text
测试现在通过 PATHFINDER_CONTENT_ROOT 使用稳定内容目录。
从仓库根目录直接运行也能通过。
```

### 3.4 CTest 注册验证

执行：

```text
ctest --test-dir build/backend -R '^(content_loader|content_registry|content_validation|content_runtime_adapter|content_integration_flow)$' --output-on-failure
```

结果：通过。

说明：

```text
P42 五个测试项已经进入 CTest。
不能再用宽泛 content 正则判断，因为历史旧测试名中仍有 reaction_content 等未构建目标。
P42 自己的测试注册是可用的。
```

### 3.5 H5 关键回归

执行：

```text
./build/backend/tests/pathfinder_tests_h5_playable story
./build/backend/tests/pathfinder_tests_h5_playable buttons_executable
./build/backend/tests/pathfinder_tests_h5_playable companion_taught_torch_autonomy
./build/backend/tests/pathfinder_tests_h5_playable frontend_gate
```

结果：通过。

说明：

```text
P42 没有破坏当前旧 H5 的最小剧情、按钮可执行性、同伴火把自治和前端门禁。
```

## 4. 已确认修复的旧问题

### 4.1 本地 nlohmann/json 已接入

当前 `json.hpp` 已迁移到：

```text
backend/third_party/nlohmann/json.hpp
```

`backend/CMakeLists.txt` 已把该目录加入 `pathfinder_content` include 路径。

结论：

```text
P42 不再依赖 CMake 配置时联网下载 JSON 库。
```

### 4.2 Strict 模式错误阻断 registry 构建

当前 `JsonContentLoader::load` 在结构校验和引用校验后执行：

```text
StrictContentRequired 模式下，只要 validation_report 有 Fatal 或 Error，就跳过 ContentRegistry 构建。
```

对应测试：

```text
loader_test_12_strict_errors_skip_registry
```

结论：

```text
配置错误时不会构建可用 registry。
这是 P42 最重要的安全门禁之一，已满足。
```

### 4.3 Scenario 字段接入明显补齐

当前 `ScenarioDefinitionDto` 已包含：

```text
welcome_text_key
initial_objects.quantity
initial_objects.visible
initial_objects.numeric
initial_agents
initial_threats
quick_action_input_texts
suggested_action_templates
threat_knowledge_templates
default_player_knowledge
default_agent_knowledge
```

`ContentRuntimeAdapter::buildDialogScenario` 已能把关键字段转换到 `DialogScenario`。

`backend/src/h5_dialog/dialog_scenario.cpp` 里也有一套内联转换，用于旧 H5 直接从 content/core 构建场景。

结论：

```text
P42 已经不是只迁移 objects / feedbacks。
最小 H5 生存闭环的核心场景字段已经能从 JSON 进入运行时。
```

### 4.4 EffectExecutionSpec / EffectSemantics 已能从 JSON 生成

当前 `ContentRuntimeAdapter` 已提供：

```text
buildEffectSpecs()
buildEffectSemantics()
```

并能把 JSON operation 映射到安全 effect operation：

```text
consume_object_quantity
produce_object_quantity
change_actor_need
add_object_state_number
set_object_state_number
remove_threat
hint_object_use
instinct_fear_response
```

结论：

```text
P42 已经开始接入 P41 通用世界效果执行链，不再只是静态内容表。
```

### 4.5 P42 集成测试已覆盖 JSON 到 H5 场景

`backend/tests/integration/p42/json_content_h5_flow_test.cpp` 已检查：

```text
first_night 场景存在。
welcome_text_key 非空。
quick_action_input_texts 非空。
suggested_action_templates 非空。
threat_knowledge_templates 非空。
红果初始数量来自 JSON。
斧头 sharpness numeric 来自 JSON。
木头 processed numeric 来自 JSON。
火把初始数量为 0。
quick actions / suggested actions / threat templates 来自 JSON。
```

结论：

```text
这不是纯解析测试，已经覆盖“内容包 -> Registry -> Runtime Adapter -> H5 Scenario”的端到端链路。
```

## 5. 当前仍存在的风险

### P1-1：旧 H5 仍有一条直接读取 ContentRegistry 的内联转换路径

位置：

```text
backend/src/h5_dialog/dialog_scenario.cpp
```

现状：

```text
DialogScenarioCatalog::defaultScenario()
  -> JsonContentLoader::load()
  -> buildFromContent(*registry, "first_night")
```

`buildFromContent` 是 H5 内部的内联转换函数，它没有直接使用 `ContentRuntimeAdapter::buildDialogScenario()`。

这已经比上一轮的 CMake 互相依赖风险更好：

```text
当前没有 h5_dialog <-> content_runtime_adapter 的 CMake 循环。
h5_dialog 只链接 pathfinder_content，不链接 pathfinder_content_runtime_adapter。
```

但它仍然有架构债务：

```text
同一类转换逻辑在 ContentRuntimeAdapter 和 h5_dialog 内部各有一份。
后续新增 scenario 字段时，低上下文 AI 可能只改其中一份。
旧 H5 仍然知道 JSON loader 和 ContentRegistry。
```

复审判断：

```text
不阻塞 P42 通过。
因为旧 H5 后续会被 V2 世界网格协议重建，不建议在旧 H5 上继续大改。
但 P43 以后必须把“前端/旧 H5 直接加载内容”的路径收口到后端 Command / Projection / Runtime Adapter 链路。
```

建议归入后续：

```text
P43-P48：所有 V2 世界行为走 WorldCommandPipeline。
P53：V2 H5 只消费 projection / patch / available_commands，不读取 ContentRegistry。
```

### P1-2：旧 H5 仍允许内容加载失败后 fallback 到内置场景

位置：

```text
backend/src/h5_dialog/dialog_scenario.cpp
```

现状：

```text
content/core 加载失败、校验错误或 scenario 构建失败时，会输出 stderr 并 fallback 到 buildDefaultScenario()。
```

风险：

```text
开发者可能以为自己正在测试 JSON 内容，实际跑的是旧内置场景。
这会掩盖 content/core 破损。
```

复审判断：

```text
不阻塞 P42。
因为 P42 loader 自身 Strict 模式已经能阻断 registry 构建，专项测试也能发现 JSON 破损。
但旧 H5 的 fallback 行为必须在 V2 H5 中禁止。
```

后续要求：

```text
V2 世界 bootstrap 如果声明 StrictContentRequired，则内容失败必须返回错误响应，不能静默进入内置世界。
旧 H5 fallback 只允许作为 legacy 演示兼容，不允许成为 V2 行为。
```

### P1-3：content/core 尚未提供 actions 目录和 action 定义文件

P42 设计文档预留了：

```text
content/core/actions/*.json
ActionDefinitionDto
ContentRegistry::findAction()
```

当前 `content/core/manifest.json` 未引用 actions 文件，`content/core/actions/` 目录不存在。

复审判断：

```text
不阻塞 P42。
因为 P42 第一版目标允许只迁移最小闭环内容，且旧 H5 的行为输入仍由 Dialog/H5 行为层承接。
但进入 V2 Command 管线后，action 定义必须与 WorldCommandOption / available_commands 对齐。
```

后续要求：

```text
P43/P44 建立 WorldCommandKind 与 content action 的桥接边界。
P47 资源采集、P48 制作反应阶段补齐 action 内容包。
P53 前端按钮只能来自 available_commands，不能自己猜 action。
```

### P2-1：ContentRegistry 的 package 元数据尚未成为强权威

当前 `ContentRegistry::build` 主要构建定义表，`packageKey()`、`contentVersion()`、`schemaVersion()` 未在 build 中稳定写入。

风险：

```text
后续做存档兼容、内容版本提示、Mod 包合并时，需要更强的 package metadata。
```

复审判断：

```text
不阻塞 P42。
因为当前只加载 core 单包，版本兼容不是本阶段验收核心。
但 P54 存档/复盘前必须补齐。
```

## 6. 与 P42 设计要求的逐项对照

| 设计要求 | 当前状态 | 结论 |
|---|---|---|
| 内容包目录可加载 | `content/core/manifest.json` 和多目录 JSON 已加载 | 通过 |
| 内容定义可注册 | object/effect/feedback/reaction/agent/threat/knowledge/scenario/locale 已注册 | 通过 |
| 跨文件引用可校验 | object/effect/reaction/threat/scenario/agent knowledge 引用已测 | 通过 |
| JSON 不拥有运行时状态 | JSON 只定义初始值和模板，运行时仍由 H5/Dialog 状态承接 | 通过 |
| 内容可组合 | effect/reaction/knowledge/scenario 已分离，能支撑后续组合 | 基础通过 |
| Effect 白名单 | operation 映射到受控 EffectExecutionOpKind | 通过 |
| 错误阻断 | Strict 模式 Error/Fatal 跳过 registry | 通过 |
| 前端不读取 JSON | 当前前端不读取 JSON；旧 H5 后端目录直接 load content | 前端通过，后端 legacy 有债务 |
| 测试门禁 | 单元、集成、CTest、H5 回归均通过 | 通过 |

## 7. 最终验收意见

P42 最终复审意见：

```text
通过。
```

允许进入：

```text
P43 统一世界 Command 管线前置。
```

但进入 V2 后必须保留以下门禁：

```text
1. V2 前端永远不能读取 JSON。
2. V2 世界行为必须走 WorldCommandPipeline，不能继续让 UI 或旧 H5 直接调用业务函数。
3. V2 bootstrap 在严格内容模式下不能静默 fallback。
4. content action 定义要在 WorldCommandOption / available_commands 体系中补齐。
5. 内容版本元数据要在存档、复盘和内容包兼容阶段补齐。
```

## 8. 本次复审后对计划的影响

计划更新方向：

```text
P42 标记为已实现并复审通过。
原“P43 基础内容包扩展”不再作为下一阶段。
V2.0 新路线已经调整为：
P43：统一世界 Command 管线前置。
P44：世界网格基础与探索投影。
P45：背包、物品实例与归属。
P46：世界生成器与资源分布。
P47：资源采集、砍伐、挖掘。
P48：制作与世界反应接入。
P49：探索行为产生知识。
P50：教学与无知 NPC 基础行动。
P51：Agent 网格目标执行。
P52：野兽网格生态与遭遇。
P53：H5 V2 地图界面。
P54：存档、复盘与调试门禁。
```
