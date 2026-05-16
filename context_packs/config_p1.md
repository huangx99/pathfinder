# config P1 上下文包

## 1. P1 目标

在 P0 foundation 之上建立配置系统最小入口。

P1 完成后必须满足：

```text
ConfigValidationIssue / ConfigValidationReport 可用。
ConfigPackage 元信息可用。
ConfigLoader 文件读取空壳可用。
config 单元测试通过。
dev_notes/config.md 已记录任务历史。
没有提前实现规则引擎。
```

## 2. P1 边界

### 2.1 允许实现的类型

```text
ConfigValidationSeverity: info / warning / error / fatal
ConfigValidationIssue: severity, ErrorCode, message, file_path, json_path, line, column
ConfigValidationReport: issues 列表, addIssue, hasErrors, hasFatal, issueCount
ConfigPackageId: 基于 StrongId
ConfigVersion: 基于 StrongId 或稳定字符串包装
SchemaVersion: 基于稳定字符串包装
ConfigFileEntry: path, category, required
ConfigPackage: package_id, config_version, schema_version, files, locale_files, test_files, checksum
LoadedConfigFile: path, content, content_hash
ConfigLoadRequest: root_path, relative_files
ConfigLoadResult: files, validation_report
ConfigLoader::loadFiles(request)
```

### 2.2 禁止实现

```text
RuleEngine 完整逻辑
RulePipeline 完整执行
InteractionRule 结算
AgentRuntime
HTTP / WebSocket
H5 前端
复杂 JSON schema
复杂表达式求值
完整对象库
完整事件系统
完整存档复盘
ConfigRegistry
ConfigParser 完整解析
ConfigValidator 完整校验
ExpressionRegistry
ExpressionEvaluator
TranslationRegistry
```

### 2.3 禁止行为

```text
ActionTarget 判断这个对象能不能吃
CommandEnvelope 直接扣血
ConfigLoader 计算食物效果
解析 JSON 内容
构建 ConfigRegistry
校验跨文件引用
```

## 3. 依赖约束

只能依赖 P0 foundation：

```text
ErrorCode
ErrorDetail
Result<T> / Result<void>
StrongId
isValidIdString
Tick
StateVersion
HashValue
RandomSeed
```

禁止依赖尚未实现的模块：

```text
engine
rules
pipeline
state
event
agent
protocol
frontend
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/config/
backend/src/config/
backend/tests/unit/config/
backend/dev_notes/config.md
context_packs/config_p1.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/foundation/ 已有 API
backend/src/foundation/ 已有 API
backend/include/pathfinder/engine/
backend/include/pathfinder/rules/
backend/include/pathfinder/pipeline/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
frontend/
```

## 6. 测试要求

```text
空 report 没有错误
warning 不算错误
error 算错误
fatal 同时算 fatal 和 error
issueCount 正确
合法 package 通过基础校验
缺 package_id 失败
缺 config_version 失败
缺 schema_version 失败
空文件路径失败
没有必需配置文件失败
读取存在文件成功
读取不存在文件返回 error issue
空文件返回 error issue 或 warning
相同内容 hash 稳定
路径穿越 ../ 必须拒绝
```

## 7. 验收命令

```text
cmake --build build/backend
ctest --test-dir build/backend -R config --output-on-failure
```

## 8. 任务执行顺序

```text
TASK-P1-000: 创建 context pack (当前)
TASK-P1-001: 创建 config/command 目录与 CMake 空目标
TASK-P1-002: 实现 ConfigValidationIssue / ConfigValidationReport
TASK-P1-003: 实现 ConfigPackage 元信息
TASK-P1-004: 实现 ConfigLoader 文件读取空壳
```

## 9. 前置文档索引

```text
doc/05_配置系统设计.md: 配置系统架构、ConfigManifest 字段
doc/03_错误码与结果类型设计.md: Result / Error 约定
backend/include/pathfinder/foundation/error.h: ErrorCode, ErrorDetail
backend/include/pathfinder/foundation/result.h: Result<T>
backend/include/pathfinder/foundation/id.h: StrongId
backend/include/pathfinder/foundation/hash.h: HashValue
```
