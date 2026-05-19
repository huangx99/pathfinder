# story_p40 context pack

## 阶段

P40：动态目标执行与外界打断系统。

## 核心目标

承接 P39 生成的 `AgentPlan`，把计划变成可执行、可暂停、可恢复、可重规划的动态目标执行流程。

P40 解决：

```text
目标怎么一步步做。
缺材料/工具坏了怎么办。
狼、雨、敌袭、小孩受伤等外界事件如何打断当前目标。
紧急事件处理完后如何恢复原任务。
执行失败后如何请求 P39 重规划。
```

## 必读文档

```text
doc/00_设计文档编写要求.md
doc/68_P39知识链式推理与目标规划任务卡设计.md
doc/67_P38世界交互结算与可见后果任务卡设计.md
doc/65_P36行为目标知识最小升级设计.md
doc/64_P35状态化对象规则后端最小实现设计.md
doc/47_P21学习闭环任务卡设计.md
doc/54_P27受限条件表达式全量重构任务卡设计.md
```

## 允许新增文件

```text
backend/include/pathfinder/goal_execution/
backend/src/goal_execution/
backend/tests/unit/goal_execution/
backend/tests/integration/p40/
doc/review/P40*.md
```

## 允许修改文件

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/include/pathfinder/world_interaction/world_services.h
backend/src/world_interaction/world_services.cpp
backend/src/h5_dialog/dialog_turn_service.cpp
backend/src/h5_playable/playable_projection_mapper.cpp
frontend/h5_playable/app.js
frontend/h5_playable/style.css
```

## 禁止事项

```text
禁止在 ChopWoodDriver 中处理狼。
禁止在 BuildStructureDriver 中处理敌袭。
禁止 P40 自己决定长期目标，不调用 P39。
禁止 P40 直接修改 WorldSnapshot。
禁止 H5 根据执行摘要自行推导下一步。
禁止无上限目标栈。
禁止外界事件无感知也打断 Agent。
禁止把内部 score 暴露给玩家。
禁止跳过 P38 WorldChangeApplier。
禁止 Driver 内部手写材料数量判断。
禁止 P40 绕过 P28/P28.5 反应系统生成材料。
```

## 核心模块

```text
GoalExecutionSystem
GoalStackManager
ActionDriverRegistry
ActionDriver
InternalBlockerResolver
InterruptSystem
InterruptPriorityEvaluator
ExecutionResumeValidator
MaterialRequirementEvaluator
MaterialReservationManager
ReactionMaterialResolver
DriverCommandAdapter
ExecutionProjectionMapper
```

## 关键概念

```text
InternalBlocker：内部阻塞，例如斧头钝、木头不够、食物没了。
ExternalInterrupt：外界打断，例如狼出现、下雨、敌袭、小孩受伤。
GoalStack：目标栈，支持主目标、子目标、紧急目标。
PausedExecutionContext：被中断后保存的上下文。
ResumeDecision：紧急目标完成后恢复/重规划/取消。
```

## 最小验收案例

```text
建房目标 -> 缺木头 -> 伐木 -> 斧头钝 -> 打磨 -> 继续伐木 -> 建房。
建房目标 -> 伐木 -> 狼出现 -> 暂停伐木 -> 处理狼 -> 恢复伐木。
狼出现必须由 InterruptSystem 处理，不能写进 ChopWoodDriver。
DriverCommand 必须适配 P38 结算，不能直接改世界。
材料数量必须通过 MaterialRequirementEvaluator 计算，不能在 Driver 内手写 wood >= N。
缺材料必须通过 ReactionMaterialResolver 连接 P28/P28.5 反应产出链。
材料预定和材料消耗必须分离，目标取消必须释放 reservation。
H5 只显示安全中文执行摘要，不显示内部栈和分数。
```

## 测试命令

```bash
cmake --build build/backend --target pathfinder_tests_goal_execution pathfinder_tests_agent_reasoning pathfinder_tests_world_interaction pathfinder_tests_h5_playable -j2
ctest --test-dir build/backend -R "goal_execution|agent_reasoning|world_interaction|h5_playable" --output-on-failure
```
