# P52 野生 Agent 与野兽生态测试文档

## 1. 单元测试

### 1.1 类型与枚举

```text
world_beast_ecology_enum_roundtrip
world_beast_ecology_profile_validate
world_beast_ecology_tick_request_validate
world_beast_ecology_intent_validate
```

验收：所有 enum 支持 toString/fromString；Unknown 非法；TestOnly 仅测试使用。

### 1.2 感知构建

```text
world_beast_ecology_perception_filters_invisible
world_beast_ecology_perception_keeps_safe_tags
```

验收：感知 port 不返回隐藏对象，不泄露 hidden truth。

### 1.3 本能门禁

```text
world_beast_ecology_instinct_allows_move_wait
world_beast_ecology_instinct_blocks_missing_capability
```

验收：野兽没有对应本能能力时不能执行该 intent。

### 1.4 决策策略

```text
world_beast_ecology_policy_waits_without_target
world_beast_ecology_policy_approaches_prey_tag
world_beast_ecology_policy_flees_danger_tag
world_beast_ecology_policy_uses_learned_risk
```

验收：策略只基于 tag/profile/claim，不基于具体内容 key。

### 1.5 命令编译

```text
world_beast_ecology_compiler_sets_beast_source
world_beast_ecology_compiler_sets_command_key
world_beast_ecology_compiler_rejects_unresolved_target
```

验收：`WorldCommandDto.validateBasic()` 通过，source 为 BeastDecision。

## 2. 集成测试

### 2.1 野兽 tick 走 pipeline

```text
world_beast_ecology_tick_executes_pipeline
```

验收：fake pipeline execute_count 增加，result 保留 command result。

### 2.2 野兽接近生成打断

```text
world_beast_ecology_projects_external_interrupt
```

验收：接近目标时输出 `ExternalInterruptSignal::ThreatAppeared` 或 `ThreatEscalated`。

### 2.3 学习后行为改变

```text
world_beast_ecology_learned_risk_changes_intent
```

验收：没有 risk claim 时可 Approach；有自身 risk claim 后选择 Flee/AvoidArea。

### 2.4 经验不被吞掉

```text
world_beast_ecology_preserves_experiences
```

验收：fake pipeline 返回 experiences，P52 result 原样保留。

## 3. 架构扫描

```bash
rg -n "h5|dialog|playable" backend/include/pathfinder/world_beast_ecology backend/src/world_beast_ecology -S || true
rg -n "wolf|torch|fire|red_berry|berry|wood|beast_shadow|companion" backend/include/pathfinder/world_beast_ecology backend/src/world_beast_ecology -S || true
rg -n "\.actors\[|\.entities\[|\.resource_nodes\[|\.inventories\[|\.put\(|createOrUpdateGeneratedCell|upsertGeneratedResourceNode" backend/include/pathfinder/world_beast_ecology backend/src/world_beast_ecology -S || true
rg -n "recipient_claim_json|learned=true|force_learn|force_teach|knowledge_json" backend/include/pathfinder/world_beast_ecology backend/src/world_beast_ecology -S || true
```

结果必须均无匹配。

## 4. 必跑命令

```bash
cmake --build build/backend --target pathfinder_tests_world_beast_ecology pathfinder_tests_world_beast_ecology_integration -j 2
ctest --test-dir build/backend -R '^(world_beast_ecology_|architecture_world_beast_ecology_)' --output-on-failure
ctest --test-dir build/backend -R '^(world_agent_execution_|world_command_|world_runtime_|world_learning_|world_teaching_|agent_|goal_execution_|knowledge_)' --output-on-failure
```
