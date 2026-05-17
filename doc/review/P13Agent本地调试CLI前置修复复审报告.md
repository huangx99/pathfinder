# P13 Agent 本地调试 CLI 前置修复复审报告

复审时间：2026-05-17

复审对象：P13 审核阻断项修复

原审核报告：

```text
doc/review/P13Agent本地调试CLI前置审核报告.md
```

## 1. 复审结论

结论：**P13 修复后通过复审。**

原审核中的 2 个阻断项已经修复：

```text
1. 已生成设计要求的 build/backend/pathfinder_agent_debug_cli。
2. --scan-content / --no-scan-content 已作为非法参数拒绝，CLI 不再允许关闭内容安全扫描。
```

同时完成 2 个非阻断建议：

```text
1. 正常执行路径不再额外打印 Parse OK。
2. backend/dev_notes/agent.md 已更新 P13 实际测试结果。
```

## 2. 修复内容

### 2.1 可执行文件名修复

修改文件：

```text
backend/CMakeLists.txt
```

修复方式：

```text
保留 CMake target 名 pathfinder_agent_debug_cli_exe。
通过 OUTPUT_NAME 生成设计要求的 pathfinder_agent_debug_cli 可执行文件。
```

验证命令：

```bash
build/backend/pathfinder_agent_debug_cli --help
```

结果：

```text
exit=0
help 文本正常输出。
```

### 2.2 移除外部扫描开关

修改文件：

```text
backend/src/agent/debug_cli.cpp
```

修复方式：

```text
help 文本移除 [--scan-content|--no-scan-content]。
Parser 不再接受 --scan-content。
Parser 不再接受 --no-scan-content。
scan_content 保持内部默认 true，CLI 用户无法关闭内容安全扫描。
```

验证命令：

```bash
build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --output-dir /tmp/p13_should_fail --scan-content
build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --output-dir /tmp/p13_should_fail --no-scan-content
```

结果：

```text
--scan-content exit=2
--no-scan-content exit=2
```

### 2.3 补充测试

修改文件：

```text
backend/tests/unit/agent/agent_debug_cli_test.cpp
backend/tests/integration/p13/agent_debug_cli_security_test.cpp
backend/tests/CMakeLists.txt
```

新增测试：

```text
agent_debug_cli_parser_scan_content_fails
agent_debug_cli_parser_no_scan_content_fails
p13_scan_content_rejected
p13_no_scan_content_rejected
```

### 2.4 控制台输出优化

修改文件：

```text
backend/tools/agent_debug_cli_main.cpp
```

修复结果：

```text
正常 export 执行只输出最终 runner 摘要，不再额外输出 Parse OK。
help / parse failure / should_execute=false 仍输出 parse summary。
```

验证输出：

```text
command=Export fixture=UnknownFruit format=Markdown output_dir=/tmp/pathfinder_p13_cli_fix file_count=2 verification=Passed
```

## 3. 验证结果

### 3.1 构建

执行命令：

```bash
cmake -S backend -B build/backend
cmake --build build/backend
```

结果：通过。

### 3.2 命令级验收

执行命令：

```bash
build/backend/pathfinder_agent_debug_cli --help
build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --format markdown --output-dir /tmp/pathfinder_p13_cli_fix --base-name unknown_fruit --overwrite
build/backend/pathfinder_agent_debug_cli export --fixture no_decision --format text --output-dir /tmp/pathfinder_p13_cli_fix --dry-run
build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --output-dir /tmp/p13_should_fail --scan-content
build/backend/pathfinder_agent_debug_cli export --fixture unknown_fruit --output-dir /tmp/p13_should_fail --no-scan-content
```

结果：

```text
help_exit=0
export_exit=0
dry_exit=0
scan_exit=2
no_scan_exit=2
```

### 3.3 P13 定向测试

执行命令：

```bash
ctest --test-dir build/backend -R "agent_debug_cli|p13" --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 54
```

### 3.4 相关回归

执行命令：

```bash
ctest --test-dir build/backend -R "agent|p13|p12|p11|p10|p9|p8|p7|p6|p5|p4|p3|replay|protocol" --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 401
```

### 3.5 全量测试

执行命令：

```bash
ctest --test-dir build/backend --output-on-failure
```

结果：

```text
100% tests passed, 0 tests failed out of 447
```

### 3.6 边界扫描

扫描范围：

```text
backend/include/pathfinder/agent/debug_cli.h
backend/src/agent/debug_cli.cpp
backend/tools/agent_debug_cli_main.cpp
backend/tests/integration/p13
```

结果：

```text
Runtime / Policy / RulePipeline / ReplayRunner / GameState / ObjectDefinition：无违规命中。
HTTP / WebSocket / SaveManager / JSON 实现 / RL / LLM：无违规命中。
生产 CLI 代码中不再出现 scan-content / no-scan-content。
```

## 4. 最终判断

P13 当前满足任务卡要求：

```text
--help 成功。
unknown_fruit markdown 导出成功。
no_decision dry-run 不写文件。
非法 base_name 失败。
--json / --load-save 失败。
--scan-content / --no-scan-content 失败。
CLI result exit codes 稳定。
P13 定向测试通过。
P10-P13 相关回归通过。
全量测试通过。
```

最终结论：

```text
P13 修复后通过，可以进入 P14 实现/审核链路。
```
