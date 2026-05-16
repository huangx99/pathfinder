# foundation 头文件

## 职责

基础类型、错误、结果、ID、版本、Hash、Tick、随机种子。

## 禁止依赖

不能依赖 config、state、event、command、rules、pipeline、engine、agent、save、protocol、frontend。

## 公开类型

```text
ErrorCode
ErrorSeverity
ErrorCategory
ErrorDetail
Result<void>
Result<T>
StrongId<Tag>
StateVersion
Tick
DurationTicks
HashValue
RandomSeed
RandomDrawRecord
```
