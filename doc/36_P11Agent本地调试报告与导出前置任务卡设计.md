# P11 Agent本地调试报告与导出前置任务卡设计

## 1. P11 要解决什么问题

P10 让 Agent 历史、复盘锁定、训练样本草稿可以被查询并投影成安全 DTO。P11 要解决的是：

```text
研发和后续 AI 需要快速看懂一段 Agent 历史为什么这样行动、是否可复盘、是否可作为训练草稿。
但现在还没有稳定的调试报告、导出片段、失败摘要格式。
```

P11 的一句话目标：

```text
把 P10 的只读投影整理成可读的本地调试报告和导出片段，方便研发、AI 审核、未来 CLI/H5 复用。
```

P11 是“报告与导出前置”，不是 CLI，不是文件系统导出，不是 HTTP，不是 H5。

## 2. P11 与前置阶段的关系

P11 依赖：

```text
P8：AgentTickRecord / AgentDecisionRecord。
P9：AgentReplayLockSet / AgentTrainingSampleDraft。
P10：AgentHistoryProjection / AgentReplayLockProjection / AgentTrainingSampleProjection / ProtocolEnvelope。
```

P11 产出：

```text
AgentDebugReport：人类可读的结构化调试报告。
AgentExportManifest：未来导出文件的清单草稿，但 P11 不写文件。
AgentExportChunk：内存级导出片段，可以是 text/markdown-like/plain payload。
AgentDiagnosticsSummary：给 AI 审核和测试失败定位用的摘要。
```

P11 不改变：

```text
规则权威仍是 RulePipeline。
复盘权威仍是 CommandReplayLog + ReplayRunner。
历史权威仍是 AgentTickRecord。
协议投影权威仍是 P10 Projection DTO。
P11 只读，不参与结算、不参与决策、不参与训练。
```

## 3. P11 工程定位

P11 分三层：

```text
DebugReport 层：把 AgentHistoryProjection 转成调试报告 DTO。
ExportDraft 层：把报告/投影包装成内存级导出清单和导出片段。
Diagnostics 层：生成失败/风险/边界摘要，服务 AI 审核和研发排查。
```

P11 不做：

```text
不写本地文件。
不做 CLI。
不做 JSON 序列化器。
不做 Markdown 文件写入。
不做 HTTP server。
不做 WebSocket。
不做 H5 页面。
不做 SaveManager。
不做数据库。
不调用 AgentRuntime。
不调用 Policy。
不调用 RulePipeline。
不调用 ReplayRunner。
不计算 Reward。
不判断 Done。
不做 RL。
不做权限系统。
```

## 4. P11 最小闭环定义

### 4.1 unknown fruit 调试报告闭环

输入：

```text
P10 AgentHistoryProjection，包含一个 PipelineSucceeded / ReplayLocked / TrainingSampleDraft 的 unknown fruit 历史项。
```

流程：

```text
AgentDebugReportBuilder 从 AgentHistoryProjection 生成 AgentDebugReport。
AgentDiagnosticsBuilder 从 AgentDebugReport 生成 AgentDiagnosticsSummary。
AgentExportDraftBuilder 从 DebugReport + Diagnostics 生成 AgentExportManifest + AgentExportChunk 列表。
```

验收：

```text
Report 包含 agent_id、tick、runtime_status、decision_status、command_id、replay_lock_status、training_sample_status。
Diagnostics 标记 replay_locked_count=1。
Export manifest validateBasic 通过。
Export chunk validateBasic 通过。
不写文件。
不调用网络。
```

### 4.2 NoDecision 调试报告闭环

输入：

```text
P10 Debug 模式 AgentHistoryProjection，包含 NoDecision / ExplainedOnly / NoDecision sample。
```

验收：

```text
Report 包含 NoDecision 条目。
Report 不包含 command_id。
Report 包含 reason_keys 或 phase_keys 摘要。
Diagnostics 标记 no_decision_count=1。
Diagnostics 不把 NoDecision 判定为 Broken。
Export chunk 不包含 hidden truth。
```

### 4.3 Public 模式导出闭环

输入：

```text
P10 Public 模式 AgentHistoryProjection。
```

验收：

```text
Export chunk 不包含 phase_keys / reason_keys / warning_keys。
Export chunk 不包含 hidden truth。
Export chunk 不包含 reward_value / done bool。
```

## 5. P11 新增核心类型

### 5.1 AgentDebugReportId

