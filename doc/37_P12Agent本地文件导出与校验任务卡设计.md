# P12 Agent本地文件导出与校验任务卡设计

## 1. P12 要解决什么问题

P11 已经能把 P10 的 AgentHistoryProjection 转成：

```text
AgentDebugReport：结构化调试报告。
AgentDiagnosticsSummary：诊断摘要。
AgentExportDraft：内存级导出清单和 chunks。
```

但 P11 明确不写文件。P12 要解决的是：

```text
研发和 AI 审核需要把 P11 的内存导出草稿安全、稳定、可校验地写成本地 .txt / .md 文件。
```

P12 的一句话目标：

```text
把 P11 AgentExportDraft 落盘为受控目录下的本地调试导出文件，并提供写入计划、写入结果和校验结果。
```

P12 是“本地文件导出层”，不是 CLI，不是 JSON，不是 HTTP，不是 H5，不是 SaveManager。

## 2. P12 与前置阶段的关系

P12 依赖：

```text
P10：AgentHistoryProjection / ProtocolEnvelope。
P11：AgentDebugReport / AgentDiagnosticsSummary / AgentExportDraft。
```

P12 产出：

```text
AgentExportWritePlan：写入计划，描述将写哪些文件。
AgentExportWriteResult：写入结果，描述实际写入状态。
AgentExportVerificationReport：导出校验报告，确认文件数量、大小、名称、内容边界。
AgentLocalExportService：本地文件导出服务。
```

P12 不改变：

```text
规则权威仍是 RulePipeline。
复盘权威仍是 CommandReplayLog + ReplayRunner。
历史权威仍是 AgentTickRecord。
投影权威仍是 P10 Projection。
报告和导出内容权威仍是 P11 AgentExportDraft。
P12 只负责受控落盘与校验，不生成新的游戏结算结果。
```

## 3. P12 工程定位

P12 分三层：

```text
PathPolicy 层：校验导出根目录、文件名、扩展名、安全路径。
WritePlan 层：把 AgentExportDraft 转成确定性的文件写入计划。
LocalExport 层：执行写入并生成写入结果和校验报告。
```

P12 不做：

```text
不做命令行 CLI。
不解析 argv。
不做 JSON 序列化。
不做 HTTP server。
不做 WebSocket。
不做 H5 页面。
不做 SaveManager。
不做数据库。
不调用 AgentRuntime。
不调用 Policy。
不调用 RulePipeline。
不调用 ReplayRunner。
不读取 GameState。
不读取 ObjectDefinition。
不计算 Reward。
不判断 Done。
不做 RL。
不做 LLM。
不做权限系统。
不写任意绝对路径。
不允许路径穿越。
```

## 4. P12 最小闭环定义

### 4.1 MarkdownLike 导出闭环

输入：

```text
P11 AgentExportDraft，format=MarkdownLike，包含 manifest 和 1 个以上 chunks。
导出根目录为测试临时目录下的 p12_agent_export。
```

流程：

```text
AgentExportPathPolicy 校验 root_dir 和文件名。
AgentExportWritePlanner 从 AgentExportDraft 生成 AgentExportWritePlan。
AgentLocalExportService 按 plan 写入 manifest 文件和 chunk 文件。
AgentExportVerifier 校验写入结果和文件内容边界。
```

验收：

```text
写入文件数量正确。
manifest 文件存在。
chunk 文件存在。
文件名稳定且合法。
verification status=Passed。
文件内容不包含 hidden truth 关键词。
不调用网络、不调用规则、不调用 AgentRuntime。
```

### 4.2 Public 模式安全导出闭环

输入：

```text
P11 Public 模式 AgentExportDraft。
```

验收：

```text
导出文件中不包含 phase_keys。
导出文件中不包含 reason_keys。
导出文件中不包含 warning_keys。
导出文件中不包含 hidden truth。
导出文件中不包含 reward_value / done bool。
```

### 4.3 路径安全失败闭环

输入：

```text
root_dir 为空、文件名包含 ../、文件名包含斜杠、扩展名不匹配、chunk payload 超限。
```

验收：

```text
validateBasic 或 write 返回错误。
不得写入任何文件。
错误码稳定。
错误 message_key 稳定。
```

## 5. P12 新增核心类型

### 5.1 AgentExportFileId

文件建议：`backend/include/pathfinder/agent/local_export.h`

