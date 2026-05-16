# Agent 模块

P5 只定义数据契约和适配器，不做完整 AI。

## 职责

- 定义 Agent 类型契约（AgentDefinition）
- 定义 Agent 绑定契约（AgentBinding）
- 定义 Agent 观察契约（AgentObservation）
- 定义 Agent 动作契约（AgentActionSpace）
- 定义 Agent 意图契约（AgentIntent）
- 实现 AgentIntent -> CommandEnvelope 适配器

## 禁止

- 不实现 AgentRuntime（完整调度器）
- 不实现 AgentPolicy（策略算法）
- 不实现 ObservationBuilder / ActionSpaceBuilder
- 不依赖 pipeline / rules / state / event
