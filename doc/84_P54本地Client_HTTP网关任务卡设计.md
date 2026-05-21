# P54 本地 Client HTTP 网关任务卡设计

## 1. 阶段定位

P54 是 V2.0 开放世界路线中“本地客户端接入后端引擎”的阶段。

它接在 P53 客户端协议之后，目标不是继续设计 UI，而是让 `app/frontend/client` 这类客户端可以通过统一 HTTP 接口真正访问后端 `Client Protocol`、`WorldCommand`、世界状态和认知系统。

一句话定位：

```text
P54 在后端新增一个轻量、本地、无第三方依赖的 Client HTTP Gateway，把浏览器请求转换为 P53 Client Protocol 调用，并服务 app/frontend 下的静态客户端文件。
```

P54 完成后，客户端访问链路应变成：

```text
浏览器客户端 app/frontend/client
  ↓ fetch('/api/client/bootstrap|command|refresh|reset')
后端 Client HTTP Gateway
  ↓ ClientProtocolCodec 解码 / 编码
ClientSessionGateway / ClientCommandGateway
  ↓
WorldCommandPipeline
  ↓
世界 Runtime / 背包 / 采集 / 制作 / 学习 / 教学 / Agent
```

## 2. 为什么现在做

当前 `app/frontend/client` 已经有了地图画布、HUD、命令按钮、事件日志和 mock 模式，也已经调用以下接口：

```text
POST /api/client/bootstrap
POST /api/client/command
POST /api/client/refresh
POST /api/client/reset
```

但后端目前还没有正式的通用 `client_http` 服务。结果是：

```text
前端只能进入 mock 模式。
无法证明 P53 协议能驱动真实引擎。
P54 UI 容易继续为了“能玩”而写死模拟逻辑。
后续客户端可能重复造不同的 HTTP / API 层。
```

所以必须先补一个后端 Gateway，让前端变成“真实协议客户端”，而不是“本地模拟器”。

## 3. 本阶段解决什么问题

P54 只解决以下问题：

```text
1. 提供本地 HTTP 服务，默认监听 1999。
2. 服务 app/frontend/client 静态文件。
3. 提供 /api/client/bootstrap。
4. 提供 /api/client/command。
5. 提供 /api/client/refresh。
6. 提供 /api/client/reset。
7. 请求和响应全部复用 P53 ClientProtocolCodec。
8. command 必须通过 ClientCommandGateway 和 WorldCommandPipeline。
9. 不让前端读取 content/json，不让前端直接访问引擎内部对象。
10. 建立最小 HTTP 集成测试，证明浏览器协议能走通真实后端。
```

## 4. 本阶段不解决什么问题

P54 不做以下事情：

```text
不重构前端 UI。
不设计正式游戏界面。
不引入 WebSocket。
不引入第三方 HTTP 框架。
不做公网服务安全。
不做账号系统。
不做多人在线。
不做 TLS / HTTPS。
不做数据库。
不做热更新。
不让 HTTP 层生成玩法规则。
不让 HTTP 层读取 content/core JSON。
不把 HTTP 层写进 client 旧命名空间。
```

正式的玩家界面体验、中文化、横屏布局和游戏化 HUD，可以在 HTTP Gateway 稳定后进入后续客户端阶段继续做。

## 5. 头脑风暴：未来可能遇到的问题

### 5.1 单机浏览器客户端

玩家用浏览器打开 `http://内网地址:1999`，本机或局域网后端处理所有规则。

风险：

```text
前端一旦连不上后端会进入 mock，玩家误以为正在玩真实系统。
```

设计要求：

```text
HTTP Gateway 上线后，正式模式必须能明确显示真实连接状态。
mock 只能用于开发，不得作为验收依据。
```

### 5.2 手机通过内网穿透访问

玩家手机访问 1999 端口，后端仍在电脑上。

风险：

```text
host 不能只绑定 127.0.0.1。
但也不能误操作 frp / 内网穿透进程。
```

设计要求：

```text
服务默认 host 可配置，测试可用 127.0.0.1，手动试玩可用 0.0.0.0。
启动脚本只动本服务，不动 frp。
```

### 5.3 前端刷新页面

浏览器刷新后 `client_id/session_id` 可能来自 localStorage。

风险：

```text
后端进程重启后，前端保留旧 session_id，后端找不到 session。
```