```cpp
struct AgentExportFileIdTag {};
using AgentExportFileId = foundation::StrongId<AgentExportFileIdTag>;
```

ID 规则：

```text
agent_export_file_<export_id>_<kind>_<index>
```

中文说明：

```text
AgentExportFileId = Agent 导出文件 ID。
用于标识一次导出中的某个逻辑文件，不等同于真实文件路径。
```

注意：

```text
必须提供 helper 生成。
不得手写随机 ID。
不得使用中文、空格、冒号、斜杠。
不得把完整路径放入 ID。
```

### 5.2 AgentExportWriteId

```cpp
struct AgentExportWriteIdTag {};
using AgentExportWriteId = foundation::StrongId<AgentExportWriteIdTag>;
```

ID 规则：

```text
agent_export_write_<manifest_export_id>_<file_count>
```

中文说明：

```text
AgentExportWriteId = Agent 导出写入任务 ID。
用于标识一次本地导出写入任务。
```

### 5.3 AgentExportVerifyId

```cpp
struct AgentExportVerifyIdTag {};
using AgentExportVerifyId = foundation::StrongId<AgentExportVerifyIdTag>;
```

ID 规则：

```text
agent_export_verify_<write_id>
```

中文说明：

```text
AgentExportVerifyId = Agent 导出校验任务 ID。
用于标识一次导出结果校验。
```

### 5.4 AgentExportFileKind

