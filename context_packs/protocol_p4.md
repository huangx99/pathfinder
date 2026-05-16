# protocol_p4 Context Pack

## P4 目标

实现最小协议外壳，让后端结果可以被包装成稳定的 ProtocolEnvelope，给未来 H5 / Agent / 工具消费。

## 允许实现

- ProtocolMessageType / ProtocolDomain / ProtocolPayloadType 枚举
- ProtocolPayload / ProtocolError / ProtocolWarning
- ProtocolEnvelope + validateBasic
- CommandResultProtocolSummary
- buildCommandResponseEnvelope (PipelineResult -> ProtocolEnvelope 适配)

## 禁止实现

- HTTP 服务
- WebSocket 服务
- H5 前端页面
- JSON 序列化
- 网络库依赖
- 完整投影系统
- AgentRuntime

## 关键约束

- ProtocolEnvelope 不暴露 StateChangeDraft 给前端
- ProtocolEnvelope 不暴露 Resolver 内部结构
- 错误只输出 ErrorDetail 摘要，不暴露内部栈
- 不引入 JSON 库或网络库
- P4 只做纯数据结构，不做序列化

## 依赖模块

- foundation: StrongId, Result, ErrorDetail, StateVersion, ConfigVersionId
- command: CommandEnvelope
- pipeline: PipelineResult, PipelineStatus

## 验收命令

```bash
cmake -S backend -B build/backend
cmake --build build/backend
ctest --test-dir build/backend -R protocol --output-on-failure
ctest --test-dir build/backend --output-on-failure
```

## 测试清单

- ProtocolMessageType toString/fromString 稳定
- ProtocolDomain toString/fromString 稳定
- ProtocolPayloadType toString/fromString 稳定
- 非法字符串 fromString 失败
- Unknown 不能作为合法业务输入
- 合法 command response envelope 通过
- 缺 protocol_version 失败
- 缺 message_id 失败
- message_id 格式非法失败
- message_type Unknown 失败
- domain Unknown 失败
- payload_type Unknown 失败
- 带 ProtocolError 的 response 可通过
- 成功 PipelineResult 可生成合法 ProtocolEnvelope
- 失败 PipelineResult 可生成带 errors 的合法 ProtocolEnvelope
- Envelope 不包含 StateChangeDraft 结构
