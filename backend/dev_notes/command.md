# command module dev notes

## P1 任务记录

### TASK-P1-001: 创建 command 目录与 CMake 目标

状态: 完成

创建内容:
- backend/include/pathfinder/command/
- backend/src/command/
- backend/tests/unit/command/

验证:
- cmake -S backend -B build/backend 通过
- cmake --build build/backend 通过
- ctest 全部 20 个测试通过 (10 foundation + 4 config + 6 command)

### TASK-P1-005: 实现命令基础枚举

状态: 完成

实现内容:
- CommandSource: player / ai / system / replay / test
- CommandIntent: unknown / experiment / exploit / teach / avoid_risk / combine / migrate / fight
- ActionTargetType: none / object / entity / location / region / knowledge / group
- ActionTargetRole: primary / secondary / tool / material / receiver / destination
- 每个枚举提供 toString
- 每个枚举提供 fromString，返回 Result<T>，未知字符串返回失败

测试:
- 每个枚举 toString 稳定
- 合法字符串可解析
- 非法字符串解析失败

### TASK-P1-006: 实现 ActionTarget

状态: 完成

实现内容:
- ActionTarget: target_type, target_id, role
- validateBasic(): 检查 target_type 不是 none、target_id 格式合法、role 合法

测试:
- 合法 object target 通过
- none target_type 失败
- 空 target_id 失败
- 非法 target_id 格式失败
- 不同 role 可以通过

### TASK-P1-007: 实现 ActionCommand 最小结构

状态: 完成

实现内容:
- ActionId: 基于 StrongId<ActionIdTag>
- ActorId: 基于 StrongId<EntityIdTag>
- ActionCommand: action_id, actor_id, targets, intent
- validateBasic(): 检查 action_id 合法、actor_id 合法、targets 非空、每个 target 基础校验通过

测试:
- 合法 ActionCommand 通过
- 缺 action_id 失败
- 缺 actor_id 失败
- 空 targets 失败
- 非法 target 传播失败
- intent unknown 允许通过

### TASK-P1-008: 实现 CommandEnvelope

状态: 完成

实现内容:
- CommandId: 基于 StrongId<CommandIdTag>
- CommandEnvelope: command_id, source, issued_tick, idempotency_key, correlation_id, payload
- validateBasic(): 检查 command_id 合法、payload 基础校验通过、key 长度限制

测试:
- 合法 envelope 通过
- 缺 command_id 失败
- 非法 payload 失败
- 空 idempotency_key 允许
- 过长 idempotency_key 失败

### TASK-P1-009: 实现 CommandValidationReport

状态: 完成

实现内容:
- CommandValidationSeverity: warning / error
- CommandValidationIssue: severity, code, message, field_path
- CommandValidationReport: addIssue, hasErrors, issueCount, issues
- validateCommandEnvelopeBasic(envelope): 调用 envelope.validateBasic 并包装结果

测试:
- 合法 envelope 生成空错误报告
- 非法 envelope 生成 error issue
- warning 不算 error
- issueCount 正确

### TASK-P1-FIX-001: P1 ID 与 source 合规修复

状态: 完成

修复内容:
1. foundation 增加 TargetIdTag / TargetId
2. ActionTarget.target_id 改为 foundation::TargetId
3. ActionCommand.action_id 改为 foundation::ActionId（不再重复 using）
4. ActionCommand.actor_id 改为 foundation::EntityId（不再重复 using）
5. CommandEnvelope.command_id 改为 foundation::CommandId（不再重复 using）
6. CommandSource 增加 Unknown
7. CommandEnvelope::validateBasic 拒绝 Unknown source
8. 更新所有相关测试

验证:
- rg -n "using ActionId =|using CommandId =" backend/include/pathfinder/command backend/src/command 无输出
- rg -n "std::string target_id" backend/include/pathfinder/command backend/src/command 无输出
- 全量测试 20/20 通过
- command_envelope_test 包含非法 source 失败用例