文件建议：`backend/include/pathfinder/agent/debug_report.h`

```cpp
struct AgentDebugReportIdTag {};
using AgentDebugReportId = foundation::StrongId<AgentDebugReportIdTag>;
```

ID 规则：

```text
agent_debug_report_<query_id>_<item_count>
```

注意：

```text
必须提供 helper 生成。
不得手写随机 ID。
不得使用中文、空格、冒号、斜杠。
```

### 5.2 AgentDebugReportMode

```cpp
enum class AgentDebugReportMode {
    Unknown,
    Public,
    Debug,
    Training
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| Public | 公开报告 | 不含 trace 细节 |
| Debug | 调试报告 | 可含 reason/phase 摘要 |
| Training | 训练报告 | 可含训练样本摘要 |

约束：

```text
Public 模式不得包含 trace keys。
Debug / Training 模式也不得包含 hidden truth。
```

### 5.3 AgentDebugReportSeverity

```cpp
enum class AgentDebugReportSeverity {
    Unknown,
    Info,
    Warning,
    Error
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| Info | 信息 | 普通说明 |
| Warning | 警告 | 可疑但不阻塞 |
| Error | 错误 | 断链、非法或不可用 |

### 5.4 AgentDebugReportItem

```cpp
struct AgentDebugReportItem {
    std::string record_id;
    std::string agent_id;
    std::string actor_id;
    int64_t tick = 0;
    std::string runtime_status;
    std::string decision_status;
    std::optional<std::string> command_id;
    std::optional<std::string> replay_lock_status;
    std::optional<std::string> training_sample_status;

    AgentDebugReportSeverity severity = AgentDebugReportSeverity::Info;
    std::vector<std::string> summary_keys;
    std::vector<std::string> reason_keys;
    std::vector<std::string> phase_keys;
    std::vector<std::string> warning_keys;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
record_id / agent_id / actor_id 必须非空。
runtime_status / decision_status 必须非空。
severity 不能 Unknown。
Public report 下 reason_keys / phase_keys / warning_keys 必须为空。
NoDecision Debug report 应保留解释 key。
```

### 5.5 AgentDebugReport

```cpp
struct AgentDebugReport {
    AgentDebugReportId report_id;
    AgentDebugReportMode mode = AgentDebugReportMode::Unknown;
    size_t total_items = 0;
    size_t replay_locked_count = 0;
    size_t explained_only_count = 0;
    size_t broken_count = 0;
    size_t no_decision_count = 0;
    size_t skipped_count = 0;
    bool truncated = false;
    std::vector<AgentDebugReportItem> items;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
report_id 必须合法。
mode 不能 Unknown。
items.size 必须 <= total_items。
各 count 不能大于 total_items。
items 必须全部 validateBasic。
```

### 5.6 AgentDebugReportBuildRequest

```cpp
struct AgentDebugReportBuildRequest {
    protocol::AgentHistoryProjection projection;
    AgentDebugReportMode mode = AgentDebugReportMode::Public;
    bool include_trace_keys = false;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
projection 必须 validateBasic。
mode 不能 Unknown。
Public 模式 include_trace_keys 必须 false。
```

### 5.7 AgentDebugReportBuilder

```cpp
class AgentDebugReportBuilder {
public:
    foundation::Result<AgentDebugReport> build(
        const AgentDebugReportBuildRequest& request) const;
};
```

职责：

```text
从 P10 AgentHistoryProjection 构建 report。
统计 replay_locked / explained_only / broken / no_decision / skipped。
根据模式裁剪 trace keys。
不读取 AgentTickRecord 原始内部结构。
不读取 GameState。
不读取 ObjectDefinition。
不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
```

## 6. P11 Diagnostics 类型

文件建议：`backend/include/pathfinder/agent/debug_report.h`

### 6.1 AgentDiagnosticsStatus

```cpp
enum class AgentDiagnosticsStatus {
    Unknown,
    Clean,
    HasWarnings,
    HasErrors
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| Clean | 干净 | 无警告无错误 |
| HasWarnings | 有警告 | 存在警告项 |
| HasErrors | 有错误 | 存在错误项 |

### 6.2 AgentDiagnosticsIssue

```cpp
struct AgentDiagnosticsIssue {
    AgentDebugReportSeverity severity = AgentDebugReportSeverity::Unknown;
    std::string issue_key;
    std::optional<std::string> record_id;
    std::optional<std::string> agent_id;
    std::vector<std::string> detail_keys;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
severity 不能 Unknown。
issue_key 必须非空。
detail_keys 只能放 key，不放长文本或 hidden truth。
```

### 6.3 AgentDiagnosticsSummary

```cpp
struct AgentDiagnosticsSummary {
    AgentDiagnosticsStatus status = AgentDiagnosticsStatus::Unknown;
    size_t item_count = 0;
    size_t warning_count = 0;
    size_t error_count = 0;
    std::vector<AgentDiagnosticsIssue> issues;

