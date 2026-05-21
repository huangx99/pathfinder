# P54 本地 Client HTTP 网关审核报告

## 审核结论

**不通过，需修复后复审。**

P54 的模块边界总体方向正确：新增了 `client_http` 独立模块、`IClientHttpTransport` / `IClientHttpRouter` 可插拔边界、`ClientHttpGateway` 复用 P53 `ClientProtocolCodec`，并且未发现 HTTP 层直接生成玩法、知识或读取 content/core JSON 的硬编码路径。

但是当前实现还不能验收，原因是 P54 自身测试在 CTest 下失败，并且最关键的 socket 传输路径没有被真实集成测试覆盖。P54 是前端接真实后端的入口层，这一阶段不能接受“不稳定测试”或“只测 router、不测真实 HTTP”的状态。

## 本次审核范围

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

### P54 定向测试

```bash
ctest --test-dir build/backend -R '^(client_http_|client_static_file_|architecture_client_http_)' --output-on-failure
```

结果：失败。

失败用例：

```text
client_http_bootstrap_roundtrip
```

失败位置：

```text
backend/tests/integration/client_http/client_http_integration_test.cpp:80
assert(result.is_ok())
```

失败现象：

```text
95% tests passed, 1 tests failed out of 22
```

说明：该用例在 CTest 下启动 `ClientHttpServer` 监听固定端口 `19991`，`server.start(19991, "127.0.0.1")` 返回错误并触发断言。即使该用例偶尔在直接执行二进制时可以通过，验收仍必须以 CTest 稳定通过为准。

### P53 回归

```bash
ctest --test-dir build/backend -R '^(client_protocol_|architecture_client_protocol_)' --output-on-failure
```

结果：通过，41/41。

### P43-P52 世界系统回归

```bash
ctest --test-dir build/backend -R '^(world_command_|world_runtime_|world_inventory_|world_generation_|world_harvest_|world_reaction_|world_learning_|world_teaching_|world_agent_execution_|world_beast_ecology_)' --output-on-failure
```

结果：通过，237/237。

## 阻塞问题

### 阻塞 1：P54 测试套件在 CTest 下失败

位置：

```text
backend/tests/integration/client_http/client_http_integration_test.cpp:71
backend/tests/integration/client_http/client_http_integration_test.cpp:79
backend/tests/integration/client_http/client_http_integration_test.cpp:80
```

问题：

```text
test_bootstrap_roundtrip 启动真实 ClientHttpServer，但使用固定端口 19991。
CTest 下 server.start 返回错误，导致断言失败。
```

影响：

```text
P54 无法满足“测试稳定通过”的最低验收要求。
后续研发会无法判断 HTTP 网关是真坏了，还是测试端口/生命周期不稳定。
```

修复要求：

```text
1. 不允许保留不稳定固定端口集成测试。
2. 如果该用例只是验证 router/bootstrap roundtrip，应移除真实 server.start。
3. 如果要验证真实 socket，应使用可控端口策略，并在失败时输出具体错误。
4. 测试必须在 ctest 定向运行和全量运行中都稳定通过。
```

### 阻塞 2：真实 socket HTTP 链路没有被有效测试

位置：

```text
backend/tests/integration/client_http/client_http_integration_test.cpp
backend/tests/unit/client_http/client_http_test.cpp:70
backend/tests/unit/client_http/client_http_test.cpp:87
backend/src/client_http/client_socket_http_transport.cpp
```

问题：

```text
多数集成测试直接构造 ClientHttpRequest 并调用 router.route(req)。
单元测试 parse_get_request / parse_post_json_request 只是手动构造 DTO，并没有调用 ClientSocketHttpTransport 的解析逻辑。
当前测试没有真正证明：浏览器/HTTP 客户端 -> socket transport -> router -> gateway -> P53 protocol 的完整链路可用。
```

影响：

```text
P54 的核心目标是“让客户端通过 HTTP 访问真实后端”。
如果不测真实 HTTP 请求，后续前端可能仍然无法连接，而测试却显示通过。
```

修复要求：

```text
至少新增一个真实 socket 集成测试：
1. 启动 ClientHttpServer。
2. 通过 TCP/HTTP 请求 POST /api/client/bootstrap。
3. 验证 HTTP 状态码、Content-Type、响应 JSON 中 session_id / available_commands。
4. 验证 GET /、GET /app.js、GET /style.css 能从 static_root 返回。
5. 测试结束必须可靠 stop server。
```

如果不想在单测里直接用真实端口，也可以把 HTTP 解析器拆成可测试的小模块，但仍需要至少一个真实 socket smoke test。

