# P14 Agent 调试输入适配前置任务卡设计

## 1. 阶段定位

P14 不是重新设计 Agent，也不是进入认知、记忆、学习主线。

P14 的唯一定位是：

```text
在 P13 本地调试 CLI 之后，定义一层安全输入适配层，让 CLI/调试工具未来可以接入“真实但安全”的 Agent 调试数据，而不直接读取 GameState、真实存档内部结构、隐藏真相或原始运行记录。
```

P14 承接关系：

```text
P10：AgentHistoryProjection，提供安全历史投影。
P11：AgentDebugReport / AgentDiagnosticsSummary，提供调试报告与诊断摘要。
P12：AgentExportDraft / AgentLocalExportService，提供本地导出草稿与文件写入校验。
P13：pathfinder_agent_debug_cli，只用内置 fixture 跑通本地调试导出。
P14：AgentDebugInputAdapter，把 fixture / 安全投影 / 安全报告统一成可被 P13/P11/P12 使用的安全输入包。
```

P14 完成后，才进入 P15 认知系统正式化。

## 2. 要解决的问题

P13 的输入只有内置 fixture，安全但不够接近真实调试场景。

后续开发会自然想让 CLI 接入真实数据，如果没有 P14，容易出现以下错误：

```text
CLI 直接读 SaveGame。
CLI 直接读 GameState。
CLI 直接读 AgentTickRecord。
CLI 直接读 ObjectDefinition。
CLI 自己解析 JSON。
CLI 暴露 edible_profile / hunger_delta / health_delta / effect_kind 等隐藏真相。
CLI 绕过 P10/P11/P12，形成第二套调试管线。
```

P14 要提前把这些入口封住，规定：真实数据必须先变成安全 DTO，再交给调试 CLI。

## 3. 本阶段目标

P14 的目标：

```text
实现 Agent 调试输入适配层，定义安全输入包、输入来源、输入清单、输入校验报告和适配器，让 P13 CLI 后续可以消费经过验证的 P10/P11/P12 安全 DTO。
```

更具体地说：

```text
1. 定义 AgentDebugInputSource，描述输入来自哪里。
2. 定义 AgentDebugInputManifest，描述输入包元信息和能力。
3. 定义 AgentDebugInputValidationReport，描述输入校验结果。
4. 定义 AgentDebugInputBundle，作为适配后的标准安全调试输入。
5. 定义 AgentDebugInputValidator，专门做安全校验。
6. 定义 AgentDebugInputAdapter，把安全输入转成 AgentDebugReport / AgentDiagnosticsSummary / 可选导出草稿。
7. 让 P13 fixture 路径未来可以通过同一层输入适配，而不是继续散落在 CLI 内部。
```

## 4. 本阶段不做什么

P14 明确不做：

```text
不做 JSON 解析。
不做 SaveManager。
不读取真实存档。
不读取 GameState。
不读取 ObjectDefinition。
不读取 AgentTickRecord 原始记录。
不调用 AgentRuntime。
不调用 Policy。
不调用 RulePipeline。
不调用 ReplayRunner。
不计算 Reward。
不判断 Done。
不实现 RL。
不实现 LLM。
不实现 H5 / HTTP / WebSocket。
不实现认知系统正式化。
不实现记忆系统。
不实现知识传播。
```

P14 只能消费已经安全化的数据：

```text
P10 AgentHistoryProjection。
P11 AgentDebugReport / AgentDiagnosticsSummary。
P12 AgentExportDraft / AgentExportManifest。
P13 内置 fixture bundle。
测试专用内存输入包。
```

## 5. 设计原则

### 5.1 输入先校验，再适配

任何输入都必须先经过 `AgentDebugInputValidator`。

禁止流程：

```text
input -> adapter -> report -> export
```

正确流程：

```text
input -> validator -> validation_report -> adapter -> normalized_bundle -> P11/P12/P13
```

### 5.2 调试输入只认安全 DTO

P14 不负责把真实世界对象变安全。