    foundation::Result<void> validateBasic() const;
};
```

### 6.4 AgentDiagnosticsBuilder

```cpp
class AgentDiagnosticsBuilder {
public:
    foundation::Result<AgentDiagnosticsSummary> build(
        const AgentDebugReport& report) const;
};
```

诊断规则：

```text
Broken replay lock -> Error issue。
Invalid / Unknown status -> Error issue。
NoDecision 无解释 key（Debug/Training 模式） -> Warning issue。
Public 模式出现 trace keys -> Error issue。
有 Error -> status=HasErrors。
无 Error 但有 Warning -> status=HasWarnings。
否则 Clean。
```

## 7. P11 Export Draft 类型

文件建议：`backend/include/pathfinder/agent/debug_export.h`

### 7.1 AgentExportFormat

```cpp
enum class AgentExportFormat {
    Unknown,
    PlainText,
    MarkdownLike,
    ProtocolText
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| PlainText | 纯文本 | 简单行文本 |
| MarkdownLike | 类 Markdown | 内存字符串，不写 md 文件 |
| ProtocolText | 协议文本 | 面向协议调试的文本摘要 |

P11 不实现：

```text
JSON。
CSV。
二进制。
文件写入。
```

### 7.2 AgentExportChunk

```cpp
struct AgentExportChunk {
    std::string chunk_id;
    AgentExportFormat format = AgentExportFormat::Unknown;
    std::string title_key;
    std::string payload;
    size_t item_count = 0;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
chunk_id 必须合法 ID 字符串。
format 不能 Unknown。
title_key 必须非空。
payload 必须非空。
payload 不得包含 hidden truth。
payload 不得包含 reward_value / done bool。
```

### 7.3 AgentExportManifest

```cpp
struct AgentExportManifest {
    std::string manifest_id;
    AgentExportFormat format = AgentExportFormat::Unknown;
    size_t chunk_count = 0;
    size_t total_item_count = 0;
    std::vector<std::string> chunk_ids;
    std::vector<std::string> warning_keys;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
manifest_id 必须合法 ID 字符串。
chunk_count 必须等于 chunk_ids.size。
format 不能 Unknown。
P11 不包含 file_path。
P11 不包含 absolute_path。
P11 不包含 write_result。
```

### 7.4 AgentExportDraft

```cpp
struct AgentExportDraft {
    AgentExportManifest manifest;
    std::vector<AgentExportChunk> chunks;

    foundation::Result<void> validateBasic() const;
};
```

### 7.5 AgentExportDraftBuilder

```cpp
struct AgentExportDraftBuildRequest {
    AgentDebugReport report;
    std::optional<AgentDiagnosticsSummary> diagnostics;
    AgentExportFormat format = AgentExportFormat::PlainText;
    size_t max_items_per_chunk = 50;

    foundation::Result<void> validateBasic() const;
};

class AgentExportDraftBuilder {
public:
    foundation::Result<AgentExportDraft> build(
        const AgentExportDraftBuildRequest& request) const;
};
```

职责：

```text
把 report 和 diagnostics 转成内存 chunks。
按 max_items_per_chunk 分块。
生成 manifest。
不写文件。
不访问 filesystem。
不序列化 JSON。
不调用网络。
```

## 8. 文件范围

### 8.1 允许新增

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

### 8.2 允许修改

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

### 8.3 禁止修改

```text
backend/src/rules/
backend/include/pathfinder/rules/
backend/src/object/
backend/include/pathfinder/object/
backend/src/pipeline/
backend/include/pathfinder/pipeline/
backend/src/replay/
backend/include/pathfinder/replay/
backend/src/protocol/，除非 P10 类型命名阻塞且必须补 include
frontend/
任何 HTTP / WebSocket / SaveManager 目录。
```

## 9. 必读文档

P11 实现 AI 必须读取：

```text
doc/36_P11Agent本地调试报告与导出前置任务卡设计.md
context_packs/agent_p11.md
context_packs/agent_p10.md
context_packs/agent_p9.md
doc/35_P10Agent历史查询与协议投影前置任务卡设计.md
backend/dev_notes/agent.md
```

最多再读取：

```text
backend/include/pathfinder/protocol/agent_projection.h
backend/include/pathfinder/agent/history_query.h
backend/include/pathfinder/agent/training_sample.h
backend/include/pathfinder/agent/replay_lock.h
```

禁止为了 P11 一次性读取：

```text
完整规则系统。
完整对象系统。
完整 H5 / HTTP / WebSocket 设计。
完整存档系统。
```

## 10. TASK-P11-000：上下文包

任务名称：创建 P11 上下文包。

允许新增：

```text
context_packs/agent_p11.md
```

必须包含：

```text
P11 目标。
P11 与 P10/P9/P8 的关系。
DebugReport / Diagnostics / ExportDraft 三层边界。
不做文件写入 / CLI / JSON / HTTP / WebSocket / H5 / SaveManager。
文件范围。
验收命令。
```

验收命令：

```bash
rg -n "P11|DebugReport|Diagnostics|ExportDraft|不写文件|不做 CLI|不做 HTTP" context_packs/agent_p11.md
```

## 11. TASK-P11-001：实现 DebugReport 基础类型

任务名称：实现 AgentDebugReport 数据契约。

允许新增：

```text
backend/include/pathfinder/agent/debug_report.h
backend/src/agent/debug_report.cpp
backend/tests/unit/agent/agent_debug_report_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentDebugReportId。
定义 AgentDebugReportMode。
定义 AgentDebugReportSeverity。
定义 AgentDebugReportItem。
定义 AgentDebugReport。
实现 toString/fromString。
实现 validateBasic。
实现 makeAgentDebugReportId helper。
不依赖 GameState。
不依赖 ObjectDefinition。
不依赖 RulePipeline / ReplayRunner。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_report --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/debug_report.h backend/src/agent/debug_report.cpp backend/tests/unit/agent/agent_debug_report_test.cpp && exit 1 || true
```

测试要求：

```text
枚举 roundtrip 覆盖全部枚举。
合法 report item 通过。
缺 record_id 失败。
缺 agent_id 失败。
缺 runtime_status 失败。
severity Unknown 失败。
合法 report 通过。
report_id 为空失败。
mode Unknown 失败。
count 大于 total_items 失败。
items 中非法 item 失败。
```

## 12. TASK-P11-002：实现 DebugReportBuilder

任务名称：从 P10 AgentHistoryProjection 构建调试报告。

允许修改：

```text
backend/include/pathfinder/agent/debug_report.h
backend/src/agent/debug_report.cpp
backend/tests/unit/agent/agent_debug_report_test.cpp
```

实现要求：

```text
定义 AgentDebugReportBuildRequest。
实现 AgentDebugReportBuilder::build。
从 AgentHistoryProjection 复制安全摘要字段。
统计 replay_locked_count / explained_only_count / broken_count / no_decision_count / skipped_count。
Public 模式清空 reason_keys / phase_keys / warning_keys。
Debug 模式保留解释 key。
Training 模式保留训练样本状态摘要。
不读取 AgentTickRecord 原始结构。
不读取 GameState / hidden truth。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_report --output-on-failure
rg -n "AgentTickRecord|GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind|RulePipeline|ReplayRunner" backend/include/pathfinder/agent/debug_report.h backend/src/agent/debug_report.cpp backend/tests/unit/agent/agent_debug_report_test.cpp && exit 1 || true
```

测试要求：

```text
PipelineSucceeded projection -> report item 包含 command_id。
NoDecision projection -> report item 不包含 command_id。
Locked item 计入 replay_locked_count。
ExplainedOnly item 计入 explained_only_count。
Broken item 计入 broken_count 且 severity=Error。
Public 模式清空 trace keys。
Debug 模式保留 trace keys。
```

## 13. TASK-P11-003：实现 Diagnostics 类型和 Builder

任务名称：实现调试诊断摘要。

允许修改：

```text
backend/include/pathfinder/agent/debug_report.h
backend/src/agent/debug_report.cpp
backend/tests/unit/agent/agent_debug_report_test.cpp
```

实现要求：

```text
定义 AgentDiagnosticsStatus。
定义 AgentDiagnosticsIssue。
定义 AgentDiagnosticsSummary。
实现 validateBasic。
实现 AgentDiagnosticsBuilder::build。
Broken replay lock -> Error。
Unknown / Invalid status -> Error。
NoDecision 无解释 key -> Warning。
Public report 出现 trace keys -> Error。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_report --output-on-failure
```

测试要求：

```text
Clean report -> status Clean。
包含 Warning item -> HasWarnings。
包含 Error item -> HasErrors。
Broken item 生成 Error issue。
NoDecision 无解释 key 生成 Warning issue。
Public trace 泄露生成 Error issue。
```

## 14. TASK-P11-004：实现 ExportDraft 基础类型

任务名称：实现内存级导出草稿数据契约。

允许新增：

```text
backend/include/pathfinder/agent/debug_export.h
backend/src/agent/debug_export.cpp
backend/tests/unit/agent/agent_debug_export_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentExportFormat。
定义 AgentExportChunk。
定义 AgentExportManifest。
定义 AgentExportDraft。
实现 toString/fromString。
实现 validateBasic。
不包含 file_path。
不包含 absolute_path。
不包含 write_result。
不包含 JSON 类型。
不访问 filesystem。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_export --output-on-failure
rg -n "fstream|ofstream|ifstream|filesystem|file_path|absolute_path|write_result|nlohmann|json|HTTP|WebSocket|SaveManager" backend/include/pathfinder/agent/debug_export.h backend/src/agent/debug_export.cpp backend/tests/unit/agent/agent_debug_export_test.cpp && exit 1 || true
```

测试要求：

```text
枚举 roundtrip 覆盖全部枚举。
合法 chunk 通过。
chunk_id 为空失败。
format Unknown 失败。
payload 为空失败。
合法 manifest 通过。
chunk_count 与 chunk_ids.size 不一致失败。
合法 draft 通过。
manifest chunk_ids 与 chunks 对不上失败。
```

## 15. TASK-P11-005：实现 ExportDraftBuilder

任务名称：从 report + diagnostics 生成内存导出片段。

允许修改：

```text
backend/include/pathfinder/agent/debug_export.h
backend/src/agent/debug_export.cpp
backend/tests/unit/agent/agent_debug_export_test.cpp
```

实现要求：

```text
定义 AgentExportDraftBuildRequest。
实现 AgentExportDraftBuilder::build。
支持 PlainText / MarkdownLike / ProtocolText。
按 max_items_per_chunk 分块。
manifest.chunk_count 与 chunks 保持一致。
不写文件。
不访问 filesystem。
不使用 JSON。
不调用网络。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_export --output-on-failure
rg -n "fstream|ofstream|ifstream|filesystem|nlohmann|json|socket|send\(|recv\(|HTTP|WebSocket|SaveManager" backend/include/pathfinder/agent/debug_export.h backend/src/agent/debug_export.cpp backend/tests/unit/agent/agent_debug_export_test.cpp && exit 1 || true
```

测试要求：

```text
PlainText 导出成功。
MarkdownLike 导出成功。
ProtocolText 导出成功。
max_items_per_chunk=0 失败。
分块数量正确。
manifest warning_keys 包含 diagnostics 警告摘要。
payload 不包含 hidden truth 关键词。
```

## 16. TASK-P11-006：unknown fruit 调试报告导出集成测试

任务名称：证明 P10 成功历史投影可以生成调试报告和导出草稿。

允许新增：

```text
backend/tests/integration/p11/agent_debug_report_export_flow_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 构造 P10 AgentHistoryProjection，包含 ReplayLocked unknown fruit item。
2. AgentDebugReportBuilder 生成 Training 模式 report。
3. AgentDiagnosticsBuilder 生成 diagnostics。
4. AgentExportDraftBuilder 生成 MarkdownLike draft。
5. 断言 replay_locked_count=1。
6. 断言 diagnostics.status=Clean。
7. 断言 manifest/chunks validateBasic 通过。
8. 断言不写文件、不访问路径。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p11_agent_debug_report_export_flow --output-on-failure
```

## 17. TASK-P11-007：NoDecision 调试报告集成测试

任务名称：证明 NoDecision 历史可形成可解释调试报告。

允许新增：

```text
backend/tests/integration/p11/agent_no_decision_debug_report_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 构造 P10 Debug 模式 AgentHistoryProjection，包含 NoDecision item。
2. AgentDebugReportBuilder 生成 Debug report。
3. 断言 no_decision_count=1。
4. 断言 item.command_id 为空。
5. 断言 reason_keys 或 phase_keys 存在。
6. AgentDiagnosticsBuilder 不产生 Error。
7. Public 模式再生成 report，断言 trace keys 清空。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p11_agent_no_decision_debug_report --output-on-failure
```

## 18. TASK-P11-008：边界审计和开发笔记

任务名称：更新开发笔记并执行 P11 审计。

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P11Agent本地调试报告与导出前置审核报告.md
```

审计要求：

```text
确认 P11 不读取 AgentTickRecord 原始结构。
确认 P11 不读取 GameState / ObjectDefinition / hidden truth。
确认 P11 不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
确认 P11 不写文件 / 不访问 filesystem。
确认 P11 不做 JSON / HTTP / WebSocket / H5 / SaveManager。
确认 P10/P9/P8 回归测试通过。
确认 P11 集成测试通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_debug_report|agent_debug_export|p11" --output-on-failure
ctest --test-dir build/backend -R "agent|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|AgentTickRecord" backend/include/pathfinder/agent/debug_report.h backend/src/agent/debug_report.cpp backend/include/pathfinder/agent/debug_export.h backend/src/agent/debug_export.cpp && exit 1 || true
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/debug_report.h backend/src/agent/debug_report.cpp backend/include/pathfinder/agent/debug_export.h backend/src/agent/debug_export.cpp backend/tests/unit/agent/agent_debug_report_test.cpp backend/tests/unit/agent/agent_debug_export_test.cpp backend/tests/integration/p11 && exit 1 || true
rg -n "fstream|ofstream|ifstream|filesystem|file_path|absolute_path|write_result|nlohmann|json|socket|send\(|recv\(|HTTP|WebSocket|SaveManager" backend/include/pathfinder/agent/debug_report.h backend/src/agent/debug_report.cpp backend/include/pathfinder/agent/debug_export.h backend/src/agent/debug_export.cpp backend/tests/integration/p11 && exit 1 || true
rg -n "reward_value|done =|is_done|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" backend/include/pathfinder/agent backend/src/agent backend/tests/integration/p11 && exit 1 || true
find backend -type d \( -path '*/frontend*' -o -path '*/http*' -o -path '*/websocket*' -o -path '*/save*' \) -print
```

审核报告必须包含：

```text
构建结果。
P11 定向测试结果。
P8-P10 回归测试结果。
全量测试结果。
边界扫描结果。
是否允许进入 P12。
```

## 19. P11 设计风险和处理

### 19.1 风险：提前写文件或做 CLI

处理：

```text
P11 只生成内存 AgentExportDraft。
禁止 file_path / filesystem / fstream。
CLI 留到 P12+。
```

### 19.2 风险：报告泄露 hidden truth

处理：

```text
P11 只读取 P10 Projection。
不读取 GameState / ObjectDefinition。
边界扫描禁止 hidden truth 字段。
```

### 19.3 风险：Public 报告泄露 trace

处理：

```text
Public 模式必须清空 reason_keys / phase_keys / warning_keys。
Diagnostics 遇到 Public trace 泄露直接 Error。
```

### 19.4 风险：导出草稿被误当最终导出系统

处理：

```text
P11 不写文件。
P11 不定义文件路径。
P11 不定义 JSON 格式。
P11 只产内存 chunk。
```

## 20. P11 完成后的系统能力

P11 完成后，系统具备：

```text
可以把 P10 AgentHistoryProjection 转成 AgentDebugReport。
可以统计 replay locked / explained only / broken / no decision / skipped。
可以生成 AgentDiagnosticsSummary。
可以生成内存级 AgentExportDraft。
可以为未来 CLI、文件导出、H5 调试面板提供稳定报告结构。
```

P11 完成后仍不具备：

```text
CLI。
文件写入。
JSON 导出。
HTTP / WebSocket。
H5 页面。
SaveManager。
权限系统。
```

## 21. P11 之后建议

P11 之后建议进入 P12：

```text
P12：本地 CLI 或文件导出实现。
```

P12 可选方向：

```text
方案 A：实现本地 CLI，把 P11 ExportDraft 写成 .txt / .md 文件。
方案 B：实现 JSON 序列化草案，为未来 HTTP/H5 做准备。
```

建议优先方案 A，因为它能先帮助研发和 AI 审核使用报告，不需要引入网络复杂度。