### 阻塞 3：API 错误响应路径不完全符合 `/api/client/*` JSON 契约

位置：

```text
backend/src/client_http/client_http_router_impl.cpp:26
backend/src/client_http/client_http_router_impl.cpp:27
backend/src/client_http/client_http_router_impl.cpp:42
```

问题：

```text
router 对所有 GET 请求先走 static_files_.serve(request.path)。
因此 GET /api/client/bootstrap 这类 API 错误路径会返回 text/plain 404，而不是稳定 JSON 405/404。
```

与设计冲突：

```text
doc/84_P54本地Client_HTTP网关任务卡设计.md 明确要求：所有 /api/client/* 错误必须返回 JSON，不返回纯文本。
```

影响：

```text
前端统一错误处理会被破坏。
调试时 API 路径错误可能变成静态资源错误，定位困难。
```

修复要求：

```text
router 应先识别 /api/client/* 路径，再判断 method。
/api/client/* 非 POST 应返回 JSON 405。
未知 /api/client/* POST 应返回 JSON 404。
非 API GET 才交给静态文件服务。
```

## 非阻塞问题

### 问题 1：错误 JSON 手工拼接，缺少字符串转义

位置：

```text
backend/src/client_http/client_http_gateway.cpp:135
backend/src/client_http/client_http_gateway.cpp:136
backend/src/client_http/client_http_gateway.cpp:140
```

问题：

```text
makeJsonError 直接拼接 error_key、message、details。
如果 message 中包含引号、反斜杠或控制字符，响应 JSON 会损坏。
```

建议：

```text
使用 nlohmann::json 或独立 jsonEscape helper 生成错误信封。
```

### 问题 2：静态文件 `.json` 白名单过宽

位置：

```text
backend/src/client_http/client_static_file_service.cpp:83
backend/src/client_http/client_static_file_service.cpp:97
```

问题：

```text
当前允许 static_root 下任意 .json。
设计里写的是 .json 仅允许客户端自身 manifest，不允许 content/core。
```

当前由于 static_root 默认指向 app/frontend，且 content/core 被显式拒绝，风险可控；但后续如果 static_root 配错，可能扩大暴露面。

建议：

```text
限制 .json 只允许 manifest.json 或明确的客户端资源清单路径。
```

### 问题 3：启动 main 中存在无用 router 对象

位置：

```text
backend/apps/client_server_main.cpp:36
```

问题：

```text
main 中先构造了一个局部 ClientHttpRouter router，但实际传给 ClientHttpServer 的是另一个 make_unique<ClientHttpRouter>。
```

建议：

```text
删除无用局部变量，避免后续误以为它被 server 使用。
```

## 已通过项

```text
1. 新增 client_http 模块命名没有继续使用 h5_v2 / h5_playable 后端命名。
2. ClientHttpGateway 未直接依赖 socket / recv / send / bind / listen。
3. ClientSocketHttpTransport 未直接依赖 ClientProtocolCodec。
4. command 请求进入 ClientCommandGateway，不在 HTTP 层自行执行玩法。
5. P53 client_protocol 回归 41/41 通过。
6. P43-P52 世界系统回归 237/237 通过。
7. 默认没有引入第三方 HTTP 框架。
8. 静态服务已拒绝 content/core、backend、doc、.git 等明显危险路径。
```

## 修复后复审要求

研发修复后，请至少提供以下验证结果：

```text
1. P54 定向测试全部通过。
2. P53 client_protocol 回归全部通过。
3. P43-P52 世界系统回归全部通过。
4. 新增真实 socket HTTP smoke test。
5. /api/client/* 错误路径统一返回 JSON。
6. CTest 重复运行 P54 测试不再出现端口或生命周期不稳定。
```

推荐复审命令：

```bash
cmake --build build/backend --target pathfinder_client_server_exe pathfinder_tests_client_http pathfinder_tests_client_http_integration -j 2
ctest --test-dir build/backend -R '^(client_http_|client_static_file_|architecture_client_http_)' --output-on-failure
ctest --test-dir build/backend -R '^(client_protocol_|architecture_client_protocol_)' --output-on-failure
ctest --test-dir build/backend -R '^(world_command_|world_runtime_|world_inventory_|world_generation_|world_harvest_|world_reaction_|world_learning_|world_teaching_|world_agent_execution_|world_beast_ecology_)' --output-on-failure
```

## 最终判断

P54 当前方向正确，但不能验收。

必须先修复 P54 CTest 失败和真实 HTTP 链路测试缺失问题，否则后续前端接入会建立在不稳定网关上，风险较高。