真实对象到安全 DTO 的转换属于更上游模块，必须先经过 P10/P11/P12 已定义的安全投影链路。

P14 只负责判断这些 DTO 是否能进入调试工具。

### 5.3 适配层不拥有规则权威

P14 不改变游戏状态，不产生状态变更，不发命令，不执行业务规则。

规则权威仍然是：

```text
CommandEnvelope -> RulePipeline -> StateChange/Event/Cognition
```

Agent 调试输入只是只读诊断材料。

### 5.4 不从导出文本反推报告

如果输入是 P12 的 `AgentExportDraft`，P14 只能验证它是安全导出草稿，不能从文本 payload 反向推导 `AgentDebugReport`。

原因：导出文本是展示结果，不是结构化真相。

### 5.5 隐藏真相宁可误杀，不可放过

出现以下 key / 字段 / 标签，默认拒绝输入：

```text
edible_profile
hunger_delta
health_delta
effect_kind
ObjectDefinition
GameState
AgentTickRecord
reward_value
done =
is_done
```

如果未来确实需要在测试中出现这些字符串，只允许出现在安全扫描测试的黑名单声明里，不能出现在业务 DTO 的 payload、summary_keys、reason_keys、phase_keys、warning_keys 或 source_label 里。

## 6. 核心类型设计

### 6.1 新增文件

```text
backend/include/pathfinder/agent/debug_input.h
backend/src/agent/debug_input.cpp
backend/tests/unit/agent/agent_debug_input_test.cpp
backend/tests/integration/p14/agent_debug_input_flow_test.cpp
backend/tests/integration/p14/agent_debug_input_security_test.cpp
context_packs/agent_p14.md
```

### 6.2 允许修改文件

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
```

修改 P13 文件的限制：

```text
只能把 fixture 输入接入 P14 adapter。
不能改 CLI 对外语义。
不能新增 JSON / save / HTTP / websocket 参数。
不能破坏 P13 已有 fixture 行为。
```

### 6.3 禁止修改文件

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
任何 SaveManager / HTTP / WebSocket 相关目录
```

注意：P14 可以 include P10/P11/P12 的头文件，但不能修改它们的语义。

## 7. 枚举设计

### 7.1 AgentDebugInputKind

中文：Agent 调试输入类型。

用途：说明输入包携带的是哪一种安全数据。

```cpp
enum class AgentDebugInputKind {
    Unknown,
    BuiltinFixtureBundle,
    HistoryProjectionBundle,
    DebugReportBundle,
    ExportDraftBundle,
    InMemoryTestBundle
};
```

取值说明：

```text
Unknown：未知类型，校验失败。
BuiltinFixtureBundle：P13 内置 fixture 输入包。
HistoryProjectionBundle：P10 AgentHistoryProjection 安全投影输入包。
DebugReportBundle：P11 AgentDebugReport / AgentDiagnosticsSummary 输入包。
ExportDraftBundle：P12 AgentExportDraft 输入包，只能做导出草稿安全校验，不能反推报告。
InMemoryTestBundle：测试专用内存输入包，只能在测试中使用。
```

使用规则：

```text
生产路径优先支持 BuiltinFixtureBundle / HistoryProjectionBundle / DebugReportBundle。
ExportDraftBundle 只作为“已导出草稿安全验证”能力，不参与 report 反向构造。
InMemoryTestBundle 不能由正式 CLI 参数触发。
```

### 7.2 AgentDebugInputTrustLevel

中文：Agent 调试输入信任等级。

用途：说明输入来源的可信程度。

```cpp
enum class AgentDebugInputTrustLevel {
    Unknown,
    Builtin,
    GeneratedSafeDto,
    TestOnly,
    ExternalUntrusted
};
```

取值说明：

```text
Unknown：未知信任等级，校验失败。
Builtin：代码内置 fixture，可用于 P13/P14 调试。
GeneratedSafeDto：由 P10/P11/P12 安全链路生成的 DTO。
TestOnly：测试构造输入，只允许测试使用。
ExternalUntrusted：外部输入，P14 阶段默认拒绝。
```

