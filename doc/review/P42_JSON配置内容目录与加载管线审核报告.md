# P42 JSON 配置内容目录与加载管线审核报告

审核日期：2026-05-20

## 1. 审核结论

当前 P42 实现 **不建议验收**。

原因不是方向错误，而是当前实现处于“能编译、部分测试能过，但架构边界和接入完整性不足”的状态。

本阶段已经做了有价值的开始：

- 已建立 `pathfinder_content` 模块。
- 已建立 `content/core` JSON 内容目录。
- 已加入本地 `backend/third_party/nlohmann/json.hpp`，不再依赖 CMake 运行时联网下载。
- 已实现 `ContentRegistry`、`JsonContentLoader`、结构校验、引用校验、Runtime Adapter 雏形。
- P42 单元测试基本可通过。

但距离 P42 设计文档要求仍有关键差距。

## 2. 已检查范围

检查文件范围：

```text
backend/CMakeLists.txt
backend/include/pathfinder/content/*
backend/src/content/*
backend/src/h5_dialog/dialog_scenario.cpp
backend/tests/unit/content/*
backend/tests/integration/p42/*
content/core/*
content/dev/*
backend/third_party/nlohmann/json.hpp
```

## 3. 已执行验证

### 3.1 构建验证

执行：

```text
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_content pathfinder_tests_content_integration -j 2
```

结果：通过。

说明：当前已不再卡在联网下载 `nlohmann_json`，这是比上一版半成品更好的地方。

### 3.2 P42 单元测试

执行：

```text
./build/backend/tests/pathfinder_tests_content loader
./build/backend/tests/pathfinder_tests_content registry
./build/backend/tests/pathfinder_tests_content validation
./build/backend/tests/pathfinder_tests_content runtime_adapter
```

结果：通过。

### 3.3 P42 集成测试

从仓库根目录执行：

```text
./build/backend/tests/pathfinder_tests_content_integration flow
```

结果：失败。

失败原因：测试中写死了相对路径 `../../../content`，从仓库根目录运行时找不到内容目录，导致 `registry == nullptr`。

从 `build/backend/tests` 目录执行：

```text
cd build/backend/tests && ./pathfinder_tests_content_integration flow
```

结果：通过。

结论：测试依赖工作目录，不稳定。

### 3.4 CTest 验证

执行：

```text
ctest --test-dir build/backend -R 'content|h5_playable_(bootstrap|p38_world|beast_reentry)' --output-on-failure
```

结果：P42 注册测试和相关 H5 测试通过，但正则误匹配到旧的 `agent_*content*`、`reaction_content_*` 测试，这些目标未构建而显示 Not Run。

结论：P42 自身注册进 CTest 后可以运行，但审核时不能用宽泛 `content` 正则判断全局通过。

### 3.5 H5 关键回归

执行：

```text
./build/backend/tests/pathfinder_tests_h5_playable story
./build/backend/tests/pathfinder_tests_h5_playable buttons_executable
./build/backend/tests/pathfinder_tests_h5_playable companion_taught_torch_autonomy
./build/backend/tests/pathfinder_tests_h5_playable frontend_gate
```

结果：通过。

说明：当前没有明显破坏 H5 最小闭环。

## 4. 主要问题

## P0-1：CMake / 模块依赖出现架构循环风险

当前 CMake 中：

```text
pathfinder_content_runtime_adapter -> pathfinder_h5_dialog
pathfinder_h5_dialog -> pathfinder_content_runtime_adapter
```

相关位置：

```text
backend/CMakeLists.txt
backend/src/h5_dialog/dialog_scenario.cpp
backend/include/pathfinder/content/content_runtime_adapter.h
```

这属于明显的分层风险。

P42 设计要求是：

```text
ContentRuntimeAdapter 把内容定义适配进旧系统。
旧系统只能读 adapter 产物，不能自己解析 JSON。
```

但当前实现让 `h5_dialog` 直接 include：

```cpp
#include "pathfinder/content/json_content_loader.h"
#include "pathfinder/content/content_runtime_adapter.h"
```

并且 `content_runtime_adapter` 又 include `h5_dialog/dialog_types.h`。

这会导致：

- 内容层与 H5 Dialog 层互相知道对方。
- 后续拆分模块时容易出现链接顺序和依赖混乱。
- 低级 AI 后续会更容易继续把 JSON 加载逻辑写进业务层。

建议修复方向：

1. 保留 `pathfinder_content` 为纯内容模块，不依赖 `h5_dialog`。
2. 将 `content_runtime_adapter` 改成上层桥接模块，例如 `pathfinder_h5_dialog_content_adapter`。
3. `h5_dialog` 不应反向链接这个 adapter；要么由更上层服务注入 scenario，要么把 adapter 逻辑放在同一层但避免互链。
4. CMake 中必须消除 `h5_dialog <-> content_runtime_adapter` 循环。

## P0-2：JSON 场景接入不完整，当前更像“对象/反馈局部迁移”

当前 `ContentRuntimeAdapter::buildDialogScenario()` 主要只做：

- 读取 scenario key。
- 构建 objects。
- 收集 feedbacks。

但 P42 设计要求至少要能迁移最小 H5 生存闭环内容。

当前遗漏：

