# P11 Agent本地调试报告与导出前置复审报告

复审时间：2026-05-17  
复审对象：P11 Agent本地调试报告与导出前置实现  
复审结论：通过，允许进入 P12。

## 1. 复审范围

本次复审依据：

```text
doc/36_P11Agent本地调试报告与导出前置任务卡设计.md
context_packs/agent_p11.md
P10 Agent历史查询与协议投影前置审核结论
```

重点复查：

```text
DebugReport 是否只消费 P10 AgentHistoryProjection。
Diagnostics 是否只基于 AgentDebugReport 生成摘要。
ExportDraft 是否只生成内存 chunk，不写文件。
Public 模式是否清空 trace keys。
P11 是否没有引入 CLI / JSON / HTTP / WebSocket / H5 / SaveManager。
P11 是否没有调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
P11 是否没有读取 GameState / ObjectDefinition / hidden truth。
```

## 2. 变更范围核对

P11 新增文件符合任务卡范围：

```text
backend/include/pathfinder/agent/debug_report.h
backend/include/pathfinder/agent/debug_export.h
backend/src/agent/debug_report.cpp
backend/src/agent/debug_export.cpp
backend/tests/unit/agent/agent_debug_report_test.cpp
backend/tests/unit/agent/agent_debug_export_test.cpp
backend/tests/integration/p11/agent_debug_report_export_flow_test.cpp
backend/tests/integration/p11/agent_no_decision_debug_report_test.cpp
context_packs/agent_p11.md
```

P11 修改文件符合任务卡范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

未发现 P11 禁止范围变更：

```text
未修改 rules / object / pipeline / replay / protocol。
未新增 frontend / http / websocket / save 目录。
```

说明：工作区中存在 P12 设计文档和 P10 审核报告等未跟踪文件，不属于本次 P11 复审范围。

## 3. 功能复审

### 3.1 DebugReport 层

结果：通过。

已确认能力：

```text
AgentDebugReportId helper 稳定生成。
AgentDebugReportMode / AgentDebugReportSeverity 支持 roundtrip。
AgentDebugReportItem 校验 record_id / agent_id / actor_id / runtime_status / decision_status / severity。
AgentDebugReport 校验 report_id / mode / count 边界 / item 合法性。
AgentDebugReportBuilder 从 P10 AgentHistoryProjection 生成报告。
ReplayLocked / ExplainedOnly / Broken / NoDecision / Skipped 计数可形成摘要。
Public 模式拒绝 include_trace_keys=true。
Debug / Training 模式可按请求保留 reason_keys / phase_keys / warning_keys。
```

边界结论：DebugReport 层没有读取 `AgentTickRecord`，没有读取 `GameState` / `ObjectDefinition`，没有调用运行时或规则管线。

### 3.2 Diagnostics 层

结果：通过。

已确认能力：

```text
AgentDiagnosticsStatus 支持 roundtrip。
AgentDiagnosticsIssue 校验 severity 和 issue_key。
AgentDiagnosticsSummary 校验 status 和 issues。
AgentDiagnosticsBuilder 可识别 broken_replay_lock。
AgentDiagnosticsBuilder 可识别 invalid_unknown_status。
Debug / Training 模式下 NoDecision 缺解释会生成 warning。
Public 模式 trace keys 泄露会生成 error。
无错误无警告时 status=Clean。
```

边界结论：Diagnostics 只消费 `AgentDebugReport`，不反向读取 P8/P9/P10 内部原始结构。

### 3.3 ExportDraft 层

结果：通过。

已确认能力：

```text
AgentExportFormat 支持 PlainText / MarkdownLike / ProtocolText。
AgentExportChunk 校验 chunk_id / format / title_key / payload。
AgentExportManifest 校验 manifest_id / format / chunk_count。
AgentExportDraft 校验 manifest 和 chunk id 顺序一致。
AgentExportDraftBuilder 可生成 PlainText / MarkdownLike / ProtocolText 内存 payload。
max_items_per_chunk=0 会失败。
manifest.warning_keys 可吸收 diagnostics warning issue_key。
payload 不包含 hidden truth 关键词。
```

边界结论：ExportDraft 只生成内存结构，不写文件，不访问路径，不使用 filesystem/fstream，不做 JSON，不做网络发送。

## 4. 测试结果

构建结果：通过。

```bash
cmake -S backend -B build/backend
cmake --build build/backend
```

P11 定向测试：通过。

```text
100% tests passed, 0 tests failed out of 56
```

P3-P11 / Agent / Replay / Protocol 阶段回归：通过。

```text
100% tests passed, 0 tests failed out of 286
```

全量测试：通过。

```text
100% tests passed, 0 tests failed out of 332
```

## 5. 边界扫描结果

运行时 / 管线 / 复盘 / 原始记录扫描：通过。

```text
扫描目标：debug_report.h/.cpp、debug_export.h/.cpp
扫描内容：AgentRuntime( / FirstSupportedPolicy / decide( / tickOne( / RulePipeline / execute( / ReplayRunner / AgentTickRecord
结果：无命中
```

隐藏真相扫描：通过。

```text
扫描目标：P11 生产代码、单元测试、集成测试
扫描内容：GameState / ObjectDefinition / edible_profile / hunger_delta / health_delta / effect_kind
结果：无命中
```

文件 / 网络 / JSON / 存档扫描：通过。

```text
扫描目标：P11 生产代码和 integration/p11
扫描内容：fstream / ofstream / ifstream / filesystem / file_path / absolute_path / write_result / nlohmann / json / socket / send( / recv( / HTTP / WebSocket / SaveManager
结果：无命中
```

补充说明：扩展扫描包含 `argc|argv` 时，命中了测试可执行入口 main 参数；P11 任务卡未禁止测试入口使用参数，且生产代码无 `argc|argv` 命中，因此不构成问题。

Reward / Done / RL 扫描：通过。

```text
扫描目标：agent include/src、integration/p11
扫描内容：reward_value / done = / is_done / RewardCalculator / EpisodeRunner / rl_environment / RLPolicy / NeuralPolicy / LLMPolicy
结果：无命中
```

目录越界扫描：通过。

```text
未发现 frontend / http / websocket / save 相关新增目录。
```

## 6. 非阻塞建议

### 6.1 DiagnosticsSummary 可增强 count 一致性校验

当前 `AgentDiagnosticsSummary::validateBasic` 校验 status 和 issue 合法性，但没有强制 `warning_count` / `error_count` 与 issues 中 severity 数量一致。当前 builder 路径正确、测试通过；建议后续字段扩展时补充一致性校验。

结论：非阻塞，不影响 P11 通过。

### 6.2 测试 target 链接依赖可后续收窄

P11 测试 target 链接了 runtime / pipeline / rules 等上游库，这是测试链接依赖层面的宽依赖；生产代码边界扫描确认没有调用运行时或规则管线。后续可以在 CMake 清理阶段收窄测试链接依赖。

结论：非阻塞，不影响 P11 通过。

## 7. 最终结论

P11 复审通过。

理由：

```text
实现范围符合 P11 任务卡。
DebugReport 只基于 P10 Projection。
Diagnostics 只基于 AgentDebugReport。
ExportDraft 只生成内存 chunk，不写文件。
Public 模式 trace 边界受保护。
没有读取隐藏真相。
没有调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
没有引入 JSON / HTTP / WebSocket / H5 / SaveManager。
P11 定向测试、阶段回归、全量测试全部通过。
```

允许进入：P12 Agent本地文件导出与校验。