设计要求：

```text
bootstrap(create_if_missing=true) 必须能恢复或重建 session。
session_not_found 时前端应重新 bootstrap。
```

### 5.4 旧 projection 提交命令

玩家网络延迟或连续点击，提交了旧 `known_projection_version`。

风险：

```text
旧命令污染世界状态。
```

设计要求：

```text
HTTP Gateway 不绕过 P53；由 ClientCommandGateway 拦截 stale version，不执行命令。
```

### 5.5 连续点击按钮

玩家双击或手机误触可能连续发 command。

风险：

```text
同一个按钮造成重复采集、重复移动、重复制作。
```

设计要求：

```text
前端用 pendingCommand 降低误触；后端后续用 client_sequence 防重放。
P54 至少不得在 HTTP 层自行重复执行同一请求。
```

### 5.6 静态文件路径穿越

浏览器请求 `../../content/core/...`。

风险：

```text
泄露配置、源码或本机文件。
```

设计要求：

```text
静态文件服务必须拒绝 ..、绝对路径、隐藏路径。
只允许 static_root 内的白名单扩展。
```

### 5.7 前端试图读取 JSON 配置

前端如果可以访问 `content/core`，就可能绕过认知系统。

风险：

```text
玩家提前知道红果、毒蘑菇、火把、狼等隐藏真相。
```

设计要求：

```text
HTTP Gateway 不提供 content 目录静态服务。
只提供 app/frontend 下的客户端资源。
```

### 5.8 错误响应被前端误解析

HTTP 返回非 JSON 或错误字段不稳定，前端会崩。

风险：

```text
玩家看到空白页，研发难定位。
```

设计要求：

```text
所有 /api/client/* 错误都返回稳定 JSON 错误信封。
```

### 5.9 前端目录后续会换

现在是 `app/frontend/client`，未来可能是 `app/frontend/world_client`、Godot、本地工具。

风险：

```text
HTTP Gateway 写死 client，后续又污染架构。
```

设计要求：

```text
Gateway 只叫 client_http，不叫 h5_http。
static_root 是启动参数。
默认可指向 app/frontend/client，但代码不能绑定该名称。
```

### 5.10 单机未来打包

未来可能打成桌面游戏，后端进程和客户端一起启动。

风险：

```text
如果 Gateway 依赖复杂外部服务，会影响打包。
```

设计要求：

```text
无第三方库、无数据库、无外部进程依赖，保持可嵌入。
```

## 6. 总体架构

新增后端模块：

```text
backend/include/pathfinder/client_http/
backend/src/client_http/
backend/apps/client_server_main.cpp
```

模块依赖方向：

```text
client_http_app
  -> client_http_transport
  -> client_http_router
  -> client_http_gateway
  -> client_protocol
  -> world_command
  -> foundation
```

其中必须严格区分两层：

```text
传输层：负责 socket / 第三方 HTTP 库 / 端口监听 / HTTP Request Response 转换。
业务层：负责把 ClientHttpRequest 路由到 ClientHttpGateway，再进入 P53 Client Protocol。
```

P54 当前可以用自写 socket 作为默认传输实现，但接口必须预留可插拔边界。未来如果要换 `cpp-httplib`、`Boost.Beast`、`Crow` 或平台内嵌 HTTP，只能替换传输层，不允许修改：

```text
ClientHttpGateway
ClientProtocolCodec
ClientSessionGateway
ClientCommandGateway
WorldCommandPipeline
/api/client/* 路径契约
```

禁止依赖：

```text
client_http 不依赖 h5_dialog。
client_http 不依赖 client。
client_http 不依赖 frontend/ 旧目录。
client_http 不直接依赖 content/json loader。
client_http 不直接依赖 world_runtime 内部数组。
client_http 不直接生成知识或规则。
```

可参考但不可继承旧命名：

```text
可参考 backend/src/client/playable_http_server.cpp 的 socket 读写方式。
不可把新类命名为 H5PlayableServer。
不可把新接口放进 pathfinder::client。
```

## 7. 新增类与职责

### 7.1 IClientHttpTransport

文件：

```text
backend/include/pathfinder/client_http/client_http_transport.h
```

职责：

