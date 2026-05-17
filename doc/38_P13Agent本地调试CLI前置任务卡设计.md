# P13 Agent本地调试CLI前置任务卡设计

## 1. P13 要解决什么问题

P10-P12 已经完成了 Agent 调试链路的三层基础：

```text
P10：把 Agent 历史转成安全协议投影 DTO。
P11：把投影转成 AgentDebugReport / Diagnostics / ExportDraft。
P12：把 ExportDraft 安全写成本地 .txt / .md 文件并校验。
```

但当前这些能力只能由测试或代码直接调用，研发和 AI 审核还没有一个最小命令行入口。

P13 要解决的是：

```text
提供一个受控的本地调试 CLI 壳层，让研发可以用命令触发 P11/P12 导出闭环，生成可查看的调试文件和简短控制台结果。
```

P13 的一句话目标：

```text
实现 Agent 本地调试 CLI 前置工具，用内置 fixture 调用 P11/P12，验证命令行解析、导出执行、退出码和控制台摘要。
```

P13 是“CLI 前置”，不是完整产品 CLI，不读取真实存档，不接 HTTP/H5，不做 JSON 序列化。

## 2. P13 与前置阶段的关系

P13 依赖：

```text
P11：AgentDebugReport / AgentDiagnosticsSummary / AgentExportDraft / AgentExportDraftBuilder。
P12：AgentExportWritePlanner / AgentLocalExportService / AgentExportVerifier。
```

P13 产出：

```text
AgentDebugCliOptions：解析后的命令行选项。
AgentDebugCliResult：CLI 执行结果 DTO。
AgentDebugCliParser：命令行参数解析器。
AgentDebugFixtureFactory：内置调试 fixture 工厂。
AgentDebugCliRunner：组合 P11/P12 的本地调试执行器。
pathfinder_agent_debug_cli：本地命令行可执行文件。
```

P13 不改变：

```text
规则权威仍是 RulePipeline。
复盘权威仍是 CommandReplayLog + ReplayRunner。
历史查询权威仍是 P10。
报告内容权威仍是 P11。
本地写入权威仍是 P12。
P13 只是开发工具壳层，不参与游戏结算、不参与训练、不参与协议服务。
```

## 3. P13 工程定位

P13 分四层：

```text
CliTypes 层：定义 CLI 命令、fixture、格式、退出码和结果 DTO。
Parser 层：把 argc/argv 转成 AgentDebugCliOptions。
Fixture 层：提供内置 AgentDebugReport / Diagnostics 样例，不读取真实存档。
Runner 层：调用 P11 ExportDraftBuilder + P12 WritePlanner/LocalExport/Verifier。
```

P13 不做：

```text
不读取真实 SaveGame。
不读取真实 AgentTickRecord。
不读取真实 AgentHistoryProjection。
不读取 GameState。
不读取 ObjectDefinition。
不做 JSON 输入。
不做 JSON 输出。
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
不做 LLM。
不做交互式 TUI。
不做 shell 自动补全。
不做多命令插件系统。
```

## 4. P13 最小闭环定义

### 4.1 help 闭环

输入：

```bash
pathfinder_agent_debug_cli --help
```

验收：

```text
返回 exit_code=0。
输出 usage 文本。
不写文件。
不创建目录。
不调用 P11/P12 写入链路。
```

### 4.2 unknown_fruit Markdown 导出闭环

输入：

```bash
pathfinder_agent_debug_cli export --fixture unknown_fruit --format markdown --output-dir <tmp> --base-name unknown_fruit
```

验收：

```text
解析成功。
构造 unknown_fruit fixture。
调用 P11 生成 MarkdownLike AgentExportDraft。
调用 P12 写入 manifest.md 和 chunk_0.md。
调用 P12 Verifier 校验通过。
返回 exit_code=0。
控制台摘要包含 output_dir、file_count、verification=Passed。
导出文件不包含 hidden truth。
```

### 4.3 no_decision dry-run 闭环

输入：

```bash
pathfinder_agent_debug_cli export --fixture no_decision --format text --output-dir <tmp> --dry-run
```

验收：

```text
解析成功。
构造 NoDecision fixture。
调用 P11 生成 PlainText AgentExportDraft。
调用 P12 生成 AgentExportWritePlan。
不写文件。
返回 exit_code=0。
控制台摘要包含 dry_run=true、planned_file_count。
```