使用规则：

```text
P14 默认只接受 Builtin / GeneratedSafeDto / TestOnly。
ExternalUntrusted 必须失败，因为 P14 不做 JSON、文件解析和外部格式安全清洗。
```

### 7.3 AgentDebugInputValidationStatus

中文：Agent 调试输入校验状态。

用途：说明 validator 的最终结论。

```cpp
enum class AgentDebugInputValidationStatus {
    Unknown,
    Valid,
    ValidWithWarnings,
    Invalid
};
```

取值说明：

```text
Unknown：未执行校验或状态异常。
Valid：可安全进入 adapter。
ValidWithWarnings：可进入 adapter，但存在非阻断警告。
Invalid：不可进入 adapter。
```

使用规则：

```text
只有 Valid / ValidWithWarnings 可以继续适配。
Invalid 必须返回 Result error。
Unknown 不允许被当作成功。
```

### 7.4 AgentDebugInputCapability

中文：Agent 调试输入能力。

用途：说明一个输入包可以被适配成什么。

```cpp
enum class AgentDebugInputCapability {
    Unknown,
    CanBuildDebugReport,
    HasDiagnosticsSummary,
    HasExportDraft,
    CanWriteThroughP12,
    TestOnly
};
```

取值说明：

```text
Unknown：未知能力，不能作为有效能力。
CanBuildDebugReport：可以生成或提供 AgentDebugReport。
HasDiagnosticsSummary：包含 AgentDiagnosticsSummary。
HasExportDraft：包含 AgentExportDraft。
CanWriteThroughP12：可以交给 P12 写入链路。
TestOnly：只用于测试。
```

使用规则：

```text
HistoryProjectionBundle 必须具备 CanBuildDebugReport。
DebugReportBundle 必须具备 CanBuildDebugReport，可选 HasDiagnosticsSummary。
ExportDraftBundle 必须具备 HasExportDraft，可选 CanWriteThroughP12，但不能标记 CanBuildDebugReport。
```

### 7.5 AgentDebugInputRejectReason

中文：Agent 调试输入拒绝原因。

用途：让测试、CLI、审计报告能稳定判断失败原因。

```cpp
enum class AgentDebugInputRejectReason {
    Unknown,
    EmptyInputId,
    UnknownKind,
    UnknownTrustLevel,
    ExternalInputNotAllowed,
    MissingPayload,
    ConflictingPayloads,
    InvalidManifest,
    InvalidProjection,
    InvalidReport,
    InvalidDiagnostics,
    InvalidExportDraft,
    HiddenTruthDetected,
    RawStateDetected,
    NotReportConvertible,
    TooManyItems,
    SchemaVersionUnsupported
};
```

取值说明：

```text
EmptyInputId：输入 ID 为空。
UnknownKind：输入类型未知。
UnknownTrustLevel：信任等级未知。
ExternalInputNotAllowed：外部不可信输入在 P14 被拒绝。
MissingPayload：manifest 声明了输入类型，但没有对应 payload。
ConflictingPayloads：一个输入包同时携带多个互斥 payload。
InvalidManifest：manifest 自身不合法。
InvalidProjection：P10 projection validateBasic 失败。
InvalidReport：P11 report validateBasic 失败。
InvalidDiagnostics：P11 diagnostics validateBasic 失败。
InvalidExportDraft：P12 export draft validateBasic 失败。
HiddenTruthDetected：发现隐藏真相字段或 key。
RawStateDetected：发现 GameState / ObjectDefinition / AgentTickRecord 等原始状态字段。
NotReportConvertible：该输入类型不能转换为 AgentDebugReport。
TooManyItems：输入条目超过 adapter 限制。
SchemaVersionUnsupported：schema_version 不受支持。
```

## 8. 数据结构设计

### 8.1 AgentDebugInputId

中文：Agent 调试输入 ID。