```text
1. 定义 HTTP 传输层可插拔接口。
2. 屏蔽自写 socket 与未来第三方 HTTP 库差异。
3. 不知道 ClientProtocol，不知道 WorldCommand，不知道游戏规则。
4. 只负责监听、停止、运行状态，以及把请求交给 router。
```

接口草案：

```cpp
class IClientHttpRouter;

class IClientHttpTransport {
public:
    virtual ~IClientHttpTransport() = default;

    virtual pathfinder::foundation::Result<void> start(
        uint16_t port,
        const std::string& host,
        IClientHttpRouter& router) = 0;

    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
};
```

设计要求：

```text
P54 自写 socket 必须实现这个接口。
未来三方库也必须实现这个接口。
业务代码不得直接依赖具体 socket / httplib / beast 类型。
```

### 7.2 IClientHttpRouter

文件：

```text
backend/include/pathfinder/client_http/client_http_router.h
```

职责：

```text
1. 定义稳定路由接口。
2. 接收统一 ClientHttpRequest。
3. 返回统一 ClientHttpResponse。
4. 作为 transport 与 gateway 之间的唯一桥梁。
```

接口草案：

```cpp
class IClientHttpRouter {
public:
    virtual ~IClientHttpRouter() = default;

    virtual ClientHttpResponse route(
        const ClientHttpRequest& request) = 0;
};
```

### 7.3 ClientHttpRouter

文件：

```text
backend/include/pathfinder/client_http/client_http_router_impl.h
backend/src/client_http/client_http_router_impl.cpp
```

职责：

```text
1. 实现 IClientHttpRouter。
2. 分发 GET 静态文件请求到 ClientStaticFileService。
3. 分发 POST /api/client/* 到 ClientHttpGateway。
4. 统一处理 404 / 405 / OPTIONS。
```

接口草案：

```cpp
class ClientHttpRouter final : public IClientHttpRouter {
public:
    ClientHttpRouter(
        ClientStaticFileService& static_files,
        ClientHttpGateway& gateway);

    ClientHttpResponse route(
        const ClientHttpRequest& request) override;
};
```

### 7.4 ClientSocketHttpTransport

文件：

```text
backend/include/pathfinder/client_http/client_socket_http_transport.h
backend/src/client_http/client_socket_http_transport.cpp
```

职责：

```text
1. P54 默认自写 socket 传输实现。
2. 启动 / 停止本地 HTTP 服务。
3. 接收 TCP 连接。
4. 解析最小 HTTP 请求为 ClientHttpRequest。
5. 调用 IClientHttpRouter::route。
6. 把 ClientHttpResponse 写回 socket。
7. 不直接调用 ClientHttpGateway，不直接调用 ClientProtocol。
```

接口草案：

```cpp
class ClientSocketHttpTransport final : public IClientHttpTransport {
public:
    pathfinder::foundation::Result<void> start(
        uint16_t port,
        const std::string& host,
        IClientHttpRouter& router) override;

    void stop() override;
    bool isRunning() const override;

private:
    void acceptLoop(uint16_t port, const std::string& host);
    void handleClient(int client_fd);
};
```

### 7.5 ClientHttpServer

文件：

```text
backend/include/pathfinder/client_http/client_http_server.h
backend/src/client_http/client_http_server.cpp
```

职责：

```text
1. 作为应用层组合器，不直接实现 socket。
2. 持有 IClientHttpTransport。
3. 持有 ClientHttpRouter。
4. 对外提供 start / stop / isRunning。
5. 让 main.cpp 不关心具体传输实现。
```

接口草案：

```cpp
class ClientHttpServer {
public:
    ClientHttpServer(
        std::unique_ptr<IClientHttpTransport> transport,
        ClientHttpRouter router);

    pathfinder::foundation::Result<void> start(
        uint16_t port,
        const std::string& host);

    void stop();
    bool isRunning() const;
};
```

### 7.6 ClientHttpGateway

文件：

```text
backend/include/pathfinder/client_http/client_http_gateway.h
backend/src/client_http/client_http_gateway.cpp
```

职责：

```text
1. 处理 /api/client/bootstrap。
2. 处理 /api/client/command。
3. 处理 /api/client/refresh。
4. 处理 /api/client/reset。
5. 使用 ClientProtocolCodec 做 JSON 编解码。
6. 调用 ClientSessionGateway / ClientCommandGateway。
7. 统一转换错误响应。
```

接口草案：

