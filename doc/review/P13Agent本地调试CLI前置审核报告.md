# P13 Agent 本地调试 CLI 前置审核报告

审核时间：2026-05-17

审核对象：P13 Agent 本地调试 CLI 前置实现

相关设计：

```text
doc/38_P13Agent本地调试CLI前置任务卡设计.md
context_packs/agent_p13.md
```

相关实现：

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tools/agent_debug_cli_main.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

## 1. 审核结论

结论：**P13 暂不通过，需要修复后复审。**

原因：功能链路和测试都已经跑通，但存在 2 个任务卡级别阻断问题：

```text
1. 设计要求的可执行文件名 pathfinder_agent_debug_cli 没有生成，当前只生成 pathfinder_agent_debug_cli_exe。
2. CLI 暴露了 P13 设计范围外的 --scan-content / --no-scan-content 参数，其中 --no-scan-content 会关闭内容安全扫描，不符合 P13 安全边界。
```

这两个问题修复后，P13 大概率可以通过复审。

## 2. 已通过项

### 2.1 构建通过

执行命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
```

结果：通过。

### 2.2 P13 定向测试通过

执行命令：

```bash
ctest --test-dir build/backend -R "agent_debug_cli|p13" --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 50
```

### 2.3 P10-P13 相关回归通过

执行命令：

```bash
ctest --test-dir build/backend -R "agent|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 397
```

### 2.4 全量测试通过

执行命令：

```bash
ctest --test-dir build/backend --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 443
```

### 2.5 P11/P12 编排方向正确

`AgentDebugCliRunner` 当前流程符合 P13 设计主线：

```text
validate options
-> AgentDebugFixtureFactory
-> AgentExportDraftBuilder
-> AgentExportWritePlanner
-> AgentLocalExportService
-> AgentExportVerifier
-> AgentDebugCliResult
```

未发现 CLI 直接调用 `AgentRuntime`、`Policy`、`RulePipeline`、`ReplayRunner`。

### 2.6 fixture 边界基本正确

已有 fixture：

```text
UnknownFruit
NoDecision
PublicSafe
```

满足 P13 最小 fixture 要求。

`PublicSafe` 没有填充 `reason_keys` / `phase_keys` / `warning_keys`，符合公开报告约束。

### 2.7 dry-run 行为正确

`dry_run=true` 时只生成写入计划，不调用文件写入，测试确认不会创建输出目录。

### 2.8 base_name 安全校验正确

`base_name` 仅允许：

```text
[a-zA-Z0-9_-]
```

`../escape` 会失败，符合 P13 路径安全要求。

## 3. 阻断问题

### 3.1 阻断问题 1：可执行文件名不符合设计

严重级别：**Blocker**

设计要求：

```text
pathfinder_agent_debug_cli：本地命令行可执行文件。
./build/backend/pathfinder_agent_debug_cli --help
```

当前实现：

```cmake
add_executable(pathfinder_agent_debug_cli_exe
    tools/agent_debug_cli_main.cpp
)
```

实际构建产物：

```text
build/backend/pathfinder_agent_debug_cli_exe
```

审核验证：

```bash
ls -l build/backend/pathfinder_agent_debug_cli*
```

结果：

```text
build/backend/pathfinder_agent_debug_cli_exe
```

审核验证：

```bash
if [ -x build/backend/pathfinder_agent_debug_cli ]; then build/backend/pathfinder_agent_debug_cli --help; else echo 'missing build/backend/pathfinder_agent_debug_cli'; fi
```

结果：

```text
missing build/backend/pathfinder_agent_debug_cli
```

影响：

```text
P13 任务卡中的最小命令不可直接执行。
后续文档、AI 实现、脚本、P14 计划都会按 pathfinder_agent_debug_cli 查找工具。
```

修复建议：

```cmake
add_executable(pathfinder_agent_debug_cli
    tools/agent_debug_cli_main.cpp
)
```

或者保留内部 target 名，但设置输出名：

```cmake
set_target_properties(pathfinder_agent_debug_cli_exe PROPERTIES
    OUTPUT_NAME pathfinder_agent_debug_cli
)
```

必须补充验收：

```bash
./build/backend/pathfinder_agent_debug_cli --help
./build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --format markdown --output-dir /tmp/pathfinder_p13_cli --base-name unknown_fruit --overwrite
```

### 3.2 阻断问题 2：暴露了设计范围外的 CLI 参数，并允许关闭安全扫描

严重级别：**Blocker**

P13 允许的 CLI 范围：

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

当前实现额外暴露：

```text
--scan-content
--no-scan-content
```

相关代码：

```text
backend/src/agent/debug_cli.cpp:228
backend/src/agent/debug_cli.cpp:331
backend/src/agent/debug_cli.cpp:333
```

审核验证：

```bash
build/backend/pathfinder_agent_debug_cli_exe export --fixture unknown_fruit --output-dir /tmp/p13_no_scan_audit --dry-run --no-scan-content; echo exit=$?
```

结果：

```text
Parse OK
command=Export fixture=UnknownFruit format=Markdown output_dir=/tmp/p13_no_scan_audit dry_run=true planned_file_count=2
exit=0
```

问题：

```text
--no-scan-content 会把 options.scan_content 设置为 false。
Runner 会把该值传入 AgentExportVerifier::verify。
这等于允许 CLI 用户关闭 P12 的 forbidden terms 内容扫描。
```

影响：

```text
违反 P13 “受控 CLI 壳层”边界。
违反 P13 “Any unknown argument must reject” 规则。
降低隐藏真相防护强度。
后续 AI 可能误以为调试 CLI 可以暴露安全开关。
```

修复建议：

```text
1. 从 help 文本移除 [--scan-content|--no-scan-content]。
2. Parser 不再接受 --scan-content / --no-scan-content。
3. AgentDebugCliOptions 可以保留内部 scan_content=true，但 P13 CLI 不允许外部关闭。
4. 增加测试：--scan-content 返回 InvalidArguments。
5. 增加测试：--no-scan-content 返回 InvalidArguments。
```

## 4. 非阻断建议

### 4.1 成功执行时 main 会额外打印 Parse OK

严重级别：Minor

当前实际命令：

```bash
build/backend/pathfinder_agent_debug_cli_exe export --fixture unknown_fruit --format markdown --output-dir /tmp/pathfinder_p13_cli_audit --base-name unknown_fruit --overwrite
```

输出：

```text
Parse OK
command=Export fixture=UnknownFruit format=Markdown output_dir=/tmp/pathfinder_p13_cli_audit file_count=2 verification=Passed
```

建议：

```text
main 只在 help / parse failure / should_execute=false 时打印 parse_result.summary_text。
正常执行路径不要打印 Parse OK，只打印 runner 的最终摘要。
```

原因：P13 要的是简短控制台结果，`Parse OK` 对研发没有实际价值，后续脚本解析也容易多一行噪声。

### 4.2 dev_notes 测试数量描述需要更新

严重级别：Minor

当前实际 P13 ctest 注册数量：

```text
50
```

`backend/dev_notes/agent.md` 写的是：

```text
P13 定向测试: 112/112 通过
```

建议修正为本次实际结果，避免后续审核误判。

## 5. 边界扫描结果

### 5.1 Runtime / Pipeline / Replay / Raw State 边界

原始扫描命中：

```text
backend/tests/integration/p13/agent_debug_cli_flow_test.cpp 中 fileContains/ASSERT_TRUE 用于确认导出文件不包含 GameState / ObjectDefinition。
```

过滤测试断言后，无违规命中。

结论：通过。

### 5.2 Network / JSON / Save / RL / LLM 边界

原始扫描命中：

```text
--json / --websocket 拒绝逻辑。
json_rejected 测试。
```

过滤拒绝逻辑和测试名称后，无违规命中。

结论：通过，但 `--scan-content / --no-scan-content` 属于单独阻断问题。

### 5.3 argc / argv 边界

`argc/argv` 只出现在允许位置：

```text
backend/tools/agent_debug_cli_main.cpp
backend/src/agent/debug_cli.cpp
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/
```

结论：通过。

## 6. 复审前必须修复清单

必须修复：

```text
1. 生成 build/backend/pathfinder_agent_debug_cli，而不是只生成 pathfinder_agent_debug_cli_exe。
2. 删除或拒绝 --scan-content / --no-scan-content 两个外部 CLI 参数。
3. 增加 --scan-content / --no-scan-content 被拒绝的 parser/security 测试。
```

建议修复：

```text
4. 正常执行路径不打印 Parse OK。
5. 修正 backend/dev_notes/agent.md 中 P13 测试数量描述。
```

## 7. 复审建议命令

修复后请执行：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
./build/backend/pathfinder_agent_debug_cli --help
./build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --format markdown --output-dir /tmp/pathfinder_p13_cli --base-name unknown_fruit --overwrite
./build/backend/pathfinder_agent_debug_cli export --fixture no_decision --format text --output-dir /tmp/pathfinder_p13_cli --dry-run
ctest --test-dir build/backend -R "agent_debug_cli|p13" --output-on-failure
ctest --test-dir build/backend -R "agent|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

还需要补充验证：

```bash
./build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --output-dir /tmp/p13_should_fail --scan-content
# 期望 exit=2

./build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --output-dir /tmp/p13_should_fail --no-scan-content
# 期望 exit=2
```

## 8. 最终判断

P13 当前实现的核心功能是正确的：

```text
fixture -> report -> draft -> plan -> write -> verify
```

但由于可执行文件名与设计不一致，并且额外暴露了可关闭安全扫描的 CLI 参数，本次审核结论为：

```text
暂不通过，修复后复审。
```
