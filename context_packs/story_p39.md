# story_p39 context pack

## 阶段

P39：知识链式推理与目标规划。

## 目标

把 P38 的规则触发型 Agent 升级为知识推理型 Agent：

```text
需求/目标 -> effect_key 语义 -> 候选行动 -> 前置条件链 -> 计划评分 -> 执行第一步。
```

核心要求：不要为“饿了吃红果、冷了造房、野兽来了用火把”分别写死规则；必须从 Agent 已掌握知识和效果语义推导。

## 必读文档

```text
doc/00_设计文档编写要求.md
doc/46_P20知识传播系统任务卡设计.md
doc/47_P21学习闭环任务卡设计.md
doc/54_P27受限条件表达式全量重构任务卡设计.md
doc/56_P28_5配置化物品与反应流水线任务卡设计.md
doc/64_P35状态化对象规则后端最小实现设计.md
doc/65_P36行为目标知识最小升级设计.md
doc/67_P38世界交互结算与可见后果任务卡设计.md
doc/68_P39知识链式推理与目标规划任务卡设计.md
```

## 允许新增文件

```text
backend/include/pathfinder/agent_reasoning/
backend/src/agent_reasoning/
backend/tests/unit/agent_reasoning/
backend/tests/integration/p39/
doc/review/P39*.md
```

## 允许修改文件

```text
backend/CMakeLists.txt
backend/tests/CMakeLists.txt
backend/src/world_interaction/world_services.cpp
backend/include/pathfinder/world_interaction/world_services.h
backend/src/h5_playable/playable_projection_mapper.cpp
frontend/h5_playable/app.js
frontend/h5_playable/style.css
```

修改现有文件只能用于接入 P39 推理结果和安全投影，不得重写 P38 世界交互架构。

## 禁止事项

```text
禁止 if hunger then eat red_berry。
禁止 if cold then build_house。
禁止 if beast then use_torch 作为新增主逻辑。
禁止 AgentReasoner 直接修改 WorldSnapshot。
禁止读取隐藏 ObjectDefinition 真相给 Agent 决策。
禁止 H5 推导计划或规则结果。
禁止无上限递归搜索。
禁止把 utility_score 原始值暴露给玩家。
禁止跳过 P27 条件表达式。
禁止跳过 P38 WorldChangeApplier。
```

## 核心类

```text
EffectSemanticsRegistry
AgentGoalGenerator
KnowledgeActionCandidateBuilder
PlanPreconditionResolver
PlanUtilityScorer
AgentReasoner
AgentPlanExecutorAdapter
ReasoningProjectionMapper
```

## 核心测试

```bash
cmake --build build/backend --target pathfinder_tests_agent_reasoning pathfinder_tests_h5_playable pathfinder_tests_world_interaction -j2
ctest --test-dir build/backend -R "agent_reasoning|world_interaction|h5_playable" --output-on-failure
```

## 最小验收案例

```text
同伴知道红果 restore_hunger，hunger 高时推理出吃红果。
同伴不知道红果知识时，不得凭空吃红果。
寒冷紧急时，在生火和造房之间优先生火。
房子容量不足时，能规划 build_house，并链式推出 cut_wood、restore_sharpness。
野兽威胁高时，能通过 repel_beast 语义选择火把反制。
循环依赖和搜索超限必须有诊断。
```