```cpp
class ClientHttpGateway {
public:
    ClientHttpGateway(
        ClientProtocolCodec& codec,
        ClientSessionGateway& session_gateway,
        ClientCommandGateway& command_gateway);

    ClientHttpResponse handleApi(
        const ClientHttpRequest& request);
};
```

### 7.7 ClientHttpTypes

文件：

```text
backend/include/pathfinder/client_http/client_http_types.h
backend/src/client_http/client_http_types.cpp
```

DTO：

```cpp
enum class ClientHttpMethod {
    Unknown,
    Get,
    Post,
    Options
};

struct ClientHttpRequest {
    ClientHttpMethod method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct ClientHttpResponse {
    int status_code = 200;
    std::string content_type;
    std::map<std::string, std::string> headers;
    std::string body;
};
```

### 7.4 ClientStaticFileService

文件：

```text
backend/include/pathfinder/client_http/client_static_file_service.h
backend/src/client_http/client_static_file_service.cpp
```

职责：

```text
1. 从 static_root 读取 index.html / app.js / style.css 等静态资源。
2. 防止路径穿越。
3. 返回正确 MIME。
4. 不服务 content/、backend/、doc/、.git/。
```

允许扩展：

```text
.html
.js
.css
.png
.jpg
.jpeg
.webp
.svg
.ico
.json 仅允许客户端自身 manifest，不允许 content/core
```

### 7.5 ClientServerRuntimeFactory

文件：

```text
backend/include/pathfinder/client_http/client_server_runtime_factory.h
backend/src/client_http/client_server_runtime_factory.cpp
```

职责：

```text
1. 创建 P54 最小可运行后端依赖。
2. 装配 ClientProjectionAdapter。
3. 装配 ClientAvailableCommandAdapter。
4. 装配 ClientSessionGateway。
5. 装配 WorldCommandDispatcher / WorldCommandPipeline。
6. 装配 ClientCommandGateway。
7. 装配 ClientHttpGateway。
```

说明：

```text
如果 P53/P54 暂时只有 Wait / Inspect，runtime factory 只注册最小 command handlers。
后续 P55/P56 再逐步注册 move / gather / craft / teach / agent。
```

## 8. HTTP API 设计

### 8.1 POST /api/client/bootstrap

请求 body：

```json
{
  "client_id": "web_xxx",
  "session_id": "session_xxx",
  "requested_actor_key": "player",
  "requested_layer_key": "surface",
  "client_protocol_version": 1,
  "create_if_missing": true,
  "dev_reset_if_allowed": false
}
```

后端流程：

```text
decodeBootstrapRequest
ClientSessionGateway::bootstrap
encodeBootstrapResponse
```

### 8.2 POST /api/client/command

请求 body：

```json
{
  "session_id": "session_xxx",
  "client_id": "web_xxx",
  "client_sequence": 1,
  "known_projection_version": 1,
  "submit_mode": "option_id",
  "option_id": "opt_wait_player",
  "selection_context": {
    "target": { "target_kind": "none" },
    "selected_actor_key": "",
    "selected_entity_id": "",
    "selected_inventory_id": "",
    "selected_area_id": ""
  }
}
```

后端流程：

```text
decodeCommandRequest
ClientCommandGateway::handleCommand
encodeCommandResponse
```

禁止：

```text
HTTP 层不得自行解析 option_id 并执行动作。
HTTP 层不得自行修改 projection_version。
HTTP 层不得自行生成 knowledge。
```

### 8.3 POST /api/client/refresh

请求 body：

```json
{
  "session_id": "session_xxx",
  "client_id": "web_xxx",
  "known_projection_version": 1,
  "requested_scopes": ["full_safe_world"],
  "requested_layer_key": "surface",
  "viewport_center_x": 0,
  "viewport_center_y": 0
}
```

后端流程：

```text
decodeRefreshRequest
ClientSessionGateway::refresh
encodeRefreshResponse
```

### 8.4 POST /api/client/reset

请求 body：

```json
{
  "session_id": "session_xxx",
  "client_id": "web_xxx",
  "confirmed": true
}
```

后端流程：

```text
decodeResetRequest
ClientSessionGateway::reset
encodeResetResponse
```

### 8.5 GET 静态文件

映射：