### 4.4 路径安全失败闭环

输入：

```bash
pathfinder_agent_debug_cli export --fixture unknown_fruit --format markdown --output-dir <tmp> --base-name ../escape
```

验收：

```text
解析或计划构建失败。
不得写入文件。
返回非 0 exit_code。
错误 key 稳定。
控制台摘要不输出隐藏信息。
```

## 5. P13 新增核心类型

文件建议：`backend/include/pathfinder/agent/debug_cli.h`

### 5.1 AgentDebugCliCommand

```cpp
enum class AgentDebugCliCommand {
    Unknown,
    Help,
    Export
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| Help | 帮助 | 输出 usage，不执行业务 |
| Export | 导出 | 执行 P11/P12 导出闭环 |

### 5.2 AgentDebugCliFixture

```cpp
enum class AgentDebugCliFixture {
    Unknown,
    UnknownFruit,
    NoDecision,
    PublicSafe
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Unknown | 未知 | 默认值，非法有效状态 |
| UnknownFruit | 未知果实样例 | 成功决策 + replay locked 样例 |
| NoDecision | 无决策样例 | 无可行动作/解释样例 |
| PublicSafe | 公开安全样例 | 验证 Public 模式无 trace 泄露 |

注意：

```text
fixture 是开发测试样例，不是游戏真实存档。
P13 不从磁盘读取 fixture JSON。
P13 不读取 GameState。
```

### 5.3 AgentDebugCliFormat

```cpp
enum class AgentDebugCliFormat {
    Unknown,
    Text,
    Markdown,
    ProtocolText
};
```

中文翻译：

| 枚举 | 中文 | 映射 |
|---|---|---|
| Unknown | 未知 | 非法有效状态 |
| Text | 纯文本 | AgentExportFormat::PlainText |
| Markdown | Markdown-like | AgentExportFormat::MarkdownLike |
| ProtocolText | 协议文本 | AgentExportFormat::ProtocolText |

注意：

```text
P13 不提供 JSON 格式。
JSON 必须等协议序列化阶段单独设计。
```

### 5.4 AgentDebugCliExitCode

```cpp
enum class AgentDebugCliExitCode {
    Success = 0,
    InvalidArguments = 2,
    BuildFailed = 3,
    WriteFailed = 4,
    VerificationFailed = 5,
    InternalError = 10
};
```

中文翻译：

| 枚举 | 中文 | 用途 |
|---|---|---|
| Success | 成功 | 命令执行成功 |
| InvalidArguments | 参数非法 | 参数缺失、未知参数、非法值 |
| BuildFailed | 构建失败 | fixture/report/export draft/plan 构建失败 |
| WriteFailed | 写入失败 | P12 本地写入失败 |
| VerificationFailed | 校验失败 | P12 Verifier 失败 |
| InternalError | 内部错误 | 未预期异常 |

约束：

```text
退出码必须稳定。
测试必须覆盖成功、参数错误、写入失败、校验失败。
```

## 6. P13 数据契约

### 6.1 AgentDebugCliOptions

```cpp
struct AgentDebugCliOptions {
    AgentDebugCliCommand command = AgentDebugCliCommand::Unknown;
    AgentDebugCliFixture fixture = AgentDebugCliFixture::Unknown;
    AgentDebugCliFormat format = AgentDebugCliFormat::Markdown;
    std::string output_dir;
    std::string base_name = "agent_debug";
    bool dry_run = false;
    bool overwrite = false;
    bool help = false;
    bool scan_content = true;
    size_t max_items_per_chunk = 50;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
command 不能 Unknown。
help=true 时允许只设置 Help。
Export 命令必须设置 fixture。
Export 命令必须设置 output_dir。
format 不能 Unknown。
base_name 必须符合 P12 base_name 安全规则。
max_items_per_chunk 必须 >0 且 <=1000。
scan_content 默认为 true。
```

### 6.2 AgentDebugCliResult

```cpp
struct AgentDebugCliResult {
    AgentDebugCliExitCode exit_code = AgentDebugCliExitCode::InternalError;
    std::string message_key;
    std::string summary_text;
    size_t planned_file_count = 0;
    size_t written_file_count = 0;
    std::string output_dir;
    std::vector<std::string> output_files;
    std::vector<std::string> warning_keys;
    std::vector<std::string> error_keys;

    foundation::Result<void> validateBasic() const;
};
```

约束：

```text
Success 时 error_keys 必须为空。
非 Success 时 error_keys 必须非空。
summary_text 必须非空。
output_files 只能包含相对路径，不能包含绝对路径。
summary_text 不允许包含 hidden truth。
```

### 6.3 AgentDebugCliParseResult

```cpp
struct AgentDebugCliParseResult {
    AgentDebugCliOptions options;
    AgentDebugCliResult result;
    bool should_execute = false;

    foundation::Result<void> validateBasic() const;
};
```

职责：

```text
表示解析命令行后的结果。
如果是 --help，should_execute=false 且 result.exit_code=Success。
如果解析失败，should_execute=false 且 result.exit_code=InvalidArguments。
如果解析成功，should_execute=true。
```

## 7. P13 服务接口

### 7.1 AgentDebugCliParser

```cpp
class AgentDebugCliParser {
public:
    foundation::Result<AgentDebugCliParseResult> parse(
        int argc,
        const char* const argv[]) const;
};
```

支持参数：

```text
--help
export
--fixture unknown_fruit|no_decision|public_safe
--format text|markdown|protocol_text
--output-dir <dir>
--base-name <safe_name>
--dry-run
--overwrite
--max-items-per-chunk <n>
```

不支持参数：

```text
--json
--http
--websocket
--save
--load-save
--agent-log
--game-state
--rl
--llm
```

解析规则：

```text
未知参数必须失败。
重复参数可以后者覆盖前者，但建议记录 warning_key。
--help 优先级最高。
export 是唯一业务命令。
未指定 format 时默认 markdown。
未指定 base-name 时默认 agent_debug。
dry-run 不写文件。
overwrite 只传给 P12 allow_overwrite。
```

### 7.2 AgentDebugFixtureFactory

```cpp
struct AgentDebugFixtureBundle {
    AgentDebugReport report;
    AgentDiagnosticsSummary diagnostics;

    foundation::Result<void> validateBasic() const;
};

class AgentDebugFixtureFactory {
public:
    foundation::Result<AgentDebugFixtureBundle> build(
        AgentDebugCliFixture fixture) const;
};
```

职责：

```text
构造 P13 内置 fixture。
```

fixture 要求：

```text
UnknownFruit：包含 PipelineSucceeded / Decided / command_id / ReplayLocked 摘要。
NoDecision：包含 NoDecision / reason_keys 或 phase_keys / Diagnostics warning 或 clean 摘要。
PublicSafe：Public 模式，不含 reason_keys / phase_keys / warning_keys。
```

禁止：

```text
不得读取真实文件。
不得读取 GameState。
不得读取 AgentTickRecord。
不得读取 AgentHistoryProjection。
不得包含 hidden truth 字段名。
```

### 7.3 AgentDebugCliRunner

```cpp
class AgentDebugCliRunner {
public:
    foundation::Result<AgentDebugCliResult> run(
        const AgentDebugCliOptions& options) const;
};
```

执行流程：

```text
1. validateBasic options。
2. 如果 command=Help，返回 usage result。
3. FixtureFactory 构造 AgentDebugFixtureBundle。
4. AgentExportDraftBuilder 构造 AgentExportDraft。
5. AgentExportWritePlanner 构造 AgentExportWritePlan。
6. dry_run=true 时不写文件，返回 planned summary。
7. AgentLocalExportService 写入文件。
8. AgentExportVerifier 校验文件。
9. 生成 AgentDebugCliResult。
```

错误映射：

```text
options validate 失败 -> InvalidArguments。
fixture / draft / plan 构建失败 -> BuildFailed。
write status=Failed -> WriteFailed。
verify status=Failed -> VerificationFailed。
异常兜底 -> InternalError。
```

### 7.4 CLI main

文件建议：`backend/tools/agent_debug_cli_main.cpp`

职责：

```text
唯一允许直接使用 argc/argv 的生产入口。
调用 AgentDebugCliParser。
如果 should_execute=true，调用 AgentDebugCliRunner。
打印 summary_text。
返回 AgentDebugCliExitCode 对应整数。
```

注意：

```text
业务解析逻辑必须放在 AgentDebugCliParser，不要堆在 main。
main 不直接调用 P11/P12。
main 不直接写文件。
main 不读取环境变量。
```

## 8. 文件范围

### 8.1 允许新增

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tools/agent_debug_cli_main.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
context_packs/agent_p13.md
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
backend/tools/ 是 P13 允许新增的本地工具目录。
CMake 允许新增 pathfinder_agent_debug_cli executable。
```

## 9. 必读文档

P13 实现 AI 必须读取：

```text
doc/38_P13Agent本地调试CLI前置任务卡设计.md
context_packs/agent_p13.md
context_packs/agent_p12.md
context_packs/agent_p11.md
doc/37_P12Agent本地文件导出与校验任务卡设计.md 的接口片段
doc/36_P11Agent本地调试报告与导出前置任务卡设计.md 的 ExportDraft 片段
backend/dev_notes/agent.md
```

最多再读取：

```text
backend/include/pathfinder/agent/debug_cli.h
backend/include/pathfinder/agent/debug_report.h
backend/include/pathfinder/agent/debug_export.h
backend/include/pathfinder/agent/local_export.h
backend/src/agent/debug_export.cpp
backend/src/agent/local_export.cpp
```

禁止为了 P13 一次性读取：

```text
01-24 全部文档。
完整规则系统。
完整对象系统。
完整协议系统。
完整 SaveManager 设计。
完整 H5 设计。
```

## 10. P13 任务拆分

### 10.1 TASK-P13-000：创建上下文包

任务名称：创建 P13 AI 实现上下文包。

允许新增：

```text
context_packs/agent_p13.md
```

内容必须包含：

```text
P13 目标。
P13 与 P11/P12 的关系。
P13 允许 CLI main 使用 argc/argv。
P13 不做 JSON/HTTP/H5/SaveManager/真实存档读取。
文件范围。
验收命令。
边界扫描命令。
```

验收命令：

```bash
rg -n "P13|CLI|AgentDebugCli|argc|argv|不做 JSON|不做 HTTP|不读真实存档" context_packs/agent_p13.md
```

### 10.2 TASK-P13-001：实现 CLI 基础类型

任务名称：实现 AgentDebugCli 数据契约。

允许新增：

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

实现要求：

```text
定义 AgentDebugCliCommand。
定义 AgentDebugCliFixture。
定义 AgentDebugCliFormat。
定义 AgentDebugCliExitCode。
实现 toString / fromString roundtrip。
定义 AgentDebugCliOptions。
定义 AgentDebugCliResult。
定义 AgentDebugCliParseResult。
实现 validateBasic。
```

测试要求：

```text
所有枚举 roundtrip。
Unknown validateBasic 失败。
Export 缺 fixture 失败。
Export 缺 output_dir 失败。
base_name 非法失败。
max_items_per_chunk=0 失败。
Success result 带 error_keys 失败。
失败 result 无 error_keys 失败。
output_files 包含绝对路径失败。
summary_text 为空失败。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_cli --output-on-failure
```

### 10.3 TASK-P13-002：实现 CLI Parser

任务名称：实现 AgentDebugCliParser。

允许修改：

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
```

实现要求：

```text
实现 parse(argc, argv)。
支持 --help。
支持 export 命令。
支持 --fixture。
支持 --format。
支持 --output-dir。
支持 --base-name。
支持 --dry-run。
支持 --overwrite。
支持 --max-items-per-chunk。
未知参数失败。
缺值参数失败。
非法枚举值失败。
```

测试要求：

```text
--help 解析成功且 should_execute=false。
export unknown_fruit markdown 解析成功。
dry-run 解析成功。
overwrite 解析成功。
未知参数失败。
--json 失败。
--load-save 失败。
--fixture 缺值失败。
--max-items-per-chunk 非数字失败。
--base-name ../escape 失败。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_cli_parser --output-on-failure
```

### 10.4 TASK-P13-003：实现 FixtureFactory

任务名称：实现内置 Agent 调试 fixture。

允许修改：

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
```

实现要求：

```text
定义 AgentDebugFixtureBundle。
实现 AgentDebugFixtureFactory::build。
UnknownFruit fixture validateBasic 通过。
NoDecision fixture validateBasic 通过。
PublicSafe fixture validateBasic 通过。
fixture 不包含 hidden truth 关键词。
fixture 不读取文件。
```

测试要求：

```text
UnknownFruit report / diagnostics validateBasic 通过。
NoDecision report / diagnostics validateBasic 通过。
PublicSafe report mode=Public。
PublicSafe 不含 reason_keys / phase_keys / warning_keys。
Unknown fixture 失败。
fixture summary 不包含 GameState / ObjectDefinition / edible_profile / reward_value。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_cli_fixture --output-on-failure
```

### 10.5 TASK-P13-004：实现 CLI Runner

任务名称：实现 AgentDebugCliRunner。

允许修改：

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
```

实现要求：

```text
实现 AgentDebugCliRunner::run。
Help 返回 usage summary。
Export 调用 FixtureFactory。
Export 调用 AgentExportDraftBuilder。
Export 调用 AgentExportWritePlanner。
dry_run=true 时不调用 AgentLocalExportService。
dry_run=false 时调用 AgentLocalExportService 和 AgentExportVerifier。
错误映射到稳定 exit_code。
summary_text 包含 command、fixture、format、output_dir、planned_file_count、written_file_count、verification。
summary_text 不包含 hidden truth。
```

测试要求：

```text
help runner 成功。
dry-run runner 成功且不写文件。
unknown_fruit markdown runner 写入成功。
no_decision text runner 写入成功。
public_safe protocol_text runner 写入成功。
非法 output_dir 返回 BuildFailed 或 WriteFailed。
已存在文件且 overwrite=false 返回 WriteFailed。
Verifier 失败返回 VerificationFailed。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_debug_cli_runner --output-on-failure
```

### 10.6 TASK-P13-005：实现 CLI main 可执行文件

任务名称：实现 pathfinder_agent_debug_cli 可执行入口。

允许新增：

```text
backend/tools/agent_debug_cli_main.cpp
```

允许修改：

```text
backend/CMakeLists.txt
```

实现要求：

```text
新增 pathfinder_agent_debug_cli executable。
main 只负责调用 Parser 和 Runner。
main 打印 summary_text。
main 返回 AgentDebugCliExitCode 整数。
main 不直接写文件。
main 不直接读文件。
main 不直接构造 fixture。
main 不调用 P11/P12 细节类。
```

手动验收命令示例：

```bash
./build/backend/pathfinder_agent_debug_cli --help
./build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --format markdown --output-dir /tmp/pathfinder_p13_cli --base-name unknown_fruit --overwrite
./build/backend/pathfinder_agent_debug_cli export --fixture no_decision --format text --output-dir /tmp/pathfinder_p13_cli --dry-run
```

### 10.7 TASK-P13-006：CLI 导出集成测试

任务名称：证明 CLI 可完成 unknown_fruit 导出闭环。

允许新增：

```text
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. 调用 AgentDebugCliParser 解析 export unknown_fruit markdown。
2. AgentDebugCliRunner 执行。
3. 断言 exit_code=Success。
4. 断言 output_files 包含 manifest 和 chunk。
5. 断言文件存在。
6. 断言 Verifier 已通过。
7. 断言文件内容不包含 hidden truth。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p13_agent_debug_cli_flow --output-on-failure
```

### 10.8 TASK-P13-007：CLI 安全集成测试

任务名称：证明 CLI 不接受越界参数和禁用能力。

允许新增：

```text
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
```

允许修改：

```text
backend/tests/CMakeLists.txt
```

实现流程：

```text
1. --base-name ../escape 解析失败。
2. --json 解析失败。
3. --load-save xxx 解析失败。
4. --fixture unknown_fruit --output-dir 缺值失败。
5. dry-run 不创建文件。
6. overwrite=false 遇到已存在文件返回 WriteFailed。
7. 确认 root_dir 外没有新文件。
```

验收命令：

```bash
cmake --build build/backend
ctest --test-dir build/backend -R p13_agent_debug_cli_security --output-on-failure
```

### 10.9 TASK-P13-008：边界审计和开发笔记

任务名称：更新开发笔记并执行 P13 审计。

允许修改：

```text
backend/dev_notes/agent.md
doc/review/P13Agent本地调试CLI前置审核报告.md
```

审计要求：

```text
确认 P13 只调用 P11/P12，不调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
确认 P13 不读取真实 SaveGame / AgentTickRecord / AgentHistoryProjection / GameState / ObjectDefinition。
确认 P13 不做 JSON / HTTP / WebSocket / H5 / SaveManager。
确认 argc/argv 只出现在 debug_cli parser/main/tests 中。
确认 CLI 写入仍受 P12 root_dir + safe relative_path 约束。
确认 P13/P12/P11/P10 回归测试通过。
```

验收命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent_debug_cli|p13" --output-on-failure
ctest --test-dir build/backend -R "agent|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "AgentRuntime\(|FirstSupportedPolicy|decide\(|tickOne\(|RulePipeline|execute\(|ReplayRunner|AgentTickRecord|AgentHistoryProjection|GameState|ObjectDefinition" backend/include/pathfinder/agent/debug_cli.h backend/src/agent/debug_cli.cpp backend/tools/agent_debug_cli_main.cpp backend/tests/integration/p13 && exit 1 || true
rg -n "httplib|asio|WebSocket|HTTP server|SaveManager|nlohmann|json|socket|send\(|recv\(|RewardCalculator|EpisodeRunner|rl_environment|RLPolicy|NeuralPolicy|LLMPolicy" backend/include/pathfinder/agent/debug_cli.h backend/src/agent/debug_cli.cpp backend/tools/agent_debug_cli_main.cpp backend/tests/integration/p13 && exit 1 || true
rg -n "argc|argv" backend/include/pathfinder/agent/debug_cli.h backend/src/agent/debug_cli.cpp backend/tools/agent_debug_cli_main.cpp backend/tests/unit/agent/agent_debug_cli_test.cpp backend/tests/integration/p13
```

审核报告必须包含：

```text
构建结果。
P13 定向测试结果。
P10-P12 回归测试结果。
全量测试结果。
CLI 手动命令或集成测试结果。
边界扫描结果。
是否允许进入 P14。
```

## 11. P13 设计风险和处理

### 11.1 风险：CLI 变成真实存档入口

处理：

```text
P13 只允许内置 fixture。
禁止 --load-save / --agent-log / --game-state。
真实存档或真实历史输入必须后续单独设计。
```

### 11.2 风险：CLI 绕过 P12 路径安全

处理：

```text
CLI 不直接写文件。
所有写入必须经过 P12 AgentLocalExportService。
base_name 和 output_dir 继续由 P12 PathPolicy/Planner 校验。
```

### 11.3 风险：提前加入 JSON 或 HTTP

处理：

```text
--json、--http、--websocket 一律解析失败。
P13 只输出控制台 summary_text 和 P12 txt/md 文件。
```

### 11.4 风险：main 堆业务逻辑

处理：

```text
main 只调用 Parser / Runner。
Parser / Runner 必须可单测。
不要把 fixture 构造、导出写入、校验逻辑写在 main。
```

### 11.5 风险：上下文过大导致 AI 实现混乱

处理：

```text
P13 实现 AI 只读 P13/P12/P11 关键上下文包和相关头文件。
不要回读 01-24 全量设计。
不要修改规则、对象、管线、协议、复盘。
```

## 12. P13 完成后的系统能力

P13 完成后，系统具备：

```text
可以通过本地 CLI 输出 help。
可以通过本地 CLI 选择内置 Agent 调试 fixture。
可以通过本地 CLI 生成 P11 ExportDraft。
可以通过本地 CLI 调用 P12 写出 .txt / .md 文件。
可以 dry-run 只生成写入计划，不落盘。
可以通过稳定退出码表达参数错误、构建失败、写入失败、校验失败。
可以为研发和 AI 审核提供最小可用本地工具入口。
```

P13 完成后仍不具备：

```text
读取真实存档。
读取真实 Agent 历史日志。
JSON 输入/输出。
HTTP / WebSocket。
H5 页面。
SaveManager。
数据库。
权限系统。
RL 训练运行。
LLM 接入。
```

## 13. P13 之后建议

P13 之后建议进入 P14：

```text
P14：Agent 调试输入适配前置，把真实但安全的 P10/P11/P12 数据来源接入 CLI 前先定义输入边界。
```

P14 可选方向：

```text
方案 A：定义只读本地调试包格式，但仍不做 JSON。
方案 B：定义测试 fixture 文件格式和解析器。
方案 C：定义从未来 Save/Replay 投影中抽取 P10 Projection 的安全接口。
```

建议优先：

```text
先做 P14 输入适配边界，不要直接做 HTTP/H5。
因为一旦 CLI 能读真实数据，必须先保证不泄露 hidden truth、不绕过 P10/P11/P12。
```
