# P5 Agent 最小闭环上下文包

## 目标

P5 只做 Agent 最小工程闭环：

- 定义一个 Agent 是谁
- 让 Agent 只能看到允许看到的观察
- 让 Agent 拥有可枚举、可校验的动作空间
- 让 Agent 把意图转换成 CommandEnvelope
- 让同一套 RulePipeline 继续作为唯一结算入口

## 核心原则

- Agent 只产生意图，不直接修改世界
- Agent 只能通过 CommandEnvelope 调用 RuleEngine
- Agent 看到的不是全量真实状态，而是符合权限、认知和观察范围的投影
- Agent 决策必须可记录、可复盘、可测试、可替换

## 禁止事项

- 禁止 Agent 直接修改 GameState
- 禁止 Agent 直接创建 StateChangeDraft
- 禁止 Agent 直接调用 EatObjectResolver
- 禁止 Agent 从 GameState 读取 DebugOnly / Hidden 真相
- 禁止 Agent 自己应用 StateChangeSet
- 禁止 Agent 自己生成最终 EventRecord
- 禁止实现完整 Agent 调度器（AgentRuntime）
- 禁止实现复杂 AI 策略算法
- 禁止实现行为树、GOAP、效用 AI、神经网络
- 禁止实现强化学习训练算法

## 允许读取的头文件

- pathfinder/foundation/id.h
- pathfinder/foundation/result.h
- pathfinder/foundation/error.h
- pathfinder/foundation/time.h
- pathfinder/foundation/version.h
- pathfinder/command/types.h
- pathfinder/command/envelope.h
- pathfinder/command/action_command.h
- pathfinder/command/target.h
- pathfinder/command/validation.h
- pathfinder/cognition/cognition_state.h
- pathfinder/event/event_record.h

## 任务清单与验收命令

### TASK-P5-000: 上下文包
验收: `rg -n "P5|Agent|CommandEnvelope|Observation|Intent|禁止|验收" context_packs/agent_p5.md backend/dev_notes/agent.md`

### TASK-P5-001: 模块骨架
验收:
```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R agent --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### TASK-P5-002: 基础枚举
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_types --output-on-failure
```

### TASK-P5-003: AgentDefinition / AgentProfile
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_definition --output-on-failure
```

### TASK-P5-004: AgentBinding
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_binding --output-on-failure
```

### TASK-P5-005: AgentObservation
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_observation --output-on-failure
rg -n "ObjectRevealLevel|visual_reveal|mosaic|shape|color" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent && exit 1 || true
```

### TASK-P5-006: AgentActionSpace
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_action_space --output-on-failure
```

### TASK-P5-007: AgentIntent / AgentDecision
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_intent --output-on-failure
```

### TASK-P5-008: AgentCommandAdapter
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R agent_command_adapter --output-on-failure
rg -n "RulePipeline|EatObjectResolver|GameState|StateChange|EventRecord" backend/include/pathfinder/agent/command_adapter.h backend/src/agent/command_adapter.cpp && exit 1 || true
```

### TASK-P5-009: unknown fruit Agent 集成测试
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R "p5|p3|p4" --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

### TASK-P5-010: spider flee fire 测试
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R spider_flee --output-on-failure
rg -n "flee.*resolver|FleeResolver|class.*Flee" backend/src backend/include/pathfinder/rules && exit 1 || true
```

### TASK-P5-011: wolf call pack 测试
验收:
```bash
cmake --build build/backend
ctest --test-dir build/backend -R wolf_call_pack --output-on-failure
rg -n "tribe_split|pack_combat|GroupCombat|WarResolver" backend/src backend/include/pathfinder && exit 1 || true
```

### TASK-P5-012: 边界审计
验收:
```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R "agent|p5|p3|p4" --output-on-failure
ctest --test-dir build/backend --output-on-failure
rg -n "RulePipeline|EatObjectResolver|GameState|StateChange|EventRecord" backend/include/pathfinder/agent/command_adapter.h backend/src/agent/command_adapter.cpp && exit 1 || true
rg -n "ObjectRevealLevel|visual_reveal|mosaic|shape|color" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p5 && exit 1 || true
rg -n "rl_environment|policy_tree|WarResolver|GroupCombat|tribe_split|SaveManager|WebSocket|HTTP" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p5 && exit 1 || true
```

## AgentIntentType 到 CommandIntent 映射

| AgentIntentType | CommandIntent | P5 行为 |
|---|---|---|
| Eat | Experiment | 可提交 RulePipeline |
| Use | Experiment | 只校验 |
| AvoidRisk | AvoidRisk | 只校验 |
| Flee | AvoidRisk | 只校验 |
| CallGroup | N/A | 返回 unsupported |
| Explore | Experiment | 只校验 |
| Teach | Teach | 只校验 |
| Combine | Combine | 只校验 |
| Fight | Fight | 只校验 |
| Wait | N/A | 不生成 CommandEnvelope |

## 推荐目录

```
backend/include/pathfinder/agent/
  README.md
  types.h
  definition.h
  binding.h
  observation.h
  action_space.h
  intent.h
  command_adapter.h
backend/src/agent/
  types.cpp
  definition.cpp
  binding.cpp
  observation.cpp
  action_space.cpp
  intent.cpp
  command_adapter.cpp
backend/tests/unit/agent/
  agent_smoke_test.cpp
  agent_types_test.cpp
  agent_definition_test.cpp
  agent_binding_test.cpp
  agent_observation_test.cpp
  agent_action_space_test.cpp
  agent_intent_test.cpp
  agent_command_adapter_test.cpp
backend/tests/integration/p5/
  agent_unknown_fruit_flow_test.cpp
  spider_flee_fire_test.cpp
  wolf_call_pack_test.cpp
```
