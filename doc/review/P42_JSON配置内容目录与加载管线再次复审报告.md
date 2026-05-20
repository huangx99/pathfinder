# P42 JSON 配置内容目录与加载管线再次复审报告

复审日期：2026-05-20

复审结论：**未通过，需要继续修复核心内容系统问题**。

口径调整：由于 V2.0 已明确后续用 V2 世界网格协议重建 H5，旧 H5 对话 / playable 原型暂不继续修复，相关 `h5_dialog_intent`、旧 `DialogScenario` 行为兼容、旧按钮式 H5 交互不再作为 P42 验收阻塞项。

本次研发已修复一部分问题：H5 `DialogScenarioCatalog::defaultScenario()` 已经尝试优先加载 `content/core`，P42 内容单元测试和集成测试可以通过。但 P42 仍存在核心验收阻塞：JSON effect 尚未接入推理/执行、场景初始状态仍未进入运行时初始化、配置 Error 仍可能构建 registry。

## 1. 本次检查范围

检查文件范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/src/h5_dialog/dialog_scenario.cpp
backend/src/h5_dialog/dialog_session.cpp
backend/include/pathfinder/h5_dialog/dialog_types.h
backend/include/pathfinder/content/*
backend/src/content/*
backend/tests/unit/content/*
backend/tests/integration/p42/*
content/core/*
```

对照文档：

```text
doc/71_P42_JSON配置内容目录与加载管线任务卡设计.md
doc/review/P42_JSON配置内容目录与加载管线审核报告.md
doc/review/P42_JSON配置内容目录与加载管线复审报告.md
```

## 2. 已执行验证

### 2.1 构建验证

执行：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_content pathfinder_tests_content_integration -j 2
cmake --build build/backend --target pathfinder_tests_h5_playable -j 2
cmake --build build/backend --target pathfinder_tests_h5_dialog_types pathfinder_tests_h5_dialog_intent pathfinder_tests_h5_dialog_scenario pathfinder_tests_h5_dialog_session pathfinder_tests_h5_dialog_learning_adapter pathfinder_tests_h5_dialog_presenter pathfinder_tests_h5_dialog_wire_codec -j 2
```

结果：通过。

### 2.2 P42 专项测试

执行：

```text
./build/backend/tests/pathfinder_tests_content loader
./build/backend/tests/pathfinder_tests_content registry
./build/backend/tests/pathfinder_tests_content validation
./build/backend/tests/pathfinder_tests_content runtime_adapter
./build/backend/tests/pathfinder_tests_content_integration flow
cd build/backend/tests && ./pathfinder_tests_content_integration flow
```

结果：全部通过。

说明：P42 Loader / Registry / Validation / Runtime Adapter 的独立测试是通过的。

### 2.3 H5 关键回归

执行：

```text
./build/backend/tests/pathfinder_tests_h5_dialog_scenario default_scenario
./build/backend/tests/pathfinder_tests_h5_dialog_scenario find_object
./build/backend/tests/pathfinder_tests_h5_dialog_scenario find_feedback
./build/backend/tests/pathfinder_tests_h5_dialog_session session_create
./build/backend/tests/pathfinder_tests_h5_playable story
./build/backend/tests/pathfinder_tests_h5_playable buttons_executable
./build/backend/tests/pathfinder_tests_h5_playable companion_taught_torch_autonomy
./build/backend/tests/pathfinder_tests_h5_playable frontend_gate
```

结果：通过。

### 2.4 H5 Intent 回归记录（已从 P42 阻塞项移除）

执行：

```text
ctest --test-dir build/backend -R '^h5_dialog_intent_' --output-on-failure
```

结果：失败。

失败用例：

```text
h5_dialog_intent_observe
h5_dialog_intent_eat_red_berry
h5_dialog_intent_teach
h5_dialog_intent_unknown
```

共同失败原因：

```text
backend/tests/unit/h5_dialog/dialog_intent_test.cpp:48
Assertion `use_beast.value().kind == DialogIntentKind::TryUse' failed.
```

口径调整：该失败属于旧 H5 对话原型与 JSON 场景接入后的兼容问题。由于项目后续不再修旧 H5，而是用 V2 世界网格协议重建 H5，本问题保留记录，但不再作为 P42 阻塞项。

## 3. 已修复项确认

### 3.1 H5 已经尝试优先加载 JSON 内容包

当前 `backend/src/h5_dialog/dialog_scenario.cpp` 中：

```text
DialogScenarioCatalog::defaultScenario()
```

已经执行：

```text
JsonContentLoader loader;
loader.load(options);
buildFromContent(*result.registry, "first_night");
```

这比上一轮“完全不接入 H5”有进步。

### 3.2 CMake 不再形成 h5_dialog 与 content_runtime_adapter 的直接互链

当前：

```text
pathfinder_h5_dialog -> pathfinder_content
pathfinder_content_runtime_adapter -> pathfinder_content + pathfinder_h5_dialog
```

`pathfinder_h5_dialog` 不再链接 `pathfinder_content_runtime_adapter`，上一轮的直接互链风险降低。

但仍有问题：`content_runtime_adapter` 现在和 `dialog_scenario.cpp::buildFromContent()` 出现重复适配逻辑，且 H5 默认场景没有使用 `ContentRuntimeAdapter`，这会造成后续维护分叉。

### 3.3 P42 内容专项测试仍通过

P42 的 parser、registry、validator、adapter、integration flow 均通过，说明基础加载能力没有退化。

## 4. 仍未修复的阻塞问题

## Legacy-1：旧 H5 Intent 回归失败（不再作为 P42 阻塞项）

问题位置：

```text
content/core/objects/basic_survival.json
backend/src/h5_dialog/dialog_intent.cpp
backend/tests/unit/h5_dialog/dialog_intent_test.cpp
```

当前 `beast_shadow` 在 JSON 中：

```text
allowed_actions: ["inspect"]
```

旧测试要求：

```text
使用靠近的野兽 -> TryUse / Use / beast_shadow
```

现在因为对象没有 `use` 动作，解析结果不是 `TryUse`，导致 H5 intent 用例失败。

口径调整：旧 H5 对话 / playable 原型后续不再维护，V2.0 会用世界网格协议重建 H5。因此本问题只作为 legacy 记录，暂不修复，也不作为 P42 阻塞项。

## P0-1：EffectExecutionSpec / EffectSemantics 仍未从 JSON 接回运行时

设计文档要求 `ContentRuntimeAdapter` 第一版至少支持：

```text
buildDialogScenario
buildEffectSpecs
buildEffectSemantics
```

当前 `backend/include/pathfinder/content/content_runtime_adapter.h` 仍只提供：

```text
buildDialogScenario
buildDialogObject
buildDialogFeedback
```

没有：

```text
buildEffectSpecs
buildEffectSemantics
```

同时运行时仍大量使用：

```text
builtInEffectExecutionSpecs()
builtInEffectSemantics()
EffectExecutionSpecRegistry 默认内置注册
EffectSemanticsRegistry 默认内置注册
```

影响：

- `content/core/effects/basic_effects.json` 仍只是可解析内容，不是推理/执行权威。
- 新增毒蘑菇、普通蘑菇、工具耐久、野兽效果时，仍可能需要改 C++ 内置效果表。
- P42 设计目标“新增内容主要通过 JSON，而不是改核心 C++ 架构”仍未达成。

结论：这是验收阻塞。

## P0-2：Scenario 初始数量和 numeric 仍未进入运行时初始化

JSON 场景已经声明：

```text
red_berry quantity = 3
decayed_red_berry quantity = 6
axe numeric.sharpness = 3
wood numeric.processed = 0
torch quantity = 0
camp_fire quantity = 0
```

但 `DialogScenarioObject` 仍没有初始 runtime state 字段：

```text
backend/include/pathfinder/h5_dialog/dialog_types.h
DialogScenarioObject 只有 object_key/display/visibility/actions/aliases/safe_tags
```

`DialogSessionState` 初始化仍在：

```text
backend/src/h5_dialog/dialog_session.cpp
```

按 object_key 写死：

```text
red_berry -> 3
decayed_red_berry -> 6
bitter_leaf -> 2
wood -> 2
dry_grass -> 2
torch/camp_fire -> 0
beast_shadow -> threat_level / observed_fire / flank_waits
axe -> sharpness
```

影响：

- 改 `content/core/scenarios/first_night.json` 的 quantity / numeric 不会真正改变新会话初始化。
- 新对象需要初始状态时仍要改 C++。
- 这违反 P42 “JSON 定义初始值，运行时状态由 Session/World 管理”的设计边界。

结论：这是验收阻塞。

## P0-3：配置 Error 仍可能构建 registry

`JsonContentLoader::load()` 当前仍只检查：

```text
result.validation_report.hasFatal()
```

然后继续：

```text
result.registry = ContentRegistry::build(draft)
```

但结构校验和引用校验中大量问题是 `Error`，不是 `Fatal`。

设计文档要求：

```text
Fatal / Error：正式运行拒绝启动内容包。
禁止配置错误时只打日志继续运行。
```

虽然 `DialogScenarioCatalog::defaultScenario()` 使用 registry 前额外检查了 `hasErrors()`，但 Loader 本身仍会在 Error 状态下生成 registry。后续其他调用方如果只判断 `registry != nullptr`，仍会误用脏内容。

结论：需要在 strict 模式下 `hasErrors()` 时不构建 registry，或返回明确不可用结果。

## 5. 仍存在的 P1 风险

### P1-1：default_agent_knowledge 仍未校验和适配

`ScenarioDefinitionContent` 仍包含：

```text
default_agent_knowledge
```

但 `ContentReferenceValidator::validateScenarioReferences()` 只校验：

```text
default_player_knowledge
```

没有校验：

```text
default_agent_knowledge
```

`buildFromContent()` 和 `ContentRuntimeAdapter::buildDialogScenario()` 也只处理 `default_player_knowledge`。

后续专家 NPC / 初始同伴知识会因此配置了但不生效。

### P1-2：validation issue 的 file_path 仍不完整

结构校验和引用校验大量 issue 仍使用：

```text
"", "$.json_path"
```

没有具体 `file_path`。

内容包变大后，错误定位会非常困难。

### P1-3：重复 key 仍会静默覆盖

`ContentRegistry::build()` 仍使用：

```text
registry->objects_[obj.key.value()] = obj
```

`checkNoDuplicates()` 仍是占位：

```text
The unordered_map nature prevents duplicates by construction.
```

实际上 unordered_map 是覆盖，不是检测重复。

P42 设计禁止同 key 静默覆盖，这个风险仍在。

### P1-4：适配逻辑重复，后续容易分叉

现在有两套内容到 DialogScenario 的转换：

```text
backend/src/h5_dialog/dialog_scenario.cpp::buildFromContent
backend/src/content/content_runtime_adapter.cpp::ContentRuntimeAdapter::buildDialogScenario
```

这两套逻辑功能相近，但不是同一个入口。

风险：

- 后续修一个漏一个。
- P42 测试测的是 ContentRuntimeAdapter，但 H5 实际用的是 inline buildFromContent。
- JSON 接入真实 H5 的覆盖测试不足。

建议后续收敛为一个权威适配入口，或者给 H5 默认场景增加专门测试，断言它真实来自 JSON。

### P1-5：`json.hpp` 迁移仍需提交时确认

当前状态仍有：

```text
D json.hpp
?? backend/third_party/nlohmann/json.hpp
```

如果这是有意迁移，需要提交时确认新 vendor 文件被纳入版本控制，且没有代码依赖根目录 `json.hpp`。

## 6. 建议修复顺序

建议按以下顺序修：

1. 实现 `buildEffectSpecs()` / `buildEffectSemantics()` 或等价的 JSON effect 运行时合并入口。
2. 把 scenario 初始 `quantity / numeric / visible` 接入运行时初始化，避免 object_key 写死数量继续扩散。
3. `JsonContentLoader::load()` 在 strict 模式下遇到 Error 不构建 registry。
4. 补 `default_agent_knowledge` 校验和适配。
5. 处理重复 key 检测。
6. 保留旧 H5 intent 失败记录，但不投入修复；等 V2 H5 世界网格协议替换后统一删除。
7. 补测试：JSON effect 能进入推理/执行、JSON 修改初始状态能影响运行时初始化、配置 Error 不产生可用 registry。

## 7. 本次复审结论

当前 P42：**未通过，但旧 H5 intent 回归不再计入阻塞**。

已改善：

- H5 默认场景开始尝试加载 JSON 内容包。
- P42 专项测试通过。
- CMake 直接互链风险有所降低。

仍阻塞 P42 的核心原因：

- JSON effect 未接入推理/执行。
- JSON 初始状态未接入运行时初始化。
- Loader 仍可能在 Error 状态下构建 registry。
- default_agent_knowledge、重复 key、file_path 等风险仍未处理。

不再阻塞 P42 的 legacy 问题：

- `h5_dialog_intent_*` 回归失败。
- 旧 `DialogScenario` 行为兼容。
- 旧 H5 对话 / playable 按钮交互。

建议修完新的 P0-1 到 P0-3 后再申请验收。
