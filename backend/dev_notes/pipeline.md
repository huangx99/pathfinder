# pipeline 模块开发记录

## P2 状态

- TASK-P2-013: P2 聚合边界检查
- 状态: 完成

## 任务历史

### TASK-P2-001: 创建目录与 CMake 空目标

- 创建 backend/include/pathfinder/pipeline/ 目录
- 创建 backend/src/pipeline/ 目录
- 创建 backend/tests/unit/pipeline/ 目录
- 创建最小 types.h / types.cpp 占位
- 创建 pipeline_smoke_test.cpp
- 更新 CMakeLists.txt 添加 pathfinder_pipeline 库
- 更新 tests/CMakeLists.txt 添加 pipeline 测试

### TASK-P2-009: 实现 PipelineStage / PipelineStep

- 实现 PipelineStage: AcceptCommand / ValidateCommand / PrepareContext / ResolveRules / ValidateStateChanges / ApplyStateChanges / BuildEvents / EmitEvents / Finish
- 实现 PipelineStatus: NotStarted / Running / Succeeded / Failed
- 实现 PipelineStep: stage / order / name
- 提供 defaultPipelineSteps()
- 默认步骤按 order 递增
- 创建 pipeline_types_test.cpp 测试
- 验收命令通过: ctest -R pipeline

### TASK-P2-010: 实现 PipelineContext

- 实现 PipelineContext: command / state_metadata / config_version / random_seed / correlation_id
- 提供 validateBasic()
- 验证: command 合法、state_id 非空
- 创建 pipeline_context_test.cpp 测试
- 验收命令通过: ctest -R pipeline

### TASK-P2-011: 实现 PipelineResult

- 实现 PipelineResult: status / executed_steps / state_changes / events / errors
- 提供 successEmpty()
- 提供 hasErrors() / validateBasic()
- 验证: status 合法、executed_steps 顺序合法、state_changes 合法、events 合法
- 创建 pipeline_result_test.cpp 测试
- 验收命令通过: ctest -R pipeline

### TASK-P2-012: 实现 RulePipeline 空执行器

- 实现 RulePipeline: execute(context) -> Result<PipelineResult>
- execute 先校验 context
- execute 记录 executed_steps
- execute 返回 PipelineStatus::Succeeded 的空结果
- execute 不修改输入 GameState
- execute 不执行任何真实规则
- 创建 rule_pipeline_test.cpp 测试
- 验收命令通过: ctest -R pipeline

### TASK-P2-013: P2 聚合边界检查

- 确认 pipeline 测试通过 (5/5)
- 确认未实现完整 RuleEngine / ActionResolver / InteractionResolver
- 确认 Pipeline 没有 eat/use/combine 特判
- 全量测试 33/33 通过

## P3 状态

- P3 实现完成，审核通过

### TASK-P3-013: 接入 RulePipeline P3 执行

- PipelineContext 增加 game_state 指针，用于承载可变 GameState
- RulePipeline::execute 对 eat 命令调用 EatObjectResolver
- Pipeline 先获得 draft，再调用 StateChangeGate (通过 MinimalStateApplier)
- Pipeline 通过 MinimalStateApplier 应用 state changes
- Pipeline 通过 CognitionUpdater 应用 evidence
- Pipeline 生成 StateChanged 和 CognitionUpdated 事件
- 非 eat 命令或无 game_state 时返回 P2 successEmpty

### P3 执行链路

```text
CommandEnvelope
  -> EatObjectResolver::canResolve (检查 action_id == eat)
  -> EatObjectResolver::resolve (生成 draft: StateChangeSet + events + evidence)
  -> MinimalStateApplier::apply (内部调用 StateChangeGate::validate)
  -> CognitionUpdater::applyEvidence (内部调用 validateBasic)
  -> 生成 StateChanged + CognitionUpdated 事件
  -> 返回 PipelineResult
```

### P3 边界

- 不是完整 RuleEngine
- 不是动态 resolver 注册系统
- 不是协议输出
- 不是 Agent 决策
- P3 只支持 unknown_fruit/eat，其他 action 返回空成功
- StateChangeGate 由 MinimalStateApplier 统一调用
- 验收命令: ctest -R pipeline
