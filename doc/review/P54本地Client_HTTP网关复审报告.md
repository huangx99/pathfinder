# P54 本地 Client HTTP 网关复审报告

## 复审结论

**通过。**

本轮修复已经覆盖上次审核提出的阻塞问题：

```text
1. P54 CTest 固定端口失败问题已修复。
2. 已新增真实 socket HTTP smoke test。
3. /api/client/* 非 POST 错误路径已返回 JSON 405。
4. 错误 JSON 已改为 nlohmann::json 生成。
5. static_root 下 .json 白名单已收窄为 manifest.json。
6. client_server_main.cpp 中无用 router 对象已移除。
```

当前 P54 可以作为“本地 Client HTTP 网关”阶段验收，但仍建议把少量 HTTP 健壮性问题放入后续 hardening 阶段处理。

## 本次复审范围

- `backend/include/pathfinder/client_http/`
- `backend/src/client_http/`
- `backend/apps/client_server_main.cpp`
- `backend/tests/unit/client_http/`
- `backend/tests/integration/client_http/`
- `backend/CMakeLists.txt`
- `backend/tests/CMakeLists.txt`
- `doc/84_P54本地Client_HTTP网关任务卡设计.md`

## 验证命令与结果

### 构建

```bash
cmake --build build/backend --target pathfinder_client_server_exe pathfinder_tests_client_http pathfinder_tests_client_http_integration -j 2
```

结果：通过。

### P54 定向测试，两轮重复验证

```bash
ctest --test-dir build/backend -R '^(client_http_|client_static_file_|architecture_client_http_)' --output-on-failure
ctest --test-dir build/backend -R '^(client_http_|client_static_file_|architecture_client_http_)' --output-on-failure
```

结果：两轮均通过。

```text
24/24 passed
24/24 passed
```

### P53 客户端协议回归

```bash
ctest --test-dir build/backend -R '^(client_protocol_|architecture_client_protocol_)' --output-on-failure
```

结果：通过。

```text
41/41 passed
```

### P43-P52 世界系统回归

```bash
ctest --test-dir build/backend -R '^(world_command_|world_runtime_|world_inventory_|world_generation_|world_harvest_|world_reaction_|world_learning_|world_teaching_|world_agent_execution_|world_beast_ecology_)' --output-on-failure
```

结果：通过。

```text
237/237 passed
```

## 上次阻塞问题复查

### 阻塞 1：P54 测试套件在 CTest 下失败

结论：已修复。

复查结果：

```text
client_http_bootstrap_roundtrip 不再启动固定端口 19991。
该用例改为 router 层 bootstrap roundtrip。
P54 定向测试在 CTest 下连续两轮通过。
```

相关位置：

```text
backend/tests/integration/client_http/client_http_integration_test.cpp:157
backend/tests/integration/client_http/client_http_integration_test.cpp:159
```

### 阻塞 2：真实 socket HTTP 链路没有被有效测试

结论：已修复。

复查结果：

```text
新增 client_http_socket_bootstrap_smoke。
测试使用 port 0 由系统分配可用端口。
测试通过真实 TCP/HTTP 请求访问 POST /api/client/bootstrap。
测试通过真实 TCP/HTTP 请求访问 GET /、GET /app.js、GET /style.css。
```

相关位置：

```text
backend/tests/integration/client_http/client_http_integration_test.cpp:385
backend/tests/integration/client_http/client_http_integration_test.cpp:401
backend/tests/integration/client_http/client_http_integration_test.cpp:412
backend/tests/integration/client_http/client_http_integration_test.cpp:420
backend/tests/CMakeLists.txt
```

这已经能证明：

```text
浏览器/HTTP 客户端
  -> ClientSocketHttpTransport
  -> ClientHttpRouter
  -> ClientHttpGateway
  -> P53 Client Protocol
  -> WorldCommandPipeline
```

最小链路可跑通。

### 阻塞 3：API 错误响应路径不完全符合 JSON 契约

结论：已修复。

复查结果：

```text
ClientHttpRouter 现在先识别 /api/client/*。
/api/client/* 非 POST 返回 application/json 405。
新增 client_http_api_get_returns_405_json 测试覆盖。
```

相关位置：

```text
backend/src/client_http/client_http_router_impl.cpp:14
backend/src/client_http/client_http_router_impl.cpp:16
backend/src/client_http/client_http_router_impl.cpp:30
backend/tests/integration/client_http/client_http_integration_test.cpp:333
backend/tests/CMakeLists.txt
```

