# cognition P3 上下文包

## 1. P3 目标

实现 cognition 最小模块，承载 P3 unknown fruit + eat 单条规则闭环所需的认知证据和认知状态。

P3 完成后必须满足：

```text
CognitionKey 可唯一定位一条认知。
CognitionEvidence 可表达一次反馈证据。
CognitionClaim 可记录主体对某条认知的置信度。
CognitionState 可管理全部认知条目。
CognitionUpdater::applyEvidence 可更新认知状态。
所有结构可测试。
不做完整记忆系统。
不做完整知识系统。
不做完整传播系统。
不做完整误解系统。
不做长期记忆压缩。
```

## 2. P3 边界

### 2.1 允许实现

```text
CognitionSubjectId (复用 foundation::EntityId)
CognitionEffectKind: Unknown / Edible / Harmful / HungerChanged / HealthChanged
CognitionConfidence: double 0.0-1.0 包装或直接字段校验
CognitionKey: subject_id / object_definition_id / action_id / effect_kind
CognitionEvidence: key / source_event_id / confidence_delta / observed_hunger_delta / observed_health_delta
CognitionClaim: key / confidence / evidence_count / last_event_id
CognitionState: claims / findClaim / upsertClaim
CognitionUpdater::applyEvidence(state, evidence)
validateBasic
```

### 2.2 禁止实现

```text
完整记忆系统。
完整知识系统。
完整传播系统。
完整误解系统。
长期记忆压缩。
CognitionOwner。
CognitionProjection。
CognitionTrace。
MemoryCandidate。
KnowledgePromotionHint。
CognitionUpdatePolicy。
CognitionPolicyMatcher。
视觉 reveal 字段 (颜色、形状、外观)。
```

## 3. 依赖约束

### 3.1 允许依赖

```text
foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / HashValue / EventId / EntityId / ObjectDefinitionId / ActionId
```

### 3.2 禁止依赖

```text
config
command
state
event
pipeline
engine
rules
agent
protocol
frontend
memory
knowledge
propagation
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/cognition/
backend/src/cognition/
backend/tests/unit/cognition/
backend/dev_notes/cognition.md
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/state/ (除非编译必须)
backend/include/pathfinder/event/
backend/include/pathfinder/pipeline/
backend/include/pathfinder/config/
backend/include/pathfinder/command/
backend/include/pathfinder/engine/
backend/include/pathfinder/rules/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
frontend/
```

## 6. 测试要求

```text
合法 CognitionKey 通过。
非法 subject_id 失败。
非法 object_definition_id 失败。
confidence_delta 越界失败。
新证据创建 claim。
第二条证据增强 confidence。
confidence 不超过 1。
非法 evidence 失败。
认知 claim 不包含颜色/形状/视觉 reveal 字段。
source_event_id 可为空或合法，按实现固定。
```

## 7. 验收命令

```bash
cmake --build build/backend
ctest --test-dir build/backend -R cognition --output-on-failure
```

## 8. 常见错误

```text
在 Cognition 中读取真实规则真相。
实现记忆压缩。
实现知识传播。
保存视觉 reveal (颜色、形状、外观)。
让 CognitionState 直接写 EventStore。
把 FeedbackEvidence 直接变成 KnowledgeClaim。
```

## 9. 任务执行顺序

```text
TASK-P3-005: 实现 Cognition 基础类型 (CognitionEffectKind / CognitionConfidence / CognitionKey / CognitionEvidence)
TASK-P3-006: 实现 CognitionState / CognitionUpdater (CognitionClaim / CognitionState / CognitionUpdater)
```

## 10. 前置文档索引

```text
doc/11_认知状态设计.md: 认知条目基本表达 (第 1-5 章)
doc/28_P3未知对象首条规则闭环任务卡设计.md: P3 任务卡
```

## 11. 设计关键约束

```text
认知是主观状态，不等于世界真实规则。
认知可以错误、冲突、衰减、修正。
认知按主体隔离。
Cognition 记录用途认知，不记录颜色形状。
认知更新只基于证据，不能偷看真实规则。
P3 只关注: unknown_fruit + eat + edible/hunger_effect + confidence > 0。
```
