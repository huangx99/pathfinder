# protocol 模块开发笔记

## 模块职责

定义对外协议消息结构，包装后端结果为稳定外壳。

## 公开类型

- ProtocolMessageType: Unknown / Request / Response / Event / Ack / Heartbeat / Error / Progress
- ProtocolDomain: Unknown / Command / Query / EventStream / ProjectionSync / Save / Replay / Config / Error / Health
- ProtocolPayloadType: Unknown / CommandResult / EventList / ReplayReport / ErrorList / Text
- ProtocolPayload: payload_type + message_key + debug_text
- ProtocolError: code + message_key + debug_text
- ProtocolWarning: code + message_key + debug_text
- ProtocolEnvelope: 统一消息外壳
- CommandResultProtocolSummary: 命令结果摘要
- CommandResponseOptions: 构建响应选项
- buildCommandResponseEnvelope: PipelineResult -> ProtocolEnvelope 适配器

## 禁止依赖

- HTTP / WebSocket 库
- JSON 库
- frontend
- engine 内部类型 (StateChangeDraft, Resolver)

## 已实现

- ProtocolMessageType / ProtocolDomain / ProtocolPayloadType 枚举 + toString/fromString (P4)
- ProtocolEnvelope + validateBasic (P4)
- ProtocolPayload / ProtocolError / ProtocolWarning (P4)
- buildCommandResponseEnvelope 适配器 (P4)
- buildCommandResponseEnvelope 设置 state_version (从最后一个事件的 state_version 推导)
- 18 个单元测试通过

## 未实现

- JSON 序列化
- HTTP 服务
- WebSocket 服务
- 完整投影系统

## 测试命令

```bash
ctest --test-dir build/backend -R protocol --output-on-failure
```

## 架构约束

- ProtocolEnvelope 不暴露 StateChangeDraft
- ProtocolEnvelope 不暴露 Resolver 内部结构
- 错误只输出 ErrorDetail 摘要，不暴露内部栈
- P4 只做纯数据结构，不做序列化
- Unknown 值不允许从字符串正常解析为有效业务值
