# P52 Kimi 实现任务单

## 1. 任务

请实现 P52：野生 Agent 与野兽生态接入。

必须先阅读：

```text
doc/00_设计文档编写要求.md
doc/82_P52野生Agent与野兽生态接入任务卡设计.md
doc/82A_P52野生Agent与野兽生态风险头脑风暴.md
doc/82B_P52野生Agent与野兽生态测试文档.md
doc/81_P51Agent网格目标执行任务卡设计.md
doc/review/P51Agent网格目标执行审核报告.md
doc/80_P50教学与无知NPC基础行动任务卡设计.md
doc/79_P49探索行为产生知识任务卡设计.md
doc/73_P43统一世界Command管线前置设计.md
doc/74_P44世界网格最小Runtime与探索投影设计.md
```

## 2. 实现范围

允许新增：

```text
backend/include/pathfinder/world_beast_ecology/
backend/src/world_beast_ecology/
backend/tests/unit/world_beast_ecology/
backend/tests/integration/world_beast_ecology/
```

允许修改：

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
```

如确实需要微调 P51 类型以支持 P52，必须先说明原因，且只能做向后兼容小改动。

## 3. 必须实现

```text
1. world_beast_ecology_types：枚举、DTO、validateBasic、toString/fromString。
2. Beast profile / world / knowledge / command pipeline ports。
3. InMemory 或 fake profile repository 仅用于测试。
4. BeastPerceptionBuilder。
5. BeastInstinctGate。
6. BeastDecisionPolicy。
7. BeastCommandCompiler。
8. BeastInterruptProjector。
9. BeastEcologyCoordinator。
10. BeastProjectionBridge。
11. 单元测试、集成测试、架构扫描测试。
```

## 4. 禁止事项

```text
不要改旧 H5。
不要写死 wolf / torch / fire / red_berry / berry / wood / beast_shadow / companion。
不要绕过 WorldCommandPipeline。
不要直接改 runtime.entities / actors / resource_nodes / inventory arrays。
不要让野兽读取玩家知识或 NPC 私有知识。
不要在 P52 直接形成 KnowledgeClaim；学习仍由 P49/P18-P21 负责。
不要把本能伪装成知识。
不要写只为当前 demo 通过的假测试。
不要提交或推送，完成后等待审核。
```

## 5. 建议实现路线

```text
第一步：类型、枚举、validateBasic、roundtrip 测试。
第二步：ports 和测试 fake。
第三步：perception builder。
第四步：instinct gate。
第五步：decision policy。
第六步：command compiler，确保 source=BeastDecision、command_key 非空。
第七步：coordinator dry-run 和 execute via fake pipeline。
第八步：interrupt projector 和 projection bridge。
第九步：架构扫描和相关回归。
```

## 6. 必跑测试

```bash
cmake --build build/backend --target pathfinder_tests_world_beast_ecology pathfinder_tests_world_beast_ecology_integration -j 2
ctest --test-dir build/backend -R '^(world_beast_ecology_|architecture_world_beast_ecology_)' --output-on-failure
ctest --test-dir build/backend -R '^(world_agent_execution_|world_command_|world_runtime_|world_learning_|world_teaching_|agent_|goal_execution_|knowledge_)' --output-on-failure
```

## 7. 完成后必须回复

```text
修改文件。
新增模块。
测试命令和结果。
是否存在未解决风险。
```