```cpp
struct AgentDebugInputIdTag {};
using AgentDebugInputId = pathfinder::foundation::StrongId<AgentDebugInputIdTag>;
```

生成规则：

```text
fixture：agent_debug_input_fixture_<fixture_name>
projection：agent_debug_input_projection_<query_id>_<item_count>
report：agent_debug_input_report_<report_id>
export_draft：agent_debug_input_export_<manifest_id>
test：agent_debug_input_test_<case_name>
```

注意事项：

```text
不要让研发手写随机 ID。
不要把真实存档路径放入 ID。
不要把玩家隐私、机器路径、时间戳直接拼入 ID。
ID 必须稳定、可测试、可复现。
```

### 8.2 AgentDebugInputManifest

中文：Agent 调试输入清单。

作用：描述输入包的元信息，不承载业务明细。

```cpp
struct AgentDebugInputManifest {
    AgentDebugInputId input_id;
    AgentDebugInputKind kind = AgentDebugInputKind::Unknown;
    AgentDebugInputTrustLevel trust_level = AgentDebugInputTrustLevel::Unknown;
    std::string source_label;
    std::string schema_version = "agent_debug_input.v1";
    AgentDebugReportMode requested_report_mode = AgentDebugReportMode::Public;
    bool include_trace_keys = false;
    bool test_only = false;
    size_t declared_item_count = 0;
    std::vector<AgentDebugInputCapability> capabilities;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};
```

字段说明：

```text
input_id：稳定输入 ID。
kind：输入类型。
trust_level：信任等级。
source_label：人类可读来源标签，只能是安全标签，不能放路径、隐藏字段或存档内部结构。
schema_version：输入包 schema 版本，P14 只支持 agent_debug_input.v1。
requested_report_mode：希望构建的报告模式。
include_trace_keys：是否允许 debug trace key，默认 false。
test_only：是否只允许测试使用。
declared_item_count：输入声明的条目数，用于上限保护。
capabilities：输入包能力。
warning_keys：非阻断警告 key。
```

### 8.3 AgentDebugInputSource

中文：Agent 调试输入来源。

作用：承载一种且仅一种安全 payload。

```cpp
struct AgentDebugInputSource {
    AgentDebugInputManifest manifest;
    std::optional<AgentDebugCliFixture> fixture;
    std::optional<pathfinder::protocol::AgentHistoryProjection> projection;
    std::optional<AgentDebugReport> report;
    std::optional<AgentDiagnosticsSummary> diagnostics;
    std::optional<AgentExportDraft> export_draft;

    pathfinder::foundation::Result<void> validateBasic() const;
};
```

互斥规则：

```text
BuiltinFixtureBundle：只能携带 fixture。
HistoryProjectionBundle：只能携带 projection，可选 diagnostics 不允许，因为 diagnostics 应由 report 生成。
DebugReportBundle：必须携带 report，可选 diagnostics。
ExportDraftBundle：只能携带 export_draft。
InMemoryTestBundle：测试内允许携带 report 或 projection，但必须 test_only = true。
```

注意：`AgentDebugInputSource` 是内存 DTO，不是文件格式。

### 8.4 AgentDebugInputValidationIssue

中文：Agent 调试输入校验问题。

```cpp
struct AgentDebugInputValidationIssue {
    AgentDebugReportSeverity severity = AgentDebugReportSeverity::Unknown;
    AgentDebugInputRejectReason reason = AgentDebugInputRejectReason::Unknown;
    std::string issue_key;
    std::optional<std::string> field_key;
    std::vector<std::string> detail_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};
```

### 8.5 AgentDebugInputValidationReport

中文：Agent 调试输入校验报告。

```cpp
struct AgentDebugInputValidationReport {
    AgentDebugInputId input_id;
    AgentDebugInputValidationStatus status = AgentDebugInputValidationStatus::Unknown;
    AgentDebugInputKind kind = AgentDebugInputKind::Unknown;
    size_t checked_item_count = 0;
    std::vector<std::string> checked_keys;
    std::vector<std::string> warning_keys;
    std::vector<AgentDebugInputValidationIssue> issues;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};
```