- `welcome_text` 没有从 JSON 读取。
- `quick_action_input_texts` 没有从 JSON 读取。
- `suggested_action_templates` 没有从 JSON 读取。
- `threat_knowledge_templates` 没有从 JSON 读取。
- `default_player_knowledge` 没有适配到 DialogScenario。
- `default_agent_knowledge` 没有适配到 DialogScenario。
- scenario 初始对象的 `quantity`、`visible`、`numeric` 暂未真正接入 `DialogScenarioObject` 或运行时初始化链路。

结果是：即使 H5 测试通过，也可能是因为旧逻辑 fallback 或其他路径仍在补齐能力，而不是 JSON 内容包真正成为内容来源。

建议修复方向：

- 扩展 `ScenarioDefinitionDto`，补齐设计文档里的字段。
- `content/core/scenarios/first_night.json` 必须恢复完整最小场景，而不是只写 4 个对象。
- Runtime Adapter 必须覆盖 scenario 中的 quick actions、suggested actions、threat knowledge、default knowledge。
- 集成测试必须断言这些字段来自 JSON，而不是只断言 objects/feedbacks。

## P1-1：集成测试路径依赖工作目录

当前：

```cpp
options.root_path = "../../../content";
```

从仓库根目录运行失败，从 `build/backend/tests` 运行成功。

这是测试脆弱点。

建议修复：

- 使用编译期注入的 `PATHFINDER_CONTENT_ROOT`。
- 或在 CMake 给测试传入参数。
- 或测试内根据 `argv[0]` 定位 repo root / build root。

验收要求：

```text
./build/backend/tests/pathfinder_tests_content_integration flow
```

从仓库根目录也必须通过。

## P1-2：错误处理策略偏宽，可能静默 fallback

`DialogScenarioCatalog::defaultScenario()` 当前逻辑是：

- 尝试加载 JSON。
- 有错误就输出 `std::cerr`。
- 然后 fallback 到内置场景。

这对开发期方便，但对 P42 目标有风险。

P42 设计要求：

```text
Fatal / Error：正式运行拒绝启动内容包。
Warning：允许启动，但测试中必须可断言。
旧 C++ 场景只作为 fallback，并输出 trace。
```

当前问题：

- fallback 是 `std::cerr`，没有进入结构化 trace。
- 测试没有断言“JSON 加载失败时 fallback 是否被明确记录”。
- 正式运行和开发运行没有区分。

建议修复：

- 增加明确的加载模式：`strict_content_required` / `allow_builtin_fallback`。
- 测试模式必须能断言 fallback trace。
- H5 projection 或 debug trace 中应能看到使用的是 JSON content 还是 built-in fallback。

## P1-3：Content Registry 已能建表，但还不是完整权威内容源

当前 content/core 已有：

```text
objects/effects/feedbacks/reactions/agents/threats/knowledge/scenarios/locales
```

但 manifest 没有声明 `knowledge/basic_knowledge.json`，虽然文件存在。

这会导致知识模板文件形同虚设。

建议修复：

- `content/core/manifest.json` 增加 knowledge 文件。
- ReferenceValidator 校验 scenario 默认知识引用。
- Runtime Adapter 接入默认知识模板。

## P1-4：Runtime Adapter 存在硬编码映射，但应受控收敛

当前 `mapActionKind()` 手写：

```cpp
if (action_key == "eat") return DialogActionKind::Eat;
if (action_key == "use") return DialogActionKind::Use;
...
```

这在 P42 第一版可以接受，但必须标记为兼容桥，不应继续扩散。

建议：

- 在文档/代码注释中明确这是旧 DialogActionKind 兼容映射。
- 新动作扩展不要继续在这里无限加 if。
- 后续应由 ActionDefinition + Command Adapter 统一处理。

## 5. 当前可保留的部分

以下部分方向正确，可以保留继续修：

- 本地 vendored `backend/third_party/nlohmann/json.hpp`，避免 CMake 联网下载。
- `pathfinder_content` 纯内容模块方向。
- `ContentRegistry` 查询能力。
- `ContentStructuralValidator` 与 `ContentReferenceValidator` 的基本思路。
- `content/core` 目录结构。
- P42 unit tests 的分层。

## 6. 是否可以继续开发

可以继续开发，但不能验收。

推荐先修复顺序：

1. 先消除 CMake 互链和层级循环。
2. 修复集成测试工作目录问题。
3. 补全 ScenarioDefinitionDto 与 Runtime Adapter 的缺失字段。
4. 让 `content/core/scenarios/first_night.json` 覆盖当前最小 H5 场景所需内容。
5. 增加测试断言 JSON 内容真实接入，而不是 fallback 靠旧 C++ 场景。
6. 再跑 P42 专项、H5 关键回归和 CTest 精确匹配。

## 7. 审核结论

当前 P42 状态：**不通过，需要修复后复审**。

不通过原因：

- 架构层存在 content adapter 与 h5_dialog 互相依赖风险。
- JSON 场景接入不完整，尚未达到 P42 设计的“最小 H5 生存闭环内容源”目标。
- 集成测试依赖工作目录，不能稳定从任意位置运行。
- fallback 策略不够结构化，容易掩盖 JSON 配置加载失败。

