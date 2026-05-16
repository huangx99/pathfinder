# P7 AgentRuntime 最小闭环复审报告

复审日期：2026-05-17  
复审对象：P7 AgentRuntime / FirstSupportedPolicy / Runtime Types 修复后实现  
复审结论：通过，允许进入 P8。

## 1. 复审范围

本次复审针对上次 P7 审核提出的 4 个阻塞问题逐项确认：

- Policy 边界扫描是否已清理干净。
- `AgentActionCandidate::validateBasic` 是否校验 `suggested_targets` 覆盖 `required_target_types`。
- P7 要求的 candidate target 传播测试是否补齐，P6 集成测试是否不再手写 target。
- `submit_action_allowlist` 是否参与 Policy 选择，而不是只在 Runtime 提交前事后拦截。
- P7 构建、定向测试、阶段回归测试、全量测试、边界扫描是否通过。

## 2. 复审结论

结论：P7 通过。

理由：

- 上次 4 个阻塞问题均已修复。
- P7 定向测试、阶段回归测试、全量测试全部通过。
- P7 严格边界扫描全部无命中。
- `AgentRuntime` 仍保持最小运行时边界，没有扩展成完整 AI、H5、RL、SaveManager 或战斗/族群系统。
- `FirstSupportedPolicy` 的 allowlist 选择语义已补上，能避免先选到不可提交动作后错过后续可提交动作。

## 3. 上次阻塞项复查

### BLOCKER-1：Policy 边界扫描未通过

状态：已修复，通过。

本次执行：

```bash
rg -n "GameState|object_store|actor_store|RulePipeline|EatObjectResolver|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/policy.h backend/src/agent/policy.cpp backend/tests/unit/agent/agent_policy_test.cpp
```

结果：无命中。

证据：

- `backend/include/pathfinder/agent/policy.h:14` 已改为不触发禁用词的注释。
- `backend/tests/unit/agent/agent_policy_test.cpp:163` 到 `backend/tests/unit/agent/agent_policy_test.cpp:169` 已改为不触发禁用词的测试说明。

判断：符合 P7 任务卡对 Policy 边界扫描的要求。

### BLOCKER-2：ActionCandidate 未校验 suggested_targets 覆盖 required_target_types

状态：已修复，通过。

证据：

- `backend/src/agent/action_space.cpp:33` 增加 `suggested_targets` 非空时的覆盖校验。
- `backend/src/agent/action_space.cpp:35` 收集已覆盖的 target type。
- `backend/src/agent/action_space.cpp:39` 遍历 `required_target_types`。
- `backend/src/agent/action_space.cpp:41` 缺少 required target type 时返回 error。
- `backend/tests/unit/agent/agent_action_space_test.cpp` 已补充 target type mismatch 失败测试和覆盖成功测试。

判断：Runtime / Policy 可以继续信任 candidate 的结构化 target，不需要从 `reason_key` 解析目标。

### BLOCKER-3：candidate target 传播测试没有完整落地

状态：已修复，通过。

证据：

- `backend/tests/unit/agent/agent_action_space_builder_test.cpp` 已断言 Eat candidate 的 `command_action_id == eat` 和 `suggested_targets`。
- `backend/tests/unit/agent/agent_action_space_builder_test.cpp:141` 到 `backend/tests/unit/agent/agent_action_space_builder_test.cpp:144` 已断言 Flee candidate 的 entity target。
- `backend/tests/unit/agent/agent_action_space_builder_test.cpp:217` 增加多 object Eat candidate 测试。
- `backend/tests/unit/agent/agent_action_space_builder_test.cpp:236` 到 `backend/tests/unit/agent/agent_action_space_builder_test.cpp:240` 确认多个 Eat candidate 共享 `command_action_id=eat` 且带 Object target。
- `backend/tests/integration/p6/agent_builder_unknown_fruit_flow_test.cpp:76` 到 `backend/tests/integration/p6/agent_builder_unknown_fruit_flow_test.cpp:77` 已从 `eat_candidate->suggested_targets` 复制 target。
- `backend/tests/integration/p6/spider_fire_action_space_test.cpp:73` 到 `backend/tests/integration/p6/spider_fire_action_space_test.cpp:84` 已从 `flee_candidate->suggested_targets` 复制 target。

补充扫描：

```bash
rg -n "ActionTarget target|intent\.targets\.push_back\(target\)" backend/tests/integration/p6/agent_builder_unknown_fruit_flow_test.cpp backend/tests/integration/p6/spider_fire_action_space_test.cpp
```

结果：无命中。

判断：P7 任务卡要求的 candidate target 传播已经被测试锁住。

### BLOCKER-4：submit_action_allowlist 没有参与 Policy 选择

状态：已修复，通过。

证据：