```text
GET /              -> static_root/index.html
GET /index.html    -> static_root/index.html
GET /app.js        -> static_root/app.js
GET /style.css     -> static_root/style.css
GET /assets/...    -> static_root/assets/...
```

默认启动参数：

```text
--static-root app/frontend/client
--port 1999
--host 127.0.0.1
```

手机试玩可用：

```text
--host 0.0.0.0
```

## 9. 错误响应设计

所有 `/api/client/*` 错误必须返回 JSON，不返回纯文本。

格式：

```json
{
  "ok": false,
  "error_key": "bad_request",
  "message": "decode command request failed",
  "details": ["validation_enum_unknown"]
}
```

HTTP 状态码约定：

```text
200：协议请求成功，业务结果可能是 succeeded / blocked / failed。
400：HTTP body 无法解码、缺字段、非法 JSON。
404：路由不存在或静态文件不存在。
405：方法不允许。
413：请求体过大。
500：后端内部错误。
```

注意：

```text
WorldCommandResultKind::Blocked 不应映射成 HTTP 500。
Blocked 是正常业务结果，应返回 HTTP 200。
```

## 10. 安全边界

虽然这是单机游戏，本地 HTTP 仍必须守住最低安全边界：

```text
1. 不服务 static_root 外文件。
2. 请求体大小限制，建议 P54 先限制 1MB。
3. 只接受 GET / POST / OPTIONS。
4. API 只接受 application/json 或空 content-type 兼容本地测试。
5. 不打印完整敏感 body 到日志。
6. 不提供 content/core 静态访问。
7. 不提供 backend/doc/.git 静态访问。
8. 不执行前端传来的任意文件路径。
9. 不在 HTTP 层跳过 P53 session / option / version 校验。
10. 不操作 frp / 内网穿透进程。
```

## 11. 启动程序设计

新增：

```text
backend/apps/client_server_main.cpp
```

二进制：

```text
pathfinder_client_server
```

命令：

```bash
./build/backend/pathfinder_client_server --host 0.0.0.0 --port 1999 --static-root app/frontend/client
```

输出：

```text
Pathfinder Client Server running on http://0.0.0.0:1999
Static root: app/frontend/client
API: /api/client/bootstrap / command / refresh / reset
```

脚本建议：

```text
scripts/restart_client_1999.sh
```

脚本要求：

```text
1. 只构建 pathfinder_client_server。
2. 只停止 1999 端口上的 pathfinder_client_server。
3. 不停止 frp / frpc。
4. 不停止旧 client_server，除非确认占用的是同一个新二进制。
5. 输出 health check 结果。
```

## 12. 测试设计

### 12.1 单元测试

新增测试目录：

```text
backend/tests/unit/client_http/
```

测试项：

```text
client_http_parse_get_request
client_http_parse_post_json_request
client_http_reject_path_traversal
client_http_mime_type
client_http_error_response_json_shape
client_static_file_serves_index
client_static_file_rejects_content_core
```

### 12.2 集成测试

新增测试目录：

```text
backend/tests/integration/client_http/
```

测试项：

```text
client_http_bootstrap_roundtrip
client_http_command_option_roundtrip
client_http_refresh_roundtrip
client_http_reset_roundtrip
client_http_stale_command_returns_full_refresh
client_http_bad_json_returns_400_json
client_http_static_index_app_style
client_http_no_content_json_exposure
```

### 12.3 架构扫描

新增扫描：

```text
architecture_client_http_no_h5_namespace
architecture_client_http_no_content_json_read
architecture_client_http_no_runtime_array_mutation
architecture_client_http_no_knowledge_injection
architecture_client_http_no_third_party_http_framework
architecture_client_http_no_frontend_rule_generation
architecture_client_http_transport_boundary
```

扫描关键词建议：

```text
client::
h5_dialog::
content/core
ContentRegistry
KnowledgeClaim
knowledge_json
recipient_claim_json
.actors[
.entities[
.inventories[
cpp-httplib
boost/beast
crow_all
ClientHttpGateway.*socket
ClientHttpGateway.*recv
ClientHttpGateway.*send
ClientSocketHttpTransport.*ClientProtocolCodec
```

说明：

```text
P54 默认不引入第三方 HTTP 库，所以 no_third_party_http_framework 扫描应通过。
如果未来新增 Pxx 替换传输层，可以在该阶段移除这条限制，但必须保留 transport_boundary 扫描。
```

