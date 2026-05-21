# P51 Kimi 实现任务单

## 1. 任务

请实现 P51：Agent 网格目标执行。

必须先阅读：

```text
doc/00_设计文档编写要求.md
doc/81_P51Agent网格目标执行任务卡设计.md
doc/81A_P51Agent网格目标执行风险头脑风暴.md
doc/81B_P51Agent网格目标执行测试文档.md
doc/80_P50教学与无知NPC基础行动任务卡设计.md
doc/79_P49探索行为产生知识任务卡设计.md
doc/73_P43统一世界Command管线前置设计.md
doc/74_P44世界网格最小Runtime与探索投影设计.md
doc/75_P45背包容器与物品归属系统设计.md
doc/77_P47资源采集砍伐挖掘任务卡设计.md
doc/78_P48制作与世界反应接入任务卡设计.md
```

## 2. 实现范围

新增目录：

```text
backend/include/pathfinder/world_agent_execution/
backend/src/world_agent_execution/
backend/tests/unit/world_agent_execution/
backend/tests/integration/world_agent_execution/
```

新增 CMake target：

```text
pathfinder_world_agent_execution
pathfinder_tests_world_agent_execution
pathfinder_tests_world_agent_execution_integration
```

## 3. 必须实现

```text
1. WorldAgentExecutionTypes：枚举、DTO、validateBasic、toString/fromString。
2. IWorldAgentExecutionContextStore + InMemoryWorldAgentExecutionContextStore。
3. Agent decision ports：visible world、inventory、knowledge、command pipeline。
4. WorldAgentContextBuilder。
5. WorldAgentGoalSelector。
6. WorldAgentReasoningBridge（适配已有 P39/P40，不重写推理）。
7. AgentActionKnowledgeGuard（必须调用 P50 NpcBasicActionKnowledgeGate）。
8. AgentPlanCommandCompiler。
9. AgentExecutionCoordinator。
10. WorldAgentProjectionBridge。
11. 单元测试、集成测试、架构扫描测试。
```

## 4. 禁止事项

```text
不要改旧 H5。
不要写死 companion / torch / wood / beast / wolf / red_berry / mushroom。
不要绕过 WorldCommandPipeline。
不要直接改 runtime.entities / actors / resource_nodes / inventory arrays。
不要让 NPC 读取玩家知识。
不要在 P51 内直接形成知识；学习仍由 P49/P18-P21 负责。
不要把 command 执行结果伪造成成功。
不要写只为当前 demo 通过的假测试。
```

## 5. 建议实现路线

```text
第一步：类型、枚举、validateBasic、roundtrip 测试。
第二步：context store 和 ports fake。
第三步：context builder。
第四步：knowledge guard 调 P50。
第五步：command compiler，先支持 Move / Gather / Wait，结构上预留其他 command。
第六步：coordinator dry-run。
第七步：coordinator execute via fake pipeline。
第八步：projection bridge。
第九步：内部 blocker / interrupt 最小闭环。
第十步：架构扫描和相关回归。
```

## 6. 必跑测试

```bash
cmake --build build/backend --target pathfinder_tests_world_agent_execution pathfinder_tests_world_agent_execution_integration -j 2
ctest --test-dir build/backend -R '^(world_agent_execution_|architecture_world_agent_execution_)' --output-on-failure
ctest --test-dir build/backend -R '^(world_command_|world_runtime_|world_inventory_|world_harvest_|world_reaction_|world_learning_|world_teaching_|agent_|goal_execution_|knowledge_)' --output-on-failure
```

## 7. 完成后必须回复

```text
修改文件。
新增模块。
测试命令和结果。
是否存在未解决风险。
```
