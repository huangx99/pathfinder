# foundation 开发记录

## 模块职责

基础类型、错误、结果、ID、版本、Hash、Tick、随机种子。

## 禁止依赖

不能依赖 config、state、event、command、rules、pipeline、engine、agent、save、protocol、frontend。

## P0 已完成

```text
Result<T>
ErrorCode / ErrorDetail
StrongId
StateVersion
HashValue
Tick
RandomSeed
Serialization 约定
```

## 公开类型

```text
ErrorCode
ErrorSeverity
ErrorCategory
ErrorDetail
ResultVoid (alias: Result)
ResultT<T>
StrongId<Tag>
StateVersion
Tick
DurationTicks
HashValue
RandomSeed
RandomDrawRecord
```

## 测试命令

```text
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R foundation
```

## 任务记录

### TASK-P0-000

- 状态: 已完成
- 内容: 创建 context_packs/foundation_p0.md
- 文件: context_packs/foundation_p0.md

### TASK-P0-001

- 状态: 已完成
- 内容: 创建 backend CMake 骨架
- 文件:
  - backend/CMakeLists.txt
  - backend/tests/CMakeLists.txt
  - backend/include/pathfinder/foundation/README.md
  - backend/src/foundation/README.md
  - backend/dev_notes/README.md
  - backend/dev_notes/foundation.md

### TASK-P0-002

- 状态: 已完成
- 内容: 创建最小测试入口
- 文件: backend/tests/unit/foundation/smoke_test.cpp

### TASK-P0-003

- 状态: 已完成
- 内容: 实现 ErrorCode / ErrorDetail
- 文件:
  - backend/include/pathfinder/foundation/error.h
  - backend/src/foundation/error.cpp
  - backend/tests/unit/foundation/error_test.cpp

### TASK-P0-004

- 状态: 已完成
- 内容: 实现 Result<void>
- 文件:
  - backend/include/pathfinder/foundation/result.h
  - backend/tests/unit/foundation/result_void_test.cpp

### TASK-P0-005

- 状态: 已完成
- 内容: 实现 Result<T>
- 文件:
  - backend/include/pathfinder/foundation/result.h (扩展)
  - backend/tests/unit/foundation/result_value_test.cpp

### TASK-P0-006

- 状态: 已完成
- 内容: 实现 StrongId 基础包装
- 文件:
  - backend/include/pathfinder/foundation/id.h
  - backend/tests/unit/foundation/id_basic_test.cpp

### TASK-P0-007

- 状态: 已完成
- 内容: 实现 StrongId 格式校验和哈希
- 文件: backend/tests/unit/foundation/id_validation_test.cpp

### TASK-P0-008

- 状态: 已完成
- 内容: 实现 StateVersion / Tick
- 文件:
  - backend/include/pathfinder/foundation/version.h
  - backend/include/pathfinder/foundation/time.h
  - backend/tests/unit/foundation/version_tick_test.cpp

### TASK-P0-009

- 状态: 已完成
- 内容: 实现 HashValue
- 文件:
  - backend/include/pathfinder/foundation/hash.h
  - backend/tests/unit/foundation/hash_test.cpp

### TASK-P0-010

- 状态: 已完成
- 内容: 实现 RandomSeed / RandomDrawRecord
- 文件:
  - backend/include/pathfinder/foundation/random.h
  - backend/tests/unit/foundation/random_seed_test.cpp

### TASK-P0-011

- 状态: 已完成
- 内容: 实现 serialization.h 约定
- 文件:
  - backend/include/pathfinder/foundation/serialization.h
  - backend/tests/unit/foundation/serialization_contract_test.cpp

### TASK-P0-012

- 状态: 已完成
- 内容: foundation 聚合检查
- 验收: 所有 10 个 foundation 测试通过

### TASK-P0-FIX-001

- 状态: 已完成
- 内容: 修正 Result 和 StrongId 校验
- 修改:
  - result.h: ResultVoid -> Result<void>, ResultT<T> -> Result<T>
  - id.h: 添加 isValidIdString 声明
  - id.cpp: 实现 isValidIdString
  - result_void_test.cpp: 使用 Result<void>
  - result_value_test.cpp: 使用 Result<T>
  - id_validation_test.cpp: 使用生产代码的 isValidIdString
- 验收: 所有 10 个 foundation 测试通过

## P1 建议

进入 P1 (config + command) 前，确认：

```text
foundation 所有测试通过。
dev_notes 已记录完成状态。
context_packs/foundation_p0.md 与实际文件一致。
没有提前实现上层模块。
```

P1 可以开始：

```text
ConfigLoader
CommandEnvelope
ActionCommand
```
