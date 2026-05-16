# object 模块开发记录

## P3 状态

- P3 实现完成，审核通过

### TASK-P3-002: 实现 Object 基础类型

- 实现 ObjectCategory: Unknown / Food / Material / Tool / Hazard
- 实现 ObjectRuntimeFlag: None / Consumed / Depleted
- 实现 EdibleEffectKind: Unknown / Edible / Harmful / Neutral
- 实现 ObjectEdibleProfile: effect_kind / hunger_delta / health_delta / risk_level
- 每个枚举提供 toString / fromString
- 创建 object_types_test.cpp 测试

### TASK-P3-003: 实现 WorldObject / ObjectStore

- 实现 ObjectDefinition: definition_id / category / allowed_action_ids / edible_profile
- 实现 WorldObject: object_id / definition_id / quantity / flag
- 实现 ObjectStore: addDefinition / addObject / findDefinition / findObject / markConsumed
- validateBasic 检查 ID 非空和格式合法
- object 引用不存在 definition 在 validateBasic 中失败
- 创建 world_object_test.cpp 测试

### P3 边界

- 不是完整对象库
- 不是完整地图索引
- 不是完整背包系统
- 不是完整对象组合反应
- 不是完整 ConfigRegistry
- 验收命令: ctest -R object
