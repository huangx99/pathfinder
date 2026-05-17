# P14 Agent 调试输入适配前置审核报告

审核日期：2026-05-17  
审核对象：P14 Agent 调试输入适配前置实现  
审核结论：通过审核

---

## 1. 审核结论

P14 实现通过审核，可以作为 P15 认知系统正式化之前的稳定调试输入适配层。

本次审核确认：

1. P14 只做“安全调试输入适配”，没有越界进入认知、记忆、学习、策略执行、奖励计算、完成态计算。
2. P14 输入来源边界清晰，只接收 P13 fixture、P10 历史投影、P11 调试报告/诊断摘要、P12 导出草稿、测试专用内存包。
3. P14 对外部不可信输入、测试专用输入、导出草稿反向构造报告、隐藏真值字段、原始状态字段均有明确拒绝路径。
4. P14 已接入构建和测试体系，定向测试、相关回归、全量测试全部通过。

---

## 2. 审核范围

### 2.1 设计依据

- `doc/40_P14Agent调试输入适配前置任务卡设计.md`
- `context_packs/agent_p14.md`
- P10：`AgentHistoryProjection`
- P11：`AgentDebugReport` / `AgentDiagnosticsSummary`
- P12：`AgentExportDraft` / 本地文件导出校验
- P13：Agent 本地调试 CLI fixture

### 2.2 代码范围

- `backend/include/pathfinder/agent/debug_input.h`
- `backend/src/agent/debug_input.cpp`
- `backend/tests/unit/agent/agent_debug_input_test.cpp`
- `backend/tests/integration/p14/agent_debug_input_flow_test.cpp`
- `backend/tests/integration/p14/agent_debug_input_security_test.cpp`
- `backend/CMakeLists.txt`
- `backend/tests/CMakeLists.txt`
- `backend/dev_notes/agent.md`

---

## 3. 实现完整性审核

### 3.1 类型与数据契约

结论：通过。

已实现 P14 需要的核心类型：

- `AgentDebugInputId`
- `AgentDebugInputKind`
- `AgentDebugInputTrustLevel`
- `AgentDebugInputValidationStatus`
- `AgentDebugInputCapability`
- `AgentDebugInputRejectReason`
- `AgentDebugInputManifest`
- `AgentDebugInputSource`
- `AgentDebugInputValidationIssue`
- `AgentDebugInputValidationReport`
- `AgentDebugInputAdapterOptions`
- `AgentDebugInputBundle`

对应位置：

- `backend/include/pathfinder/agent/debug_input.h:18`
- `backend/include/pathfinder/agent/debug_input.h:23`
- `backend/include/pathfinder/agent/debug_input.h:35`
- `backend/include/pathfinder/agent/debug_input.h:46`
- `backend/include/pathfinder/agent/debug_input.h:56`
- `backend/include/pathfinder/agent/debug_input.h:68`
- `backend/include/pathfinder/agent/debug_input.h:97`
- `backend/include/pathfinder/agent/debug_input.h:113`
- `backend/include/pathfinder/agent/debug_input.h:124`
- `backend/include/pathfinder/agent/debug_input.h:134`
- `backend/include/pathfinder/agent/debug_input.h:147`
- `backend/include/pathfinder/agent/debug_input.h:158`

这些类型保持 DTO 化，没有绑定运行时、规则管线、存档系统或网络系统。

### 3.2 工厂入口

结论：通过。

`AgentDebugInputFactory` 提供稳定构造入口，避免研发手写 manifest 和 ID：

- `fromFixture`
- `fromProjection`
- `fromReport`
- `fromExportDraft`
- `fromTestReport`

对应位置：

- `backend/include/pathfinder/agent/debug_input.h:186`
- `backend/src/agent/debug_input.cpp:1057`
- `backend/src/agent/debug_input.cpp:1081`
- `backend/src/agent/debug_input.cpp:1101`
- `backend/src/agent/debug_input.cpp:1125`
- `backend/src/agent/debug_input.cpp:1145`

审核结果：

- fixture 输入标记为 `BuiltinFixtureBundle` + `Builtin`。
- projection/report/export draft 标记为 `GeneratedSafeDto`。
- test 输入标记为 `InMemoryTestBundle` + `TestOnly`。
- ID 命名遵循 P14 设计约定。
- 未发现从 `GameState`、`ObjectDefinition`、`AgentTickRecord`、SaveGame 或 SaveManager 直接构造调试输入的入口。

### 3.3 校验器

结论：通过。

`AgentDebugInputValidator::validate` 按 P14 要求建立了输入防线：

- 先校验 adapter options。
- 再执行 source 基础校验。
- 校验 schema 版本，只接受 `agent_debug_input.v1`。
- 默认拒绝 `ExternalUntrusted`。
- 默认拒绝 `test_only` 输入。
- 按 kind 检查 payload 是否存在、是否冲突、是否可转换。
- 检查 item 数量上限。
- 扫描隐藏真值字段与原始状态字段。
- 产出 `Valid` / `ValidWithWarnings` / `Invalid`。

对应位置：