## 上次非阻塞问题复查

### 问题 1：错误 JSON 手工拼接

结论：已修复。

复查结果：

```text
ClientHttpGateway::makeJsonError 已使用 nlohmann::json 生成错误响应。
字符串转义风险已消除。
```

相关位置：

```text
backend/src/client_http/client_http_gateway.cpp:136
backend/src/client_http/client_http_gateway.cpp:145
```

### 问题 2：静态文件 .json 白名单过宽

结论：已修复。

复查结果：

```text
ClientStaticFileService 现在只允许 manifest.json。
不再允许 static_root 下任意 .json。
```

相关位置：

```text
backend/src/client_http/client_static_file_service.cpp:97
backend/src/client_http/client_static_file_service.cpp:98
```

### 问题 3：main 中无用 router 对象

结论：已修复。

复查结果：

```text
client_server_main.cpp 中无用局部 router 已移除。
ClientHttpServer 只持有实际传入的 router。
```

相关位置：

```text
backend/apps/client_server_main.cpp:34
backend/apps/client_server_main.cpp:36
```

## 架构复查结论

当前 P54 架构符合设计要求：

```text
1. client_http 模块未使用 h5_v2 / h5_playable 后端命名。
2. ClientHttpGateway 不依赖 sys/socket、netinet/in、arpa/inet。
3. ClientSocketHttpTransport 不直接依赖 ClientProtocolCodec。
4. HTTP 层不生成红果、狼、火把等玩法内容。
5. HTTP 层不直接读取 content/core JSON。
6. HTTP 层不直接修改 world runtime 内部数组。
7. command 仍通过 P53 ClientCommandGateway 与 WorldCommandPipeline。
8. transport 通过 IClientHttpTransport 可替换，后续接第三方 HTTP 库不用改业务层。
```

## 仍建议后续优化的问题

以下问题不阻塞 P54 验收，但建议进入后续 HTTP hardening 或客户端联调阶段：

### 建议 1：socket smoke test 不应静默 SKIP

位置：

```text
backend/tests/integration/client_http/client_http_integration_test.cpp:401
backend/tests/integration/client_http/client_http_integration_test.cpp:403
backend/tests/integration/client_http/client_http_integration_test.cpp:405
```

当前如果 `server.start(0, "127.0.0.1")` 失败，测试会打印 SKIP 并 return，CTest 会认为通过。

建议后续改为：

```text
真实 socket smoke test 启动失败应视为测试失败。
否则未来 socket 层坏了可能被隐藏。
```

本次不阻塞原因：本轮实际运行中该测试已真实通过，且 P54 定向测试两轮稳定通过。

### 建议 2：HTTP 解析器仍是最小实现

位置：

```text
backend/src/client_http/client_socket_http_transport.cpp:135
backend/src/client_http/client_socket_http_transport.cpp:142
```

当前 `readRequest` 对 `Content-Length:` 的识别是大小写敏感的，且没有复杂 HTTP 特性支持，例如 chunked body、连接超时、慢请求防护。

P54 是本地单机轻量网关，这可以接受；但如果后续要远程部署或公网访问，应优先选择新的 `IClientHttpTransport` 实现，例如基于成熟 HTTP 库的 transport。

### 建议 3：API 405 JSON 在 Router 中有一处重复拼接

位置：

```text
backend/src/client_http/client_http_router_impl.cpp:30
backend/src/client_http/client_http_router_impl.cpp:34
```

当前 `/api/client/*` 非 POST 的 JSON 405 在 Router 中直接写字符串。功能正确，但如果后续错误信封格式变化，Router 与 Gateway 可能需要同步修改。

建议后续把 API 错误响应生成提成公共 helper，避免重复。

## 最终判断

P54 本轮复审通过。

它现在已经能作为 V2.0 客户端接入后端的最小 HTTP 网关基础：

```text
客户端静态文件
  -> 本地 HTTP Gateway
  -> P53 Client Protocol
  -> WorldCommandPipeline
  -> 世界 Runtime / 认知 / 学习 / 教学 / Agent 系统
```

后续可以进入下一阶段，但建议不要把 P54 当前 socket 实现当作公网级 HTTP 服务；如果未来要服务器远程游玩，应通过 `IClientHttpTransport` 增加更强的 transport 实现。
