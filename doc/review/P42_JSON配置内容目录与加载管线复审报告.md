# P42 JSON 配置内容目录与加载管线复审报告

复审日期：2026-05-20

复审结论：**未通过，仍需修复后再验收**。

本次实现相比上一轮有明显进步：P42 内容目录、Loader、Registry、Reference Validator、Runtime Adapter 测试均能构建并通过，`knowledge/basic_knowledge.json` 已纳入 manifest，集成测试路径也已改为编译期注入。

但 P42 设计目标不是“能读 JSON”，而是让 JSON 内容包成为后续新增内容的权威入口。当前代码仍没有真正接入 H5 运行时和效果执行链，因此不能验收。

## 1. 复审范围

检查范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/content/*
backend/src/content/*
backend/src/h5_dialog/dialog_scenario.cpp
backend/src/h5_dialog/dialog_session.cpp
backend/src/h5_playable/*
backend/tests/unit/content/*
backend/tests/integration/p42/*
content/core/*
content/dev/*
```

重点对照：

```text
doc/71_P42_JSON配置内容目录与加载管线任务卡设计.md
doc/review/P42_JSON配置内容目录与加载管线审核报告.md
```

## 2. 已执行验证

构建：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_content pathfinder_tests_content_integration pathfinder_tests_h5_playable -j 2
```

结果：通过。

P42 单元测试：

```text
./build/backend/tests/pathfinder_tests_content loader
./build/backend/tests/pathfinder_tests_content registry
./build/backend/tests/pathfinder_tests_content validation
./build/backend/tests/pathfinder_tests_content runtime_adapter
```

结果：全部通过。

P42 集成测试：

```text
./build/backend/tests/pathfinder_tests_content_integration flow
cd build/backend/tests && ./pathfinder_tests_content_integration flow
```

结果：两个工作目录下均通过。上一轮“依赖工作目录”的问题已修复。

H5 关键回归：

```text
./build/backend/tests/pathfinder_tests_h5_playable story
./build/backend/tests/pathfinder_tests_h5_playable buttons_executable
./build/backend/tests/pathfinder_tests_h5_playable companion_taught_torch_autonomy
./build/backend/tests/pathfinder_tests_h5_playable frontend_gate
```

结果：全部通过。

说明：测试通过只能说明现有行为没有明显退化，不能证明 P42 已作为运行时内容源接入。

## 3. 已修复项

### 3.1 manifest 已纳入知识文件

`content/core/manifest.json` 已包含：

```text
knowledge/basic_knowledge.json
```

上一轮 `basic_knowledge.json` 存在但未加载的问题已修复。

### 3.2 Scenario DTO 已补充关键字段

当前 `ScenarioDefinitionDto` 已包含：

```text
welcome_text_key
quick_action_input_texts
suggested_action_templates
threat_knowledge_templates
default_player_knowledge
default_agent_knowledge
```

比上一轮只迁移对象和反馈更完整。

### 3.3 Runtime Adapter 已能构建部分 DialogScenario

`ContentRuntimeAdapter::buildDialogScenario()` 当前已能从 `ContentRegistry` 生成：

```text
scenario_key
display_name
welcome_text
objects
feedbacks
quick_action_input_texts
suggested_action_templates
threat_knowledge_templates
default_player_knowledge 对应的 default_knowledge_templates
```

### 3.4 集成测试路径问题已修复

`backend/tests/CMakeLists.txt` 通过：

```text
PATHFINDER_CONTENT_ROOT="${CMAKE_SOURCE_DIR}/../content"
```

给集成测试注入内容目录。

## 4. 阻塞问题

## P0-1：P42 没有真正接入 H5 运行时，H5 仍使用内置场景

设计文档要求：

```text
DialogScenarioCatalog::defaultScenario() 优先使用 core content。
保留 fallback。
```

当前实际代码中：

```text
backend/src/h5_dialog/dialog_scenario.cpp
```

`DialogScenarioCatalog::defaultScenario()` 仍直接调用 `buildDefaultScenario()`，而 `buildDefaultScenario()` 仍手写红果、腐烂红果、斧头、木头、火把、野兽等完整场景。

同时搜索结果显示：

```text
backend/src/h5_dialog/* 没有 JsonContentLoader / ContentRuntimeAdapter 接入
backend/src/h5_playable/* 仍通过 DialogScenarioCatalog::defaultScenario() 获取场景
```

这意味着：

- P42 JSON 内容包目前只在 P42 测试里被加载。
- 实际 H5 游戏启动仍不是从 `content/core/scenarios/first_night.json` 构建。
- 后续新增毒蘑菇、普通蘑菇、房子、狼，如果只写 JSON，H5 不会自动得到这些内容。

结论：这是验收阻塞。P42 的核心目标是内容目录化，不只是旁路测试可读。

## P0-2：EffectExecutionSpec / EffectSemantics 未从 JSON 接回运行时

设计文档 `TASK-P42-006` 要求 Runtime Adapter 第一版至少支持：

```text
buildDialogScenario
buildEffectSpecs
buildEffectSemantics
```

当前 `backend/include/pathfinder/content/content_runtime_adapter.h` 和 `backend/src/content/content_runtime_adapter.cpp` 只实现了：

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

而现有运行时仍使用：

```text
builtInEffectExecutionSpecs()
builtInEffectSemantics()
EffectExecutionSpecRegistry 默认内置注册
EffectSemanticsRegistry 默认内置注册
```

影响：

- JSON 中的 `effects/basic_effects.json` 当前能解析、能注册、能被测试读取，但还不能成为智能 NPC 推理和目标执行的权威效果来源。
- “新增毒蘑菇只需要 JSON + locale + 测试，不需要改核心执行代码”的验收目标当前做不到。
- P39/P40/P41 的智能闭环仍主要依赖 C++ 内置效果表。

结论：这是验收阻塞。否则 JSON 只是说明书，不是可执行内容。

## P0-3：Scenario 初始数量和 numeric 没有进入 Session 初始化

JSON 场景中已经声明：

```text
red_berry quantity = 3
decayed_red_berry quantity = 6
axe numeric.sharpness = 3
torch quantity = 0
beast_shadow initial threat level
```

但当前 `DialogScenarioObject` 没有承载 initial runtime state 的字段，`ContentRuntimeAdapter::buildDialogObject()` 也没有把 `ScenarioInitialObjectDto.quantity / numeric / visible` 写入 H5 可用结构。

实际 Session 初始化仍在：

```text
backend/src/h5_dialog/dialog_session.cpp
```

按 object_key 手写：

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

- JSON 改数量不会真正改变新会话。
- 新物品如果需要初始数量或 numeric 状态，仍要改 C++。
- 这违反 P42 “JSON 定义初始值，运行时状态由 Session/World 管理”的分层：当前不是从 JSON 初始化运行时，而是 Session 自己继续写死。

结论：这是验收阻塞。

## P0-4：配置错误存在继续构建 registry 的风险

`JsonContentLoader::load()` 在结构校验和引用校验后只检查：

```text
result.validation_report.hasFatal()
```

然后就继续：

```text
result.registry = ContentRegistry::build(draft)
```

但结构校验和引用校验大量问题是 `Error`，不是 `Fatal`。

设计文档禁止：

```text
禁止配置错误时只打日志继续运行。
Fatal / Error：正式运行拒绝启动内容包。
```

影响：

- 非法引用、未知对象、未知效果可能仍生成 registry。
- 上层如果只判断 `registry != nullptr`，就可能继续运行错误内容包。
- 这会在后续大内容包阶段制造非常难查的脏数据。

结论：需要至少在 strict / normal 加载模式下 `hasErrors()` 时不构建 registry；允许 fallback 的模式也必须有结构化 trace。

## 5. 非阻塞但必须修的风险

### P1-1：default_agent_knowledge 已解析但未适配

`ScenarioDefinitionDto` 有 `default_agent_knowledge`，JSON 也有字段，但 `ContentRuntimeAdapter::buildDialogScenario()` 只处理了 `default_player_knowledge`。

如果后续给专家 NPC / 同伴默认知识，只写 JSON 不会生效。

### P1-2：Reference Validator 没有校验 default_agent_knowledge

当前 `ContentReferenceValidator::validateScenarioReferences()` 校验了：

```text
default_player_knowledge
```

但没有等价校验：

```text
default_agent_knowledge
```

并且未知默认知识目前是 Warning，不是 Error。考虑到默认知识会影响 NPC 行动，建议升级为 Error 或至少 strict 模式下视为 Error。

### P1-3：校验 issue 的 file_path 不完整

Parser 失败时能带 `file_entry.path`，但结构校验和引用校验目前大量 issue 的 `file_path` 是空字符串。

设计要求错误必须指出：

```text
file_path
json_path
message
```

当前 `json_path` 有基础值，但 file_path 不能定位具体文件。后续内容包变多后，低级 AI 或人工排查会很困难。

### P1-4：重复 key 会被静默覆盖

`ContentRegistry::build()` 使用 unordered_map 直接赋值：

```text
registry->objects_[obj.key.value()] = obj
```

`checkNoDuplicates()` 当前只是占位注释。

设计禁止：

```text
禁止同一个 object_key 在多个包中静默覆盖。
```

P42 第一版如果只加载 core 包，可以暂不做 override 复杂策略，但至少应该在 draft/registry 构建阶段检测重复并报错。

### P1-5：`json.hpp` 从仓库根目录删除但新 vendor 文件还未纳入跟踪

当前 `git status` 显示：

```text
D json.hpp
?? backend/third_party/
```

如果这是有意把依赖从根目录迁移到 `backend/third_party/nlohmann/json.hpp`，方向可以接受。

但提交时必须确认：

- 新 vendor 文件被 `git add`。
- 没有其他代码仍依赖根目录 `json.hpp`。
- 删除根目录 `json.hpp` 是有意迁移，不是误删。

## 6. 当前可以保留的实现

以下方向是对的，可以在修复时保留：

- `content/core` 目录结构。
- `ContentFileCategory`、`ContentDefinitionKind`、`ContentLoadStage` 等枚举方向。
- `JsonContentParser` + `JsonContentLoader` 分离方向。
- `ContentRegistry` 只读查询方向。
- `ContentStructuralValidator` 与 `ContentReferenceValidator` 的分层方向。
- 本地 vendored `nlohmann/json.hpp`，避免构建联网。
- P42 单元测试和集成测试的基本框架。

## 7. 建议修复顺序

建议按下面顺序修，不要先扩内容：

1. 先实现 H5 场景加载入口：`DialogScenarioCatalog::defaultScenario()` 优先加载 `content/core`，失败时走可追踪 fallback。
2. 给 `DialogScenario` 或单独 Runtime Init DTO 增加初始状态承载，把 `ScenarioInitialObjectDto.quantity / numeric / visible` 接入 `DialogSessionState` 初始化。
3. `ContentRuntimeAdapter` 增加 `buildEffectSpecs()` 和 `buildEffectSemantics()`，并让运行时 registry 能合并 JSON 内容。
4. `JsonContentLoader::load()` 在 `hasErrors()` 时阻断 registry 构建，除非显式 allow-fallback/debug 模式。
5. 补 `default_agent_knowledge` 的引用校验和 Runtime Adapter 映射。
6. 给 validation issue 补 file_path，至少在 draft content 中保存来源文件。
7. 增加重复 key 检测，禁止静默覆盖。
8. 补测试：H5 默认场景必须断言来自 JSON；JSON 修改数量能影响新会话；JSON 新效果能进入推理/执行 registry。

## 8. 复审结论

当前 P42：**未通过**。

原因不是测试失败，而是实现没有达到 P42 的核心架构目标：

- JSON 内容包还没有成为 H5 最小可玩运行时内容源。
- JSON effect 还没有接入推理与执行。
- JSON 初始状态还没有接入 Session 初始化。
- 配置错误仍可能构建 registry。

建议修完 P0-1 到 P0-4 后再复审。