- `backend/src/agent/debug_input.cpp:519`
- `backend/src/agent/debug_input.cpp:551`
- `backend/src/agent/debug_input.cpp:561`
- `backend/src/agent/debug_input.cpp:569`
- `backend/src/agent/debug_input.cpp:577`
- `backend/src/agent/debug_input.cpp:727`
- `backend/src/agent/debug_input.cpp:860`
- `backend/src/agent/debug_input.cpp:919`

### 3.4 适配器

结论：通过。

`AgentDebugInputAdapter::adapt` 的关键顺序正确：先校验，再适配。

对应位置：

- `backend/src/agent/debug_input.cpp:941`
- `backend/src/agent/debug_input.cpp:945`
- `backend/src/agent/debug_input.cpp:950`

适配路径审核结果：

- fixture：通过 P13 fixture 构建 report + diagnostics。
- projection：通过 P11 report builder 构建 report，再生成 diagnostics。
- report：透传 report，并补齐 diagnostics。
- export draft：默认不允许作为 report 来源；仅在 `allow_export_draft_only=true` 时保留 draft，不反向构造 report。
- test bundle：默认拒绝；只有显式 `allow_test_only=true` 时才进入测试路径。

对应位置：

- `backend/src/agent/debug_input.cpp:962`
- `backend/src/agent/debug_input.cpp:971`
- `backend/src/agent/debug_input.cpp:986`
- `backend/src/agent/debug_input.cpp:998`
- `backend/src/agent/debug_input.cpp:1009`

---

## 4. 边界与安全审核

### 4.1 不允许依赖的系统

结论：通过。

扫描范围：

- `backend/include/pathfinder/agent/debug_input.h`
- `backend/src/agent/debug_input.cpp`
- `backend/tests/integration/p14`

扫描关键字：

- `AgentRuntime(`
- `FirstSupportedPolicy`
- `decide(`
- `tickOne(`
- `RulePipeline`
- `execute(`
- `ReplayRunner`
- `SaveManager`
- `httplib`
- `asio`
- `WebSocket`
- `HTTP server`
- `nlohmann`
- `json`
- `socket`
- `send(`
- `recv(`

结果：未发现 P14 调试输入适配层调用运行时、策略、规则管线、回放运行器、存档、HTTP/WebSocket/JSON 解析或网络接口。

### 4.2 隐藏真值与原始状态防护

结论：通过。

P14 明确维护禁止字段清单：

- `edible_profile`
- `hunger_delta`
- `health_delta`
- `effect_kind`
- `ObjectDefinition`
- `GameState`
- `AgentTickRecord`
- `reward_value`
- `done =`
- `is_done`
- `SaveGame`
- `SaveManager`

对应位置：

- `backend/src/agent/debug_input.cpp:158`
- `backend/src/agent/debug_input.cpp:188`

扫描结果说明：

- 命中点只出现在禁止字段清单与安全测试中。
- 未发现业务适配路径使用隐藏真值生成 report。
- 未发现 export draft 反向解析文本构造 report。

### 4.3 测试专用输入防护

结论：通过。

- `test_only` 默认拒绝。
- 只有 `allow_test_only=true` 时才允许 `InMemoryTestBundle`。
- 集成安全测试覆盖了默认拒绝与显式允许两条路径。

对应位置：

- `backend/src/agent/debug_input.cpp:569`
- `backend/src/agent/debug_input.cpp:1009`
- `backend/tests/integration/p14/agent_debug_input_security_test.cpp:243`
- `backend/tests/integration/p14/agent_debug_input_security_test.cpp:271`

---

## 5. 测试审核

### 5.1 构建验证

命令：

```bash
cmake --build build/backend
```

结果：通过。

### 5.2 P14 定向测试

命令：

```bash
ctest --test-dir build/backend -R "agent_debug_input|p14" --output-on-failure
```

结果：42/42 通过。

覆盖内容：

- 枚举 roundtrip。
- factory 构造。
- validator 正常输入。
- validator 拒绝 unknown kind / unknown trust level / external untrusted / missing payload / conflicting payloads。
- validator 拒绝 hidden truth / raw state。
- adapter fixture/report/projection/export draft/test bundle。
- P13 fixture 经 P14 adapter 进入 report。
- P11 report 经 P14 进入 P12 导出。
- P10 projection 经 P14 进入 report。
- export draft 不反向构造 report。
- P14 安全边界。

### 5.3 相关回归测试

命令：

```bash
ctest --test-dir build/backend -R "agent|p14|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
```

结果：443/443 通过。

### 5.4 全量测试

命令：

```bash
ctest --test-dir build/backend --output-on-failure
```

结果：489/489 通过。

---

## 6. 非阻塞建议

以下不是阻塞问题，不影响 P14 通过：

1. 后续 P15 开始后，认知/记忆/学习系统不要绕过 P14 的安全输入边界读取 P13/P10/P11/P12 数据。
2. 后续如果增加新的调试输入来源，必须先扩展 `AgentDebugInputKind`、trust level、factory、validator、安全测试，再允许 adapter 使用。
3. 后续如果接入 H5 或 HTTP 调试入口，不应写入 P14；应新建更外层的 API 层，将外部请求转为 P14 已认可的安全 DTO。

---

## 7. 最终判定

P14 通过审核。

可以进入下一阶段：P15 认知系统正式化。