规则：

```text
status = Valid 时，issues 不能有 Error。
status = ValidWithWarnings 时，至少有一个 warning。
status = Invalid 时，至少有一个 Error。
ok() 只在 Valid / ValidWithWarnings 时返回 true。
```

### 8.6 AgentDebugInputAdapterOptions

中文：Agent 调试输入适配选项。

```cpp
struct AgentDebugInputAdapterOptions {
    AgentDebugReportMode report_mode = AgentDebugReportMode::Public;
    bool include_trace_keys = false;
    bool allow_test_only = false;
    bool allow_external_untrusted = false;
    bool allow_export_draft_only = false;
    size_t max_items = 1000;
};
```

默认值说明：

```text
report_mode = Public：默认生成公开安全报告。
include_trace_keys = false：默认不输出 trace key。
allow_test_only = false：生产路径默认拒绝测试输入。
allow_external_untrusted = false：P14 默认拒绝外部不可信输入。
allow_export_draft_only = false：默认要求输入能生成 AgentDebugReport。
max_items = 1000：防止调试输入过大。
```

### 8.7 AgentDebugInputBundle

中文：Agent 调试输入标准包。

作用：adapter 的输出，供 P13/P11/P12 继续使用。

```cpp
struct AgentDebugInputBundle {
    AgentDebugInputManifest manifest;
    AgentDebugInputValidationReport validation;
    std::optional<AgentDebugReport> report;
    std::optional<AgentDiagnosticsSummary> diagnostics;
    std::optional<AgentExportDraft> export_draft;

    bool hasReport() const;
    bool hasExportDraft() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};
```

规则：

```text
正常 P13/P11/P12 调试导出路径必须 hasReport() = true。
ExportDraftBundle 只有在 allow_export_draft_only = true 时允许 hasReport() = false。
validation.ok() 必须为 true。
report / diagnostics / export_draft 都必须通过各自 validateBasic。
```

## 9. 服务接口设计

### 9.1 AgentDebugInputValidator

中文：Agent 调试输入校验器。

职责：只做校验，不做转换。

```cpp
class AgentDebugInputValidator {
public:
    pathfinder::foundation::Result<AgentDebugInputValidationReport> validate(
        const AgentDebugInputSource& source,
        const AgentDebugInputAdapterOptions& options = {}) const;
};
```

校验内容：

```text
manifest 基础字段。
schema_version 是否为 agent_debug_input.v1。
kind / trust_level 是否允许。
payload 是否存在。
payload 是否互斥。
item_count 是否超过 max_items。
P10/P11/P12 DTO validateBasic 是否通过。
capability 是否与 kind 匹配。
是否包含隐藏真相 key。
是否包含原始状态 key。
是否违反 test_only / external_untrusted 策略。
```

### 9.2 AgentDebugInputAdapter

中文：Agent 调试输入适配器。

职责：把已校验输入转换成标准包。

```cpp
class AgentDebugInputAdapter {
public:
    pathfinder::foundation::Result<AgentDebugInputBundle> adapt(
        const AgentDebugInputSource& source,
        const AgentDebugInputAdapterOptions& options = {}) const;
};
```

适配规则：

```text
BuiltinFixtureBundle：调用 P13 AgentDebugFixtureFactory，得到 report + diagnostics。
HistoryProjectionBundle：调用 P11 AgentDebugReportBuilder，再调用 AgentDiagnosticsBuilder。
DebugReportBundle：校验后透传 report；如果没有 diagnostics，则调用 AgentDiagnosticsBuilder 生成。
ExportDraftBundle：只在 allow_export_draft_only = true 时输出 export_draft，不反推 report。
InMemoryTestBundle：只在 allow_test_only = true 时允许。
```

### 9.3 AgentDebugInputFactory

中文：Agent 调试输入工厂。

职责：提供稳定构造函数，避免研发手写 manifest 和 ID。