## 13. 验收标准

P54 必须满足：

```text
1. 能构建 pathfinder_client_server。
2. 能通过 GET / 返回 app/frontend/client/index.html。
3. 能通过 GET /app.js 返回 app/frontend/client/app.js。
4. 能通过 GET /style.css 返回 app/frontend/client/style.css。
5. POST /api/client/bootstrap 返回 P53 bootstrap JSON。
6. POST /api/client/command 真实进入 ClientCommandGateway。
7. stale projection command 不执行，返回 requires_full_refresh。
8. POST /api/client/refresh 返回 full_projection。
9. POST /api/client/reset 重建 session。
10. API 错误返回稳定 JSON。
11. 静态文件不能越界访问 content/core。
12. 不新增 h5_* 正式模块。
13. P54 默认实现不引入第三方 HTTP 库。
14. HTTP 传输层通过 IClientHttpTransport 可替换。
15. ClientHttpGateway 不直接依赖 socket / recv / send / 第三方 HTTP 类型。
16. 替换传输层不需要修改 ClientHttpGateway 和 P53 Client Protocol。
17. P53 client_protocol 测试仍全过。
18. P43-P52 回归测试仍全过。
```

## 14. 与当前前端的关系

当前前端目录：

```text
app/frontend/client/
```

P54 Gateway 应该能服务这个目录，但不能把后端模块命名为 `client`。

当前前端仍有以下问题，不属于 P54 必须修复：

```text
1. UI 仍偏调试台。
2. 英文较多，不是正式中文界面。
3. mock 模式较强，容易掩盖真实后端问题。
4. 地图 DTO 与 P53 projection fields 仍需要适配。
```

P54 只要求：

```text
前端可以连到真实 /api/client/*。
不再只能靠 mock 模式运行。
```

正式前端体验优化应进入后续客户端 UI 阶段。

## 15. 实现顺序建议

推荐研发顺序：

```text
1. 新增 client_http_types。
2. 新增 IClientHttpTransport / IClientHttpRouter 接口。
3. 新增 ClientStaticFileService。
4. 新增 ClientHttpGateway，先用单元测试直接传 body。
5. 新增 ClientHttpRouter，组合静态文件和 API gateway。
6. 新增 ClientSocketHttpTransport。
7. 新增 ClientHttpServer 应用组合器。
8. 新增 client_server_main.cpp。
9. 接入 CMake。
10. 增加 client_http 单元测试。
11. 增加 client_http 集成测试。
12. 增加架构扫描。
13. 手动启动 1999，用浏览器验证 app/frontend/client。
```

## 16. 研发注意事项

```text
1. 不要修改 P53 Client Protocol 的核心语义来迁就 HTTP。
2. HTTP 层只做传输适配，不做玩法。
3. 如果前端字段不匹配，优先修 projection adapter / codec / 前端映射，不在 HTTP 层造假。
4. 不允许把 mock 数据搬到后端。
5. 不允许为了让地图显示而在 HTTP 层生成红果、狼、火把、树。
6. 本阶段可以先只显示 P53 stub projection，但必须是真实协议返回。
7. 所有新增命名使用 Client / client_http，不使用 H5V2 / h5_v2。
8. 启动脚本不要动内网穿透。
9. 如果 1999 被占用，只能停止 pathfinder_client_server 自己。
10. 文档、测试、代码必须一起提交，不能只实现服务。
11. 不允许 ClientHttpGateway 直接包含 sys/socket.h、netinet/in.h、arpa/inet.h。
12. 不允许 ClientHttpGateway 直接出现 recv / send / accept / bind / listen。
13. 不允许 ClientSocketHttpTransport 包含 client_protocol 头文件。
14. 未来替换三方库时，只能新增新的 transport 实现，不允许改 /api/client/* 语义。
```

## 17. 后续扩展预留

P54 结束后可以继续扩展：

```text
P55：客户端真实协议接入。
P56：客户端世界投影与命令桥接。
P57：正式中文横屏游戏 UI。
P58：本地存档 / 读档 HTTP API。
P59：可选 WebSocket 事件推送。
P60：桌面打包启动器。
```

这些扩展都必须继续遵守：

```text
前端只消费 Projection / available_commands / event_feed。
后端引擎不依赖前端。
客户端目录可删除，不影响 backend 编译和测试。
```
