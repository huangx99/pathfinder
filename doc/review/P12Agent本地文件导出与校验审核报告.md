# P12 Agent本地文件导出与校验审核报告

审核时间：2026-05-17  
审核对象：P12 Agent本地文件导出与校验实现  
审核结论：通过，允许进入 P13。

## 1. 审核范围

本次审核依据：

```text
doc/37_P12Agent本地文件导出与校验任务卡设计.md
context_packs/agent_p12.md
P11 Agent本地调试报告与导出前置复审结论
```

重点检查：

```text
P12 是否只消费 P11 AgentExportDraft。
P12 是否把导出限制在 root_dir + safe relative_path。
P12 是否拒绝路径穿越、非法 base_name、非法 relative_path。
P12 是否正确生成写入计划、写入结果和校验报告。
P12 是否按设计允许 std::filesystem / std::ofstream / std::ifstream。
P12 是否没有引入 CLI / JSON / HTTP / WebSocket / H5 / SaveManager。
P12 是否没有读取 GameState / ObjectDefinition / P8/P10 原始结构。
```

## 2. 变更范围核对

P12 新增文件符合任务卡范围：

```text
backend/include/pathfinder/agent/local_export.h
backend/src/agent/local_export.cpp
backend/tests/unit/agent/agent_local_export_test.cpp
backend/tests/integration/p12/agent_local_export_flow_test.cpp
backend/tests/integration/p12/agent_local_export_security_test.cpp
context_packs/agent_p12.md
```