```cpp
class AgentDebugInputFactory {
public:
    static pathfinder::foundation::Result<AgentDebugInputSource> fromFixture(
        AgentDebugCliFixture fixture);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromProjection(
        const pathfinder::protocol::AgentHistoryProjection& projection,
        AgentDebugReportMode report_mode = AgentDebugReportMode::Public);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromReport(
        const AgentDebugReport& report,
        const std::optional<AgentDiagnosticsSummary>& diagnostics = std::nullopt);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromExportDraft(
        const AgentExportDraft& draft);

    static pathfinder::foundation::Result<AgentDebugInputSource> fromTestReport(
        const std::string& case_name,
        const AgentDebugReport& report);
};
```

规则：

```text
业务代码优先用 factory，不直接拼 AgentDebugInputSource。
factory 必须生成稳定 input_id。
factory 必须填充正确 kind / trust_level / capabilities。
fromTestReport 必须 test_only = true。
```

## 10. 与 P13 CLI 的关系

P14 可以对 P13 做最小重构：

```text
P13 原流程：CLI options -> FixtureFactory -> P11/P12 -> files
P14 后流程：CLI options -> InputFactory::fromFixture -> InputAdapter -> P11/P12 -> files
```

但是 P14 不新增对外文件输入参数。

P14 不允许新增：

```text
--json
--load-save
--save
--agent-log
--game-state
--http
--websocket
```

P14 可以预留内部接口：

```cpp
class AgentDebugCliRunner {
public:
    pathfinder::foundation::Result<AgentDebugCliResult> run(
        const AgentDebugCliOptions& options) const;

    pathfinder::foundation::Result<AgentDebugCliResult> runWithInput(
        const AgentDebugCliOptions& options,
        const AgentDebugInputSource& input) const;
};
```

限制：

```text
runWithInput 是 C++ 内部测试/工具接口，不是 CLI 文件入口。
runWithInput 必须调用 AgentDebugInputAdapter。
run(options) 的 fixture 行为必须保持 P13 兼容。
```

如果 P13 实现尚未稳定，P14 可以先只实现 `debug_input.h/cpp` 与测试，不强制改 `AgentDebugCliRunner`；但任务完成前必须至少有一个集成测试证明 P13 fixture 能经由 P14 adapter 生成等价 report。

## 11. 数据流

### 11.1 fixture 输入流

```text
CLI --fixture unknown_fruit
-> AgentDebugInputFactory::fromFixture(UnknownFruit)
-> AgentDebugInputValidator::validate
-> AgentDebugInputAdapter::adapt
-> AgentDebugFixtureFactory::build
-> AgentDebugReport + AgentDiagnosticsSummary
-> AgentExportDraftBuilder
-> AgentExportWritePlanner
-> AgentLocalExportService
-> AgentExportVerifier
```

### 11.2 P10 projection 输入流

```text
AgentHistoryProjection
-> AgentDebugInputFactory::fromProjection
-> AgentDebugInputValidator::validate
-> AgentDebugInputAdapter::adapt
-> AgentDebugReportBuilder
-> AgentDiagnosticsBuilder
-> AgentDebugInputBundle(report, diagnostics)
```

### 11.3 P11 report 输入流

```text
AgentDebugReport + optional AgentDiagnosticsSummary
-> AgentDebugInputFactory::fromReport
-> AgentDebugInputValidator::validate
-> AgentDebugInputAdapter::adapt
-> AgentDebugInputBundle(report, diagnostics)
```

### 11.4 P12 export draft 输入流

```text
AgentExportDraft
-> AgentDebugInputFactory::fromExportDraft
-> AgentDebugInputValidator::validate
-> AgentDebugInputAdapter::adapt(allow_export_draft_only = true)
-> AgentDebugInputBundle(export_draft)
```

注意：这个流不能得到 report。

## 12. 隐藏真相扫描设计

P14 需要中心化黑名单函数：

```cpp
std::vector<std::string> agentDebugInputForbiddenKeys();
```

黑名单至少包含：

