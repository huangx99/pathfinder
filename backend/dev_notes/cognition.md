# cognition 模块开发记录

## P3 状态

- P3 实现完成，审核通过

### TASK-P3-005: 实现 Cognition 基础类型

- 实现 CognitionEffectKind: Unknown / Edible / Harmful / HungerChanged / HealthChanged
- 实现 CognitionKey: subject_id / object_definition_id / action_id / effect_kind
- 实现 CognitionEvidence: key / source_event_id / confidence_delta / observed_hunger_delta / observed_health_delta
- validateBasic 检查 ID 非空且格式合法 (isValidIdString)
- confidence_delta 必须在 -1.0 到 1.0 范围内
- source_event_id 非空时检查格式合法
- 创建 cognition_types_test.cpp 测试

### TASK-P3-006: 实现 CognitionState / CognitionUpdater

- 实现 CognitionClaim: key / confidence / evidence_count / last_event_id
- 实现 CognitionState: claims / findClaim / upsertClaim
- 实现 CognitionUpdater::applyEvidence(state, evidence) -> Result<CognitionClaim>
- applyEvidence 先调用 evidence.validateBasic，非法证据返回错误
- confidence = clamp(old + delta, 0, 1)
- 新证据创建 claim，第二条证据增强 confidence
- 创建 cognition_state_test.cpp 测试

### P3 边界

- 不是完整记忆系统
- 不是完整知识系统
- 不是完整传播系统
- 不是完整误解系统
- 不是长期记忆压缩
- 没有视觉 reveal 字段
- 验收命令: ctest -R cognition