```cpp
enum class AgentExportFileKind {
    Unknown,
    Manifest,
    Chunk,
    Diagnostics
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| Manifest | 清单文件 | 描述导出的总体信息 |
| Chunk | 分片文件 | 保存 P11 AgentExportChunk payload |
| Diagnostics | 诊断文件 | 可选保存诊断摘要 |

约束：

```text
Unknown 不能作为有效文件类型。
P12 最小实现必须支持 Manifest 和 Chunk。
Diagnostics 可先作为枚举保留，但不强制写独立文件。
```

### 5.5 AgentExportFileExtension

```cpp
enum class AgentExportFileExtension {
    Unknown,
    Txt,
    Md
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| Txt | 文本文件 | PlainText / ProtocolText 导出 |
| Md | Markdown 文件 | MarkdownLike 导出 |

映射规则：

```text
PlainText -> Txt
ProtocolText -> Txt
MarkdownLike -> Md
```

注意：

```text
P12 不设计 Json 扩展名。
JSON 留到后续专门序列化阶段。
```

### 5.6 AgentExportWriteStatus

```cpp
enum class AgentExportWriteStatus {
    Unknown,
    Planned,
    Written,
    Failed,
    Skipped
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| Planned | 已计划 | 已生成写入计划，尚未执行 |
| Written | 已写入 | 文件成功写入 |
| Failed | 写入失败 | 文件写入失败 |
| Skipped | 已跳过 | 因选项或空内容跳过 |

### 5.7 AgentExportVerificationStatus

```cpp
enum class AgentExportVerificationStatus {
    Unknown,
    Passed,
    Failed,
    Warning
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值 |
| Passed | 通过 | 导出校验通过 |
| Failed | 失败 | 导出文件缺失、越界或内容非法 |
| Warning | 警告 | 可用但存在非阻塞风险 |

## 6. P12 数据契约

### 6.1 AgentExportPathPolicy

```cpp
struct AgentExportPathPolicy {
    std::string root_dir;
    bool allow_create_directories = true;
    bool allow_overwrite = false;
    size_t max_file_count = 64;
    size_t max_file_bytes = 1024 * 1024;
    size_t max_total_bytes = 8 * 1024 * 1024;

    foundation::Result<void> validateBasic() const;
};
```

职责：

```text
定义本地导出的路径和大小安全边界。
```

约束：

```text
root_dir 不能为空。
root_dir 不允许包含 ".."。
max_file_count 必须 >0 且 <=1024。
max_file_bytes 必须 >0。
max_total_bytes 必须 >= max_file_bytes。
P12 测试只能写入 build/backend 或 /tmp 下的测试目录。
```

### 6.2 AgentExportFilePlan

```cpp
struct AgentExportFilePlan {
    AgentExportFileId file_id;
    AgentExportFileKind kind = AgentExportFileKind::Unknown;
    AgentExportFileExtension extension = AgentExportFileExtension::Unknown;
    std::string relative_path;
    std::string content;
    size_t byte_size = 0;
    bool overwrite = false;

    foundation::Result<void> validateBasic() const;
};
```

职责：

```text
描述一个逻辑文件将以什么相对路径、什么内容写入。
```

约束：

```text
file_id 必须合法。
kind 不能 Unknown。
extension 不能 Unknown。
relative_path 必须非空。
relative_path 必须是相对路径。
relative_path 不允许包含 ".."。
relative_path 不允许以 / 开头。
relative_path 不允许包含反斜杠。
relative_path 只能使用字母、数字、下划线、中划线、点和斜杠。
content 可以为空，但 Manifest 不允许空 content。
byte_size 必须等于 content.size()。
```

### 6.3 AgentExportWritePlan

```cpp
struct AgentExportWritePlan {
    AgentExportWriteId write_id;
    std::string export_id;
    AgentExportPathPolicy path_policy;
    std::vector<AgentExportFilePlan> files;
    size_t total_bytes = 0;

    foundation::Result<void> validateBasic() const;
};
```

职责：

```text
描述一次导出写入任务的完整计划。
```

约束：

```text
write_id 必须合法。
export_id 必须非空。
files 必须非空。
files.size <= path_policy.max_file_count。
total_bytes 必须等于所有 file.byte_size 之和。
total_bytes <= path_policy.max_total_bytes。
每个 file.byte_size <= path_policy.max_file_bytes。
relative_path 不允许重复。
必须包含一个 Manifest 文件。
```

### 6.4 AgentExportFileWriteResult

```cpp
struct AgentExportFileWriteResult {
    AgentExportFileId file_id;
    AgentExportFileKind kind = AgentExportFileKind::Unknown;
    AgentExportWriteStatus status = AgentExportWriteStatus::Unknown;
    std::string relative_path;
    size_t byte_size = 0;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    foundation::Result<void> validateBasic() const;
};
```

职责：

```text
描述某一个文件的写入结果。
```

约束：

```text
file_id 必须合法。
kind 不能 Unknown。
status 不能 Unknown。
relative_path 必须合法。
Written 状态下 error_keys 必须为空。
Failed 状态下 error_keys 必须非空。
```

### 6.5 AgentExportWriteResult

```cpp
struct AgentExportWriteResult {
    AgentExportWriteId write_id;
    AgentExportWriteStatus status = AgentExportWriteStatus::Unknown;
    std::string root_dir;
    size_t planned_file_count = 0;
    size_t written_file_count = 0;
    size_t total_bytes = 0;
    std::vector<AgentExportFileWriteResult> files;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    foundation::Result<void> validateBasic() const;
};
```

职责：

```text
描述一次导出写入的整体结果。
```

约束：

```text
write_id 必须合法。
status 不能 Unknown。
root_dir 必须非空。
planned_file_count 必须等于 plan.files.size。
written_file_count 必须等于 status=Written 的文件数。
Written 状态下 error_keys 必须为空。
Failed 状态下 error_keys 必须非空。
```

### 6.6 AgentExportVerificationReport

```cpp
struct AgentExportVerificationReport {
    AgentExportVerifyId verify_id;
    AgentExportWriteId write_id;
    AgentExportVerificationStatus status = AgentExportVerificationStatus::Unknown;
    size_t expected_file_count = 0;
    size_t actual_file_count = 0;
    size_t expected_total_bytes = 0;
    size_t actual_total_bytes = 0;
    std::vector<std::string> checked_relative_paths;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    foundation::Result<void> validateBasic() const;
};
```

职责：

```text
校验本地导出结果是否和写入计划一致。
```

约束：

```text
verify_id 必须合法。
write_id 必须合法。
status 不能 Unknown。
Passed 状态下 expected_file_count == actual_file_count。
Passed 状态下 expected_total_bytes == actual_total_bytes。
Passed 状态下 error_keys 必须为空。
Failed 状态下 error_keys 必须非空。
checked_relative_paths 不允许重复。
```

## 7. P12 服务接口

### 7.1 AgentExportWritePlanner

```cpp
struct AgentExportWritePlanRequest {
    AgentExportDraft draft;
    AgentExportPathPolicy path_policy;
    std::string base_name = "agent_export";
    bool include_manifest_file = true;

    foundation::Result<void> validateBasic() const;
};

class AgentExportWritePlanner {
public:
    foundation::Result<AgentExportWritePlan> buildPlan(
        const AgentExportWritePlanRequest& request) const;
};
```

职责：

```text
把 P11 AgentExportDraft 转成确定性的 AgentExportWritePlan。
```

文件命名规则：

```text
<safe_base_name>_manifest.<ext>
<safe_base_name>_chunk_<index>.<ext>
```

base_name 约束：

```text
不能为空。
不允许包含 /、\、..、空格、冒号。
只允许字母、数字、下划线、中划线。
```

### 7.2 AgentLocalExportService

```cpp
class AgentLocalExportService {
public:
    foundation::Result<AgentExportWriteResult> write(
        const AgentExportWritePlan& plan) const;
};
```

职责：

```text
执行本地文件写入。
```

写入规则：

```text
写入前必须 validateBasic plan。
如果 root_dir 不存在且 allow_create_directories=true，则创建目录。
如果 root_dir 不存在且 allow_create_directories=false，则失败。
如果目标文件存在且 allow_overwrite=false，则失败。
只能写入 root_dir 下的 relative_path。
任何单文件失败时，整体 status=Failed。
失败时不得继续写后续文件。
```

注意：

```text
P12 允许使用 std::filesystem 和 std::ofstream。
这是 P12 和 P11 的明确差异。
但 P12 不允许网络、JSON、CLI、SaveManager。
```

### 7.3 AgentExportVerifier

```cpp
struct AgentExportVerifyRequest {
    AgentExportWritePlan plan;
    AgentExportWriteResult write_result;
    bool scan_content_for_forbidden_terms = true;

    foundation::Result<void> validateBasic() const;
};

class AgentExportVerifier {
public:
    foundation::Result<AgentExportVerificationReport> verify(
        const AgentExportVerifyRequest& request) const;
};
```

职责：

```text
确认文件确实存在、大小一致、数量一致、路径安全、内容不包含禁止词。
```

禁止词最小集合：

```text
GameState
ObjectDefinition
edible_profile
hunger_delta
health_delta
effect_kind
reward_value
is_done
done =
```

说明：

```text
禁止词扫描是本地导出安全兜底，不替代 P10/P11 的投影边界。
```

## 8. 文件范围

### 8.1 允许新增

```text
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
backend/tests/integration/p12/agent_local_export_flow_test.cpp
backend/tests/integration/p12/agent_local_export_security_test.cpp
context_packs/agent_p12.md
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
backend/src/protocol/
backend/include/pathfinder/protocol/
frontend/
任何 HTTP / WebSocket / SaveManager 目录。
```

例外：

```text
如果 CMake 需要链接 P11 agent debug report target，可以只修改 CMake，不修改 P11 类型语义。
```

## 9. 必读文档

P12 实现 AI 必须读取：

```text
doc/37_P12Agent本地文件导出与校验任务卡设计.md
context_packs/agent_p12.md
context_packs/agent_p11.md
doc/36_P11Agent本地调试报告与导出前置任务卡设计.md 的 ExportDraft 片段
backend/dev_notes/agent.md
```

最多再读取：

```text
backend/include/pathfinder/agent/debug_report.h
backend/src/agent/debug_report.cpp
backend/include/pathfinder/foundation/result.h
backend/include/pathfinder/foundation/id.h
backend/tests/unit/agent/agent_debug_report_test.cpp
```

禁止为了 P12 一次性读取：

```text
01-24 全部文档。
完整规则系统。
完整对象系统。
完整协议系统。
完整 H5 或 SaveManager 设计。
```

## 10. P12 任务拆分

### 10.1 TASK-P12-000：创建上下文包

任务名称：创建 P12 AI 实现上下文包。

允许新增：

```text
context_packs/agent_p12.md
```

内容必须包含：

```text
P12 目标。
P12 与 P11 的关系。
P12 允许使用 filesystem/ofstream。
P12 仍禁止 CLI/JSON/HTTP/H5/SaveManager。
文件范围。
验收命令。
边界扫描命令。
```

验收命令：

```bash
rg -n "P12|LocalExport|filesystem|ofstream|不做 CLI|不做 JSON|不做 HTTP" context_packs/agent_p12.md
```

### 10.2 TASK-P12-001：实现本地导出基础类型

任务名称：实现 Agent local export 数据契约。

允许新增：

```text
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentExportFileId / AgentExportWriteId / AgentExportVerifyId。
定义 AgentExportFileKind。
定义 AgentExportFileExtension。
定义 AgentExportWriteStatus。
定义 AgentExportVerificationStatus。
实现 toString / fromString roundtrip。
实现 ID helper。
实现 AgentExportPathPolicy。
实现 AgentExportFilePlan。
实现 AgentExportWritePlan。
实现 AgentExportFileWriteResult。
实现 AgentExportWriteResult。
实现 AgentExportVerificationReport。
实现 validateBasic。
```

测试要求：

```text
所有枚举 roundtrip。
Unknown fromString 允许解析但 validateBasic 不允许作为有效状态。
ID helper 结果稳定。
空 root_dir 失败。
非法 relative_path 失败。
byte_size 与 content.size 不一致失败。
重复 relative_path 失败。
缺 Manifest 失败。
超过 max_file_count 失败。
超过 max_file_bytes 失败。
超过 max_total_bytes 失败。
Written result 带 error_keys 失败。
Failed result 无 error_keys 失败。
Passed verification 数量/大小不一致失败。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_local_export --output-on-failure
```

### 10.3 TASK-P12-002：实现写入计划构建器

任务名称：实现 AgentExportWritePlanner。

允许修改：

```text
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
```

实现要求：

```text
定义 AgentExportWritePlanRequest。
实现 AgentExportWritePlanner::buildPlan。
根据 AgentExportDraft format 选择 Txt/Md 扩展名。
生成 manifest 文件计划。
生成 chunk 文件计划。
计算 byte_size 和 total_bytes。
校验 base_name 安全。
输出稳定 relative_path。
```

测试要求：

```text
PlainText -> .txt。
ProtocolText -> .txt。
MarkdownLike -> .md。
manifest 文件名稳定。
chunk 文件名按 index 稳定。
base_name 包含 ../ 失败。
base_name 包含空格失败。
base_name 包含冒号失败。
空 draft 失败。
超出 path_policy 限制失败。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_local_export_planner --output-on-failure
```

### 10.4 TASK-P12-003：实现本地写入服务

任务名称：实现 AgentLocalExportService。

允许修改：

```text
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
```

实现要求：

```text
实现 AgentLocalExportService::write。
使用 std::filesystem 创建目录。
使用 std::ofstream 写文件。
写入前校验 plan。
不允许覆盖时遇到已存在文件必须失败。
任一文件失败时整体 Failed。
失败时记录稳定 error_key。
成功时 status=Written。
```

测试要求：

```text
root_dir 不存在且 allow_create_directories=true 时可创建并写入。
root_dir 不存在且 allow_create_directories=false 时失败。
目标文件已存在且 allow_overwrite=false 时失败。
目标文件已存在且 allow_overwrite=true 时成功覆盖。
成功结果 written_file_count 正确。
失败结果 error_keys 非空。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_local_export_write --output-on-failure
```

### 10.5 TASK-P12-004：实现导出校验器

任务名称：实现 AgentExportVerifier。

允许修改：

```text
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
```

实现要求：

```text
定义 AgentExportVerifyRequest。
实现 AgentExportVerifier::verify。
检查文件是否存在。
检查实际文件数量。
检查实际文件大小。
检查 relative_path 未越界。
可选扫描禁止词。
输出 AgentExportVerificationReport。
```

测试要求：

```text
完整写入后 verify Passed。
缺文件 verify Failed。
文件大小不一致 verify Failed。
文件内容包含 hidden truth 关键词 verify Failed。
scan_content_for_forbidden_terms=false 时不扫描内容。
checked_relative_paths 不重复。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_local_export_verify --output-on-failure
```

### 10.6 TASK-P12-005：MarkdownLike 本地导出集成测试

任务名称：证明 P11 MarkdownLike ExportDraft 可以安全落盘。

允许新增：

```text
backend/tests/integration/p12/agent_local_export_flow_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 构造 P11 AgentDebugReport。
2. 构造 AgentDiagnosticsSummary。
3. AgentExportDraftBuilder 生成 MarkdownLike draft。
4. AgentExportWritePlanner 生成 plan。
5. AgentLocalExportService 写入测试目录。
6. AgentExportVerifier 校验。
7. 断言 verification status=Passed。
8. 断言 manifest/chunk 文件存在。
9. 断言文件内容不包含 hidden truth。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p12_agent_local_export_flow --output-on-failure
```

### 10.7 TASK-P12-006：路径安全集成测试

任务名称：证明 P12 不允许路径穿越和越界写入。

允许新增：

```text
backend/tests/integration/p12/agent_local_export_security_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 构造正常 AgentExportDraft。
2. 使用非法 base_name：../escape。
3. 断言 buildPlan 失败。
4. 手工构造非法 relative_path：../escape.txt。
5. 断言 plan validateBasic 失败。
6. 使用 allow_overwrite=false 写已存在文件。
7. 断言 write 失败且 error_keys 非空。
8. 确认 root_dir 外没有新文件。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p12_agent_local_export_security --output-on-failure
```

### 10.8 TASK-P12-007：边界审计和开发笔记

任务名称：更新开发笔记并执行 P12 审计。

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P12Agent本地文件导出与校验审核报告.md
```

审计要求：

```text
确认 P12 只消费 P11 AgentExportDraft，不读取 P10/P8 原始内部结构。
确认 P12 不读取 GameState / ObjectDefinition / hidden truth。
确认 P12 不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
确认 P12 不做 JSON / HTTP / WebSocket / H5 / SaveManager。
确认 P12 文件写入仅限 root_dir + safe relative_path。
确认 P12 路径穿越测试通过。
确认 P12/P11/P10/P9/P8 回归测试通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_local_export|p12" --output-on-failure
ctest --test-dir build/backend -R "agent|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|AgentTickRecord|AgentHistoryProjection" backend/include/pathfinder/agent/local_export.h backend/src/agent/local_export.cpp backend/tests/integration/p12 && exit 1 || true
rg -n "GameState|ObjectDefinition|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/local_export.h backend/src/agent/local_export.cpp backend/tests/unit/agent/agent_local_export_test.cpp backend/tests/integration/p12 && exit 1 || true
rg -n "httplib|asio|WebSocket|HTTP server|SaveManager|nlohmann|json|socket|send\(|recv\(|argc|argv" backend/include/pathfinder/agent/local_export.h backend/src/agent/local_export.cpp backend/tests/integration/p12 && exit 1 || true
rg -n "reward_value|done =|is_done|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" backend/include/pathfinder/agent/local_export.h backend/src/agent/local_export.cpp backend/tests/integration/p12 && exit 1 || true
```

审核报告必须包含：

```text
构建结果。
P12 定向测试结果。
P8-P11 回归测试结果。
全量测试结果。
路径安全测试结果。
边界扫描结果。
是否允许进入 P13。
```

## 11. P12 设计风险和处理

### 11.1 风险：变成任意文件写入器

处理：

```text
只允许 root_dir + safe relative_path。
relative_path 不允许 ..、绝对路径、反斜杠。
base_name 严格白名单。
测试必须覆盖路径穿越。
```

### 11.2 风险：提前做 CLI 导致范围膨胀

处理：

```text
P12 不解析 argv。
P12 只提供 C++ service。
CLI 留给 P13 或更后阶段。
```

### 11.3 风险：提前做 JSON 导出

处理：

```text
P12 只支持 Txt / Md。
JSON 需要协议序列化设计，不能塞进本阶段。
```

### 11.4 风险：文件内容泄露隐藏真相

处理：

```text
P12 只消费 P11 AgentExportDraft。
Verifier 增加 forbidden terms 扫描。
边界扫描覆盖 hidden truth 关键词。
```

### 11.5 风险：测试污染工作区

处理：

```text
测试目录必须使用 build/backend 下的 p12 临时目录或 /tmp。
测试开始前清理自己的 p12 测试目录。
不得删除非 p12 测试目录。
```

## 12. P12 完成后的系统能力

P12 完成后，系统具备：

```text
可以把 P11 AgentExportDraft 写成本地 .txt / .md 文件。
可以生成确定性的写入计划。
可以校验导出文件数量、大小、路径和内容边界。
可以阻止路径穿越和非法覆盖。
可以为研发和 AI 审核提供真实文件附件。
```

P12 完成后仍不具备：

```text
CLI。
JSON 导出。
HTTP / WebSocket。
H5 页面。
SaveManager。
数据库。
权限系统。
RL 训练运行。
LLM 接入。
```

## 13. P12 之后建议

P12 之后建议进入 P13：

```text
P13：本地调试 CLI 前置，把 P10/P11/P12 组合成可手动调用的开发工具。
```

P13 可做：

```text
命令行参数解析。
选择输入样例或测试 fixture。
调用 P11 构建导出草稿。
调用 P12 写入本地文件。
输出简短控制台结果。
```

P13 仍应避免：

```text
HTTP / WebSocket。
H5。
SaveManager。
JSON 协议层。
训练算法。
```