```text
edible_profile
hunger_delta
health_delta
effect_kind
ObjectDefinition
GameState
AgentTickRecord
reward_value
done =
is_done
SaveGame
SaveManager
```

扫描范围：

```text
manifest.source_label
manifest.warning_keys
report item summary_keys
report item reason_keys
report item phase_keys
report item warning_keys
diagnostics issue_key / detail_keys
export manifest warning_keys
export chunk title_key
export chunk payload
projection summary/status/warning 类字符串字段
```

注意事项：

```text
黑名单函数和安全测试中可以出现这些字符串。
生产 payload 中出现这些字符串必须失败。
不要因为字段大小写不同就放过，扫描应使用小写归一化。
不要把扫描做成 assert；必须返回可测试的 validation issue。
```

## 13. 错误处理

所有错误必须通过 `foundation::Result<T>` 返回。

禁止：

```text
throw exception。
assert 处理业务失败。
返回半初始化 bundle。
失败后继续导出文件。
```

错误 key 建议：

```text
agent_debug_input.empty_input_id
agent_debug_input.unknown_kind
agent_debug_input.unknown_trust_level
agent_debug_input.external_input_not_allowed
agent_debug_input.missing_payload
agent_debug_input.conflicting_payloads
agent_debug_input.invalid_manifest
agent_debug_input.invalid_projection
agent_debug_input.invalid_report
agent_debug_input.invalid_diagnostics
agent_debug_input.invalid_export_draft
agent_debug_input.hidden_truth_detected
agent_debug_input.raw_state_detected
agent_debug_input.not_report_convertible
agent_debug_input.too_many_items
agent_debug_input.schema_version_unsupported
```

## 14. 测试设计

### 14.1 单元测试

文件：

```text
backend/tests/unit/agent/agent_debug_input_test.cpp
```

必须覆盖：

```text
AgentDebugInputKind toString/fromString。
AgentDebugInputTrustLevel toString/fromString。
AgentDebugInputValidationStatus toString/fromString。
AgentDebugInputCapability toString/fromString。
AgentDebugInputRejectReason toString/fromString。
factory fromFixture 生成稳定 ID 和 manifest。
factory fromReport 生成稳定 ID 和能力。
factory fromProjection 生成稳定 ID 和能力。
validator 接受合法 fixture source。
validator 接受合法 report source。
validator 接受合法 projection source。
validator 拒绝 Unknown kind。
validator 拒绝 Unknown trust_level。
validator 拒绝 ExternalUntrusted。
validator 拒绝 missing payload。
validator 拒绝 conflicting payload。
validator 拒绝 hidden truth key。
validator 拒绝 raw state key。
adapter 把 fixture 转成 report + diagnostics。
adapter 把 report 透传成 bundle。
adapter 给没有 diagnostics 的 report 补 diagnostics。
adapter 把 projection 转成 report + diagnostics。
adapter 默认拒绝 export draft only。
adapter 在 allow_export_draft_only=true 时接受 export draft，但 hasReport=false。
```

### 14.2 集成测试

文件：

```text
backend/tests/integration/p14/agent_debug_input_flow_test.cpp
```

必须覆盖：

```text
P13 unknown_fruit fixture 经 P14 adapter 后仍可导出 markdown。
P13 no_decision fixture 经 P14 adapter 后 dry-run 不写文件。
P11 report input 经 P14 adapter 后可交给 P12 生成本地写入计划。
P10 projection input 经 P14 adapter 后可生成 AgentDebugReport。
ExportDraftBundle 不会反推 report。
```

### 14.3 安全测试

文件：

```text
backend/tests/integration/p14/agent_debug_input_security_test.cpp
```

必须覆盖：

```text
source_label 包含 GameState -> 失败。
summary_keys 包含 edible_profile -> 失败。
reason_keys 包含 hunger_delta -> 失败。
phase_keys 包含 health_delta -> 失败。
warning_keys 包含 effect_kind -> 失败。
export payload 包含 ObjectDefinition -> 失败。
ExternalUntrusted -> 失败。
test_only 输入在 allow_test_only=false 时失败。
test_only 输入在 allow_test_only=true 时成功。
```