P12 修改文件符合任务卡范围：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/dev_notes/agent.md
```

未发现 P12 禁止范围变更：

```text
未修改 rules / object / pipeline / replay / protocol。
未新增 frontend / http / websocket / save 目录。
```

说明：工作区中仍存在 P10/P11/P12 文档类未跟踪文件，不影响本次 P12 实现审核结论。

## 3. 功能审核

### 3.1 基础类型与枚举

结果：通过。

已确认能力：

```text
AgentExportFileId / AgentExportWriteId / AgentExportVerifyId helper 稳定生成。
AgentExportFileKind 支持 Unknown / Manifest / Chunk / Diagnostics。
AgentExportFileExtension 支持 Unknown / Txt / Md。
AgentExportWriteStatus 支持 Unknown / Planned / Written / Failed / Skipped。
AgentExportVerificationStatus 支持 Unknown / Passed / Failed / Warning。
所有枚举 roundtrip 测试通过。
Unknown 可解析，但 validateBasic 不允许作为有效状态。
```

### 3.2 PathPolicy 与 FilePlan

结果：通过。

已确认能力：

```text
root_dir 不能为空。
root_dir 包含 .. 会失败。
max_file_count 必须在 1..1024。
max_file_bytes 必须 >0。
max_total_bytes 必须 >= max_file_bytes。
relative_path 不能为空。
relative_path 不允许以 / 开头。
relative_path 不允许包含 ..。
relative_path 不允许包含反斜杠。
relative_path 只允许安全字符。
Manifest content 不允许为空。
byte_size 必须等于 content.size()。
```

边界结论：路径穿越和绝对 relative_path 已被数据契约阻断。

### 3.3 WritePlan 与 Planner

结果：通过。

已确认能力：

```text
AgentExportWritePlan 要求 write_id / export_id / files 非空。
files.size 受 max_file_count 限制。
单文件 byte_size 受 max_file_bytes 限制。
total_bytes 必须等于文件 byte_size 之和。
total_bytes 受 max_total_bytes 限制。
relative_path 不允许重复。
必须包含 Manifest 文件。
PlainText -> .txt。
ProtocolText -> .txt。
MarkdownLike -> .md。
base_name 只允许字母、数字、下划线、中划线。
manifest/chunk 文件名稳定。
```

说明：`include_manifest_file=false` 当前会因为 WritePlan 必须包含 Manifest 而构建失败，符合 P12 “最小实现必须包含 manifest 文件”的安全策略。

### 3.4 LocalExportService

结果：通过。

已确认能力：

```text
写入前会 validateBasic plan。
root_dir 不存在且 allow_create_directories=true 时会创建目录。
root_dir 不存在且 allow_create_directories=false 时失败。
目标文件已存在且 allow_overwrite=false 时失败。
目标文件已存在且 allow_overwrite=true 时可覆盖。
写入成功时 status=Written。
写入失败时 status=Failed 且 error_keys 非空。
任一文件失败后不继续写后续文件。
```

边界结论：P12 生产代码允许且仅在本阶段使用 `std::filesystem` / `std::ofstream` 进行受控本地写入，未引入网络、CLI 或 SaveManager。

### 3.5 ExportVerifier

结果：通过。

已确认能力：

```text
校验 write_id 与 plan 一致。
检查导出文件是否存在。
检查实际文件大小是否等于计划 byte_size。
统计 actual_file_count / actual_total_bytes。
可选扫描 forbidden terms。
发现缺文件、大小不一致、禁止词时 status=Failed。
完整写入后 status=Passed。
```

禁止词集合已覆盖：

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

## 4. 测试结果

构建结果：通过。

```bash
cmake -S backend -B build/backend
cmake --build build/backend
```

P12 定向测试：通过。

```text
100% tests passed, 0 tests failed out of 61
```

P3-P12 / Agent / Replay / Protocol 阶段回归：通过。

```text
100% tests passed, 0 tests failed out of 347
```

全量测试：通过。

```text
100% tests passed, 0 tests failed out of 393
```

路径安全集成测试：通过。

```text
p12_path_security 通过。
覆盖 base_name=../escape、relative_path=../escape.txt、allow_overwrite=false 已存在文件。
```

## 5. 边界扫描结果

运行时 / 管线 / 复盘 / 原始历史扫描：通过，有 1 个测试注释命中。

```text
扫描内容：AgentRuntime( / FirstSupportedPolicy / decide( / tickOne( / RulePipeline / execute( / ReplayRunner / AgentTickRecord / AgentHistoryProjection
生产代码结果：无命中。
测试结果：仅命中 integration/p12 注释“no AgentHistoryProjection dependency”。
结论：不构成越界。
```

隐藏真相扫描：通过，有设计性命中。

```text
生产代码命中：local_export.cpp 中 forbiddenTerms() 禁止词列表。
测试命中：单元测试和集成测试用于验证禁止词扫描。
结论：这些命中是 P12 Verifier 必需能力，不是隐藏真相读取或泄露。
```

网络 / JSON / CLI / SaveManager 扫描：通过，有测试入口命中。

```text
生产代码结果：无 httplib / asio / WebSocket / HTTP server / SaveManager / nlohmann / json / socket / send( / recv( / argc / argv 命中。
测试结果：integration/p12 main(argc, argv) 命中。
结论：测试可执行入口参数不等同于 CLI 功能，不构成越界。
```

Reward / Done / RL 扫描：通过，有设计性命中。

```text
生产代码命中：local_export.cpp 中 forbiddenTerms() 禁止词列表包含 reward_value / is_done / done =。
测试命中：导出内容安全断言。
结论：这些命中是禁止词扫描能力，不是 Reward/Done/RL 实现。
```

目录越界扫描：通过。

```text
未发现 frontend / http / websocket / save 相关新增目录。
```

## 6. 非阻塞建议

### 6.1 P12 审计扫描命令应增加白名单说明

P12 与 P11 不同，P12 必须在 Verifier 中保存 forbidden terms，因此简单扫描 `GameState|ObjectDefinition|reward_value|done` 会命中安全扫描器自身。建议后续审计脚本把 `forbiddenTerms()` 和对应测试用例作为白名单，避免误报。

结论：非阻塞，不影响 P12 通过。

### 6.2 WriteResult 可增强 planned_file_count 校验

当前 `AgentExportWriteResult::validateBasic` 会校验 `written_file_count` 与实际 Written 文件数一致，但未强制校验 `planned_file_count` 与调用方 plan 的 files.size 一致。由于 `write()` 会正确设置该字段，当前不影响实际路径；后续可在 `AgentExportVerifyRequest::validateBasic` 中补充 plan/files 与 write_result.planned_file_count 的一致性校验。

结论：非阻塞，不影响 P12 通过。

### 6.3 后续可考虑 partial write 标记

当前写入服务遇到后续文件失败会停止并返回 Failed，但前面已经成功写入的文件会保留。P12 任务卡只要求失败后不继续写后续文件，未要求回滚。后续如需要更强事务语义，可增加 `partial_written` 标记或清理策略。

结论：非阻塞，不影响 P12 通过。

## 7. 最终结论

P12 审核通过。

理由：

```text
实现范围符合 P12 任务卡。
P12 只消费 P11 AgentExportDraft，不读取 P8/P10 原始结构。
路径安全、base_name 安全、relative_path 安全均有校验和测试。
本地写入被限制在 root_dir + safe relative_path。
Verifier 能校验文件存在、大小、数量和禁止词。
没有引入 CLI / JSON / HTTP / WebSocket / H5 / SaveManager。
没有调用 AgentRuntime / Policy / RulePipeline / ReplayRunner。
P12 定向测试、阶段回归、全量测试全部通过。
```

允许进入：P13 本地调试 CLI 前置。