- `backend/include/pathfinder/agent/policy.h:22` 在 `AgentPolicyRequest` 中新增 `submit_action_allowlist`。
- `backend/src/agent/runtime.cpp:66` 将 runtime options 中的 allowlist 传给 policy request。
- `backend/src/agent/policy.cpp:33` 在 allowlist 非空时进入优先选择流程。
- `backend/src/agent/policy.cpp:38` 到 `backend/src/agent/policy.cpp:45` 优先选择 `command_action_id` 或 `action_id` 命中 allowlist 的 supported candidate。
- `backend/tests/unit/agent/agent_policy_test.cpp:172` 增加 `[flee, eat] + allowlist=[eat]` 的优先选择测试。
- `backend/tests/unit/agent/agent_policy_test.cpp:208` 到 `backend/tests/unit/agent/agent_policy_test.cpp:209` 明确断言最终选择 Eat。

判断：allowlist 不再只是 Runtime 事后拦截，而是参与最小策略选择；这符合 P7 任务卡目标。

## 4. 测试结果

本次执行：

```bash
cmake -S backend -B build/backend
```

结果：通过。

本次执行：

```bash
cmake --build build/backend
```

结果：通过。

本次执行：

```bash
ctest --test-dir build/backend -R "agent_policy|agent_runtime_types|agent_runtime|p7" --output-on-failure
```

结果：7/7 passed。

本次执行：

```bash
ctest --test-dir build/backend -R "agent|p7|p6|p5|p3|p4" --output-on-failure
```

结果：32/32 passed。

本次执行：

```bash
ctest --test-dir build/backend --output-on-failure
```

结果：125/125 passed。

## 5. 边界扫描结果

本次执行：

```bash
rg -n "GameState|object_store|actor_store|RulePipeline|EatObjectResolver|edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent/policy.h backend/src/agent/policy.cpp backend/tests/unit/agent/agent_policy_test.cpp
```

结果：无命中。

本次执行：

```bash
rg -n "edible_profile|hunger_delta|health_delta|effect_kind" backend/include/pathfinder/agent backend/src/agent backend/tests/unit/agent backend/tests/integration/p7
```

结果：无命中。

本次执行：

```bash
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat|rl_environment|BehaviorTree|GOAP|UtilityPolicy|NeuralPolicy|LLMPolicy|WebSocket|HTTP|SaveManager" backend/include/pathfinder/agent backend/src/agent backend/tests/integration/p7
```

结果：无命中。

本次执行：

```bash
find backend -type d \( -path '*/frontend*' -o -path '*/http*' -o -path '*/websocket*' -o -path '*/save*' \) -print
```

结果：无输出。

本次执行：

```bash
rg -n "FleeResolver|GroupCombat|WarResolver|tribe_split|pack_combat" backend/include/pathfinder backend/src
```

结果：无命中。

## 6. 通过项确认

已确认通过：

- `pathfinder_agent_runtime` 库已建立，文件范围符合 P7 任务卡。
- `AgentRuntime` 串联 `ObservationBuilder`、`ActionSpaceBuilder`、`FirstSupportedPolicy`、`AgentCommandAdapter`、`RulePipeline`。
- `AgentRuntime` 不直接修改状态，只通过 `RulePipeline` 结算允许提交的命令。
- `FirstSupportedPolicy` 只看 observation/action_space/policy request，不读取世界真实状态。
- `submit_action_allowlist` 已参与选择并继续在 Runtime 提交前做保护。
- unknown fruit P7 集成测试通过，P3 语义保持稳定。
- Fire threat 不提交规则管线，Social/CallGroup 不生成可提交命令。
- P6 target 传播测试已从 candidate 复制 targets，不再手写。
- 未新增 H5、HTTP、WebSocket、SaveManager、RL、完整 AI、FleeResolver、GroupCombat、WarResolver。

## 7. 非阻塞建议

### MINOR-1：`policy.h` 建议显式 include `<vector>`

`AgentPolicyRequest` 现在使用 `std::vector`，当前能编译是因为其他头间接包含了 vector。建议后续顺手在 `backend/include/pathfinder/agent/policy.h` 显式加入 `<vector>`，降低头文件隐式依赖。

不作为 P7 阻塞项。

### MINOR-2：Runtime trace 对多个同语义 action 的候选定位仍可增强

当前 trace 仍根据 action id / command action id 回找 candidate。P7 的最小策略可接受；后续 P8 做 `AgentDecisionRecord` 时建议增加 selected candidate instance id，避免多个 Eat candidate 时 trace 歧义。

不作为 P7 阻塞项。

## 8. 最终结论

P7 复审通过，可以进入 P8。

P7 当前已经完成最小 AgentRuntime 调度闭环：

- 能从可见输入构建主观观察。
- 能从观察构建动作空间。
- 能用最小策略选择动作。
- 能通过统一 Adapter 生成命令。
- 能按 allowlist 受控提交 RulePipeline。
- 能避免未实现的 Flee / CallGroup 被误判为规则成功。

这为后续 P8 的 Agent 运行记录、复盘兼容和训练样本采集打下了稳定基础。