## 15. 验收命令

基础构建：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
```

P14 专项测试：

```bash
ctest --test-dir build/backend -R "agent_debug_input|p14" --output-on-failure
```

相关回归：

```bash
ctest --test-dir build/backend -R "agent|p14|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
```

全量测试：

```bash
ctest --test-dir build/backend --output-on-failure
```

边界扫描：

```bash
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|SaveManager|httplib|asio|WebSocket|HTTP server|nlohmann|json|socket|send\(|recv\(" \
  backend/include/pathfinder/agent/debug_input.h \
  backend/src/agent/debug_input.cpp \
  backend/tests/integration/p14 \
  && exit 1 || true
```

隐藏真相扫描需要允许黑名单声明本身出现：

```bash
rg -n "edible_profile|hunger_delta|health_delta|effect_kind|reward_value|done =|is_done|GameState|ObjectDefinition|AgentTickRecord" \
  backend/include/pathfinder/agent/debug_input.h \
  backend/src/agent/debug_input.cpp \
  backend/tests/integration/p14 \
  | rg -v "agentDebugInputForbiddenKeys|forbidden|RejectsHiddenTruth|RejectsRawState" \
  && exit 1 || true
```

## 16. 完成标准

P14 完成必须同时满足：

```text
新增 debug_input.h/cpp。
所有枚举有 toString/fromString 测试。
所有 DTO 有 validateBasic。
AgentDebugInputFactory 可稳定构造 fixture/projection/report/export_draft/test 输入。
AgentDebugInputValidator 能拒绝非法输入、隐藏真相、原始状态、不可信外部输入。
AgentDebugInputAdapter 能把 fixture/projection/report 变成标准 report bundle。
ExportDraftBundle 不会反推 report。
P13 fixture 行为不破坏。
P10/P11/P12/P13 回归通过。
没有新增 JSON / SaveManager / HTTP / WebSocket / H5 / RL / LLM。
```

## 17. 给实现 AI 的拆分建议

按下面顺序实现，避免上下文过大：

```text
1. 只建 debug_input.h，写枚举、DTO、接口声明。
2. 写枚举 toString/fromString 和最小 validateBasic。
3. 写 AgentDebugInputFactory，先支持 fromFixture/fromReport。
4. 写 AgentDebugInputValidator，先做 manifest/payload/trust/schema 校验。
5. 增加 forbidden key 扫描。
6. 写 AgentDebugInputAdapter，先支持 fixture/report。
7. 增加 projection -> report。
8. 增加 export_draft only 的受控路径。
9. 接入 P13 fixture 路径或新增 runWithInput 内部接口。
10. 补齐单元测试、集成测试、安全测试。
11. 跑 P14 专项测试。
12. 跑 P10-P14 相关回归。
```

不要一次性改 CLI、P10、P11、P12、P13 多个语义层。

## 18. P14 与后续阶段关系

P14 完成后，系统具备：

```text
安全调试输入入口。
稳定调试输入校验报告。
真实安全 DTO 到 AgentDebugReport 的适配路径。
P13 CLI 从 fixture 走向真实安全数据的架构预留。
```

P14 不让玩家玩法变多，但它让后续 P15-P21 的认知、记忆、知识、传播调试变安全。

后续 P15 开始后，认知系统产生的安全事件/认知摘要可以先变成安全 DTO，再经 P14 调试输入层进入报告和导出链路。

## 19. 关键注意事项

```text
P14 是调试工具输入层，不是游戏规则层。
P14 是只读层，不写 GameState。
P14 不碰 SaveManager。
P14 不碰 JSON。
P14 不碰网络。
P14 不接 H5。
P14 不接 RL/LLM。
P14 不暴露隐藏真相。
P14 不从导出文本反推结构化报告。
P14 不把 ExternalUntrusted 当作可用输入。
P14 允许未来扩展，但本阶段必须小而硬。
```

