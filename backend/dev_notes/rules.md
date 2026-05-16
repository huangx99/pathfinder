# rules 模块开发记录

## P3 状态

- P3 实现完成，审核通过

### TASK-P3-011: 实现 UnknownFruitFixture

- 实现 UnknownFruitFixture::createInitialState()
  - actor_001: hunger=80, health=100
  - unknown_fruit definition: category=Food, edible_profile={Edible, hunger_delta=-20, health_delta=0}
  - obj_unknown_fruit_001: quantity=1, flag=None
  - CognitionState 为空
- 实现 UnknownFruitFixture::createEatCommand()
  - command_id=cmd_try_eat_001, source=Test
  - action_id=eat, actor_id=actor_001, intent=Experiment
  - target: obj_unknown_fruit_001 (Object, Primary)
- 创建 unknown_fruit_fixture_test.cpp 测试

### TASK-P3-012: 实现 EatObjectResolver

- 实现 EatObjectResolver::canResolve(command) - 检查 action_id == eat
- 实现 EatObjectResolver::resolve(input) -> Result<EatObjectResolveDraft>
  - 检查 actor 存在
  - 检查 object 存在且未 consumed
  - 检查 object definition 存在
  - 使用 ObjectEdibleProfile 生成 StateChangeSet
  - 生成 ActionResolved 和 FeedbackGenerated 事件
  - 生成 CognitionEvidence
  - 不直接修改 GameState
- 创建 eat_object_resolver_test.cpp 测试

### P3 边界

- 不是完整 RuleEngine
- 不是完整 RuleRegistry
- 不是完整 InteractionRule DSL
- 不是 use/combine/collect/teach resolver
- 不是复杂规则优先级系统
- 验收命令: ctest -R rules
