# P61 WASM 单机运行架构任务卡设计

## 1. 阶段定位

P61 是 V2.0 开放世界路线中的“Web 单机运行架构设计”阶段。

它接在 P53-P60 之后：P53 定义客户端协议，P54 建立本地 Client HTTP 网关，P55-P56 把客户端接入真实 Runtime 投影与 available_commands，P57-P60 补齐无限地图、区块扩边、状态缓存恢复和正式地图交互触发。

P61 的一句话定位：

```text
把“浏览器通过 HTTP 调电脑后端”设计成可替换的 Client Runtime Transport，使同一套客户端协议未来既能走 remote_http，也能走 local_wasm；本阶段只设计 WASM 单机架构，不直接重写前端玩法。
```

P61 不是界面重构阶段，不是性能小补丁阶段，也不是把规则搬到 JavaScript 的阶段。

P61 要解决的是：当前 Web 客户端每一步移动都需要经过 HTTP、内网穿透、JSON 请求、后端响应、前端合并和重绘，这对手机试玩来说延迟明显。如果本项目目标包含单机开放世界，那么 Web 端最终应支持浏览器本地运行 C++ Runtime。

因此 P61 的核心不是“写一个 WASM demo”，而是先把架构边界设计清楚：JS 前端仍然只提交意图；WASM Bridge 只是一种本地传输/调用方式；真正规则仍在 C++ Runtime、WorldCommandPipeline、Inventory、Knowledge、Region、Agent 等权威系统里。

## 2. 为什么现在做

现在做 P61 有三个直接原因。

第一，移动卡顿已经暴露出 HTTP 原型链路的上限：remote_http 模式下，玩家每走一步都要等待一次请求往返；内网穿透会放大延迟和抖动；JSON projection 即使已从 200KB 级降到 70KB 级，手机端仍然要解析、合并、重绘。

第二，P53-P60 已经有足够前置能力支撑 WASM 设计：P53 已定义 ClientBootstrap / ClientCommand / ClientRefresh / ClientProjection / Patch / available_commands；P54 已把 HTTP 做成可插拔传输边界；P56 已把真实 WorldRuntime 投影和 available_commands 接回客户端协议；P57-P60 已把开放世界 region 生成、扩边、保存恢复、地图交互触发链路接上。

第三，如果继续只按 HTTP 优化，容易把短期优化写进错误位置：前端可能为了流畅自己预测规则结果；HTTP 网关可能为了性能绕过 P53 协议；移动或寻路可能绕过 WorldCommandPipeline；区块保存恢复可能被前端临时缓存替代。

P61 必须提前规定：未来要流畅，不靠前端写规则，而靠把同一套 Runtime 放到本地。

## 3. 阶段目标

P61 完成后，系统必须具备以下设计能力：

```text
1. 明确 Web 单机运行总体架构：JS Client -> Wasm Bridge -> C++ Runtime Core。
2. 明确 remote_http 与 local_wasm 共用同一套 Client 协议 DTO。
3. 明确 Runtime Core 必须从 Client HTTP Server 中解耦出来，供 HTTP 和 WASM 双方复用。
4. 明确 WASM 暴露给 JS 的接口范围：bootstrap、command、refresh、reset、save、load、diagnostics。
5. 明确 WASM 不能暴露 hidden truth、raw runtime、直接状态修改接口。
6. 明确本地存档、内容加载、版本兼容和浏览器存储的分阶段方案。
7. 明确 P61 不实现完整 WASM，只为后置 WASM 路线提供可执行任务拆分。
8. 明确测试策略：用同一组协议 harness 同时验证 remote_http 和 local_wasm 行为一致。
9. 明确性能目标：单步命令不再等待网络；移动响应由本地调用和渲染成本决定。
10. 明确后续多人/服务器模式仍可保留 remote_http，不把项目锁死为纯单机。
```

## 4. 本阶段不解决的问题

P61 不解决以下问题：

```text
不直接编译 WASM。
不引入 Emscripten 工程脚本。
不改现有前端交互逻辑。
不删除 remote_http 模式。
不做多人同步。
不做云存档。
不做正式 PWA 安装包。
不做 IndexedDB 具体实现。
不重写 WorldCommandPipeline。
不把规则迁移到 JavaScript。
不为了 WASM 修改现有玩法规则。
不处理全部 C++ 第三方库兼容问题，只列出扫描要求。
```

这些内容放到后续阶段：

```text
P63：Runtime Core 解耦（WASM 路线后置阶段）。
P64：WASM 最小原型。
P65：WASM 本地存档与内容加载。
P66：客户端双模式切换。
P67：WASM 性能优化与离线发布。
```

## 5. 核心设计原则

### 5.1 WASM 是运行载体，不是第二套规则

WASM 只改变调用路径：

```text
remote_http: JS -> HTTP -> ClientHttpGateway -> Runtime
local_wasm: JS -> WasmBridge -> Runtime
```

两者必须进入同一套 Runtime Core。

禁止在 JS 里判断能不能采集、扣除物品数量、生成知识或 NPC 行动结果；禁止在 WASM Bridge 里绕过 WorldCommandPipeline 直接改 WorldGridRuntime。

### 5.2 Client Protocol 是前端唯一依赖

前端不应该关心底层是 HTTP 还是 WASM。前端只依赖统一接口：

```text
ClientRuntimeTransport.bootstrap()
ClientRuntimeTransport.submitCommand()
ClientRuntimeTransport.refresh()
ClientRuntimeTransport.reset()
ClientRuntimeTransport.save()
ClientRuntimeTransport.load()
```

返回值继续使用 P53 DTO 语义。

### 5.3 Runtime Core 必须可嵌入

当前 `ClientServerRuntimeFactory` 以服务端进程为中心。WASM 需要一个不依赖 socket、不依赖 HTTP request、不依赖进程级全局状态的核心对象。

目标形态：

```text
ClientRuntimeCore
  owns / references WorldRuntime、InventoryRuntime、RegionLifecycle、CommandGateway、ProjectionAdapter
  exposes bootstrap / command / refresh / reset / save / load / advance
  does not know HTTP
  does not know DOM
  does not know browser
```

### 5.4 权威状态仍在 C++ Runtime

WASM 本地运行后，浏览器进程里会同时有 JS 和 C++ WASM 内存。权威状态必须在 C++ Runtime 内。

JS 只能持有：

```text
ClientWorldProjection
ClientProjectionPatch
WorldCommandOptionDto
EventFeed
FrontendHint
SafeDiagnostics
```

JS 不能持有或修改：

```text
WorldGridRuntime mutable arrays
InventoryRuntime mutable storage
Knowledge store raw claims
Region snapshot raw data
Agent internal plan queue
```

### 5.5 存档必须走 Save/Region 权威接口

WASM 本地存档不能变成前端自己保存 projection。

正确链路：

```text
ClientRuntimeCore.save()
-> SaveGamePackage / RegionSnapshot / Inventory / Knowledge / EventLog
-> WasmStoragePort
-> IndexedDB / File System Access / localStorage fallback
```

Projection 只能用于显示，不能作为存档源。

### 5.6 内容加载必须可校验

WASM 版本未来会面临内置 content、浏览器缓存 content、玩家下载 mod content、服务器下发 content。无论哪种来源，都必须进入：

```text
ContentRegistry -> 校验 -> 冲突检测 -> 版本兼容 -> Runtime 注册
```

不能让 JS 直接把 JSON 解释成玩法结果。

### 5.7 remote_http 必须继续保留

WASM 是单机体验优化，不是删除服务器能力。必须保留：

```text
remote_http：调试、远程服务器、未来多人、云端权威。
local_wasm：单机、离线、手机本地运行。
```

前端通过配置选择模式，不通过复制两套客户端代码实现。

### 5.8 WASM 设计必须可测试

同一条 fake client 脚本必须可以跑 remote_http 和 local_wasm；同一组 command 序列应得到同构 projection / patch / events；安全扫描必须确认 WASM Bridge 没有导出 direct state mutation API。

## 6. 头脑风暴：后续场景压力测试

### 6.1 手机离线游玩

玩家在没有网络时打开网页，继续探索、采集、制作、教学 NPC。

要求：Runtime 本地可启动；内容包本地可读；存档本地可保存；再次打开能恢复世界。

### 6.2 远程服务器游玩

玩家未来也可能把游戏部署在服务器上，从外网访问。

要求：remote_http 不能被删除；前端同一套 UI 能切换 transport；服务器模式仍以后端进程为权威。

### 6.3 大世界长时间探索

玩家走很远，产生大量 region，采集、丢弃、建造、保存、恢复。

要求：WASM 内存不能无限增长；Region lifecycle 必须继续 seal/detach/restore；本地存档不能只存当前视野。

### 6.4 NPC 自主行动

NPC 后续会自己采集、制作、避险、教学、建造。

要求：Agent tick 仍由 C++ Runtime 调度；JS 不能模拟 NPC 决策；WASM Bridge 可以提供 tick 或 advance 接口，但不能提供直接改 NPC 状态接口。

### 6.5 知识学习与教学

玩家探索世界形成知识，再教 NPC，NPC 后续依知识行动。

要求：Experience -> Memory -> Knowledge -> Teaching -> Agent Use 链路不变；WASM 只改变运行位置，不改变认知系统。

### 6.6 复杂制作与材料链

后续可能有房屋、工具、火、食物、农业、机械、电力。

要求：制作命令仍由 available_commands 提供；缺材料触发后续目标规划仍在 Agent/Command/Reaction 体系内；JS 不允许因为看到“缺木头”就自己添加砍树任务结果。

### 6.7 战斗、危险和外界打断

狼、野兽、天气、区域魔法、机械危机等会打断玩家或 NPC 行动。

要求：危险事件、区域事件、打断结果由 Runtime 产生；WASM Bridge 只返回 event_feed 和 projection_patch；前端只表现，不裁决。

### 6.8 高级文明与大量实体

未来可能有村落、建筑、道路、仓库、工具链、族群分裂。

要求：Projection 需要支持作用域和裁剪；WASM 本地也不能每帧把全世界投给 JS；Region snapshot 和 SavePackage 要能分块保存。

### 6.9 魔法世界或区域规则

未来可能有区域魔法、天气、地下/空中虚拟 Z 层。

要求：条件表达式、区域效果、虚拟 layer/z 仍由后端权威系统处理；WASM Bridge 不能把 area_effect 解释成规则。

### 6.10 Mod 和内容包

玩家或作者后续可能添加毒蘑菇、斧头、动物、机械、魔法材料。

要求：内容包必须进入 ContentRegistry 校验；WASM 需要明确 content source 和 content version；不能因为浏览器加载 JSON 就跳过 C++ 内容注册管线。

## 7. 模块划分

| 模块 | 中文含义 | 职责 | 输入 | 输出 | 禁止事项 |
|---|---|---|---|---|---|
| `ClientRuntimeTransport` | 客户端运行传输抽象 | 为前端提供统一 bootstrap/command/refresh/reset/save/load 接口 | JS 请求对象 | P53 Client DTO | 禁止写玩法规则 |
| `RemoteHttpClientTransport` | 远程 HTTP 传输 | 复用现有 P54 HTTP API | HTTP URL、ClientRequest | ClientResponse JSON | 禁止绕过 HTTP 网关直接调内部服务 |
| `LocalWasmClientTransport` | 本地 WASM 传输 | 把 JS 请求转给 WASM Bridge | JS Request DTO | JS Response DTO | 禁止解释规则结果 |
| `WasmRuntimeBridge` | WASM 导出桥 | 提供 C ABI / embind 可调用入口 | JSON/string/buffer 请求 | JSON/string/buffer 响应 | 禁止暴露 mutable runtime 指针 |
| `ClientRuntimeCore` | 可嵌入客户端运行核心 | 承载 bootstrap/command/refresh/reset 的实际编排 | P53 请求、runtime 依赖 | P53 响应 | 禁止依赖 socket/DOM |
| `WasmStoragePort` | 浏览器本地存储端口 | 把 SaveGamePackage 持久化到浏览器存储 | save key、bytes/json | 成功/失败、bytes/json | 禁止保存 projection 当存档 |
| `WasmContentPort` | 浏览器内容加载端口 | 加载内容包并交给 C++ 校验 | content manifest、bytes/json | validated content package | 禁止 JS 直接应用内容规则 |
| `WasmDiagnosticsPort` | 本地诊断端口 | 输出性能、内存、版本和错误诊断 | runtime status | safe diagnostics | 禁止泄露 hidden truth |
| `ClientModeSelector` | 客户端模式选择器 | 选择 remote_http 或 local_wasm | 配置、URL、WASM 可用性 | transport instance | 禁止复制两套前端逻辑 |
| `ProtocolParityHarness` | 协议一致性测试 | 比较 HTTP 与 WASM 响应一致性 | command script | parity report | 禁止用不同规则路径生成结果 |

## 8. 枚举设计

### 8.1 `ClientRuntimeMode`

用途：描述客户端当前运行模式。

枚举值：

```text
Unknown：默认值或非法输入。
RemoteHttp：通过 P54 HTTP 网关访问后端进程。
LocalWasm：通过 WASM Bridge 本地运行 C++ Runtime。
TestFake：测试专用 fake transport。
```

规则：toString/fromString 必须稳定；非法字符串返回 Unknown，并由 validateBasic 拒绝；前端可以请求模式，但最终可用模式由 ClientModeSelector 检测决定。

### 8.2 `WasmBridgeStatus`

枚举值：

```text
Unknown：默认值。
Unavailable：浏览器或构建不支持。
Loading：wasm 文件正在加载。
Ready：Runtime Core 已初始化，可接受请求。
Failed：初始化失败。
Disposed：已释放，不再接受请求。
```

规则：Loading 状态下不得提交 command；Failed 必须返回结构化错误和 safe diagnostics；Disposed 后只能重新 create，不能复用旧指针。

### 8.3 `WasmStorageBackend`

枚举值：

```text
Unknown：默认值。
MemoryOnly：仅内存，刷新丢失，用于测试或无存储权限。
IndexedDB：浏览器 IndexedDB。
FileSystemAccess：浏览器 File System Access API。
LocalStorageFallback：小型临时 fallback，不适合大世界。
TestInMemory：测试专用。
```

规则：正式单机默认 IndexedDB；LocalStorageFallback 必须有容量警告；MemoryOnly 不能误报为已持久化。

### 8.4 `WasmContentSourceKind`

枚举值：

```text
Unknown：默认值。
Embedded：编译进 WASM 或随 Web 包发布的基础内容。
BrowserCache：浏览器缓存内容。
UserFile：玩家选择的本地内容文件。
RemoteDownloaded：远程下载后缓存的内容。
TestFixture：测试专用内容。
```

规则：所有来源都必须进入 ContentRegistry 校验；UserFile 和 RemoteDownloaded 必须记录 content hash。

### 8.5 `WasmHostCapability`

枚举值：

```text
Unknown：默认值。
WasmBasic：支持基础 WASM。
WasmThreads：支持线程和 SharedArrayBuffer。
IndexedDB：支持 IndexedDB。
CompressionStream：支持压缩流。
FileSystemAccess：支持文件系统访问。
```

规则：能力检测只影响性能和存储选择，不影响规则结果；缺少高级能力时必须 fallback，而不是禁止启动基础单机。

## 9. DTO 设计

### 9.1 `ClientRuntimeTransportConfig`

字段：

```text
mode: ClientRuntimeMode
remote_base_url: string
wasm_module_url: string
wasm_worker_url: string
preferred_storage: WasmStorageBackend
content_manifest_url: string
dev_reset_if_allowed: bool
safe_diagnostics_enabled: bool
```

校验：mode 不能为 Unknown；RemoteHttp 模式 remote_base_url 必须非空；LocalWasm 模式 wasm_module_url 必须非空；preferred_storage 不能为 Unknown。

### 9.2 `WasmRuntimeCreateRequest`

字段：

```text
session_id: string
client_id: string
world_id: string
seed: string
content_manifest_hash: string
storage_backend: WasmStorageBackend
host_capabilities: list<WasmHostCapability>
```

校验：session_id/client_id/world_id 必须非空；content_manifest_hash 可以在 P63 最小原型中为空，但 P64 后必须非空；storage_backend 不能为 Unknown；host_capabilities 中 Unknown 必须被拒绝。

### 9.3 `WasmRuntimeCreateResponse`

字段：

```text
status: WasmBridgeStatus
runtime_handle: string
schema_version: string
engine_version: string
content_version: string
safe_diagnostics: list<string>
error_key: string
```

校验：Ready 状态 runtime_handle 必须非空；Failed 状态 error_key 必须非空；safe_diagnostics 不能包含 hidden truth 或 raw state。

### 9.4 `WasmCommandEnvelope`

字段：

```text
runtime_handle: string
request_json: string
request_kind: ClientRequestKind
client_sequence: int64
known_projection_version: int64
```

说明：request_json 内容必须是 P53 Client 请求 DTO，WASM Bridge 不新增玩法命令格式。

校验：runtime_handle 必须对应有效 Runtime Core；request_kind 必须和 request_json 内部 kind 一致；client_sequence 必须单调递增；known_projection_version 必须参与版本校验。

### 9.5 `WasmResponseEnvelope`

字段：

```text
runtime_handle: string
response_json: string
response_kind: ClientRequestKind
base_projection_version: int64
new_projection_version: int64
requires_full_refresh: bool
safe_diagnostics: list<string>
```

说明：response_json 内容必须是 P53 Client 响应 DTO。base/new version 必须与 response_json 一致。

### 9.6 `WasmSaveRequest`

字段：

```text
runtime_handle: string
save_slot: string
reason: string
include_region_snapshots: bool
include_command_log: bool
```

校验：save_slot 必须非空；reason 必须是安全文本；include_region_snapshots 为 false 时必须明确这是临时轻量保存，不可标记为完整世界存档。

### 9.7 `WasmLoadRequest`

字段：

```text
runtime_handle: string
save_slot: string
expected_schema_version: string
expected_content_version: string
allow_migration: bool
```

校验：save_slot 必须非空；版本不匹配且 allow_migration=false 时必须拒绝；迁移失败不能半恢复世界。

### 9.8 `WasmSafeDiagnostics`

字段：

```text
status: WasmBridgeStatus
runtime_allocated_bytes: int64
active_region_count: int32
cached_region_count: int32
last_command_ms: double
last_projection_cells: int32
last_projection_entities: int32
last_error_key: string
safe_messages: list<string>
```

校验：只能输出性能和安全状态，不能输出真实隐藏属性、未发现资源、怪物内部目标、raw knowledge claim。

## 10. 服务接口设计

### 10.1 `ClientRuntimeTransport`

接口：

```text
bootstrap(ClientBootstrapRequest) -> ClientBootstrapResponse
submitCommand(ClientCommandRequest) -> ClientCommandResponse
refresh(ClientRefreshRequest) -> ClientRefreshResponse
reset(ClientResetRequest) -> ClientBootstrapResponse
save(ClientSaveRequest) -> ClientSaveResponse
load(ClientLoadRequest) -> ClientBootstrapResponse
diagnostics() -> ClientSafeDiagnostics
```

职责：前端只依赖这个接口；RemoteHttp 和 LocalWasm 都实现它。失败必须结构化返回，不能让前端猜测失败原因后自行修复状态。

### 10.2 `ClientRuntimeCore`

接口：

```text
create(ClientRuntimeCoreConfig) -> Result<ClientRuntimeCore>
bootstrap(ClientBootstrapRequest) -> ClientBootstrapResponse
execute(ClientCommandRequest) -> ClientCommandResponse
refresh(ClientRefreshRequest) -> ClientRefreshResponse
reset(ClientResetRequest) -> ClientBootstrapResponse
advance(ClientAdvanceRequest) -> ClientAdvanceResponse
save(SaveRuntimeRequest) -> SaveRuntimeResponse
load(LoadRuntimeRequest) -> LoadRuntimeResponse
```

职责：把当前 ClientServerRuntimeFactory 中可复用的 runtime 初始化、session、command、projection、region lifecycle 编排抽出来。HTTP Server 和 WASM Bridge 都调用它。

禁止：不能 include HTTP socket 类型；不能 include DOM 或 JS 类型；不能读取浏览器 storage；不能暴露 mutable runtime 容器给调用者。

### 10.3 `WasmRuntimeBridge`

接口形态可以有两种，具体由 P63 决策：

```text
C ABI string/buffer API：
  pf_create_runtime(json) -> json
  pf_submit(json) -> json
  pf_dispose(handle) -> json

或 embind API：
  Runtime.create(config)
  runtime.bootstrap(json)
  runtime.submitCommand(json)
  runtime.refresh(json)
```

桥接层职责：做 JSON/buffer 编解码；查找 runtime_handle；调用 ClientRuntimeCore；返回 P53 响应。

桥接层禁止：直接调用 WorldGridRuntime::setCell；直接调用 InventoryRuntime mutable API；生成 available_commands；解释 content JSON。

### 10.4 `WasmStoragePort`

接口：

```text
write(slot, bytes, metadata) -> StorageWriteResult
read(slot) -> StorageReadResult
list() -> list<StorageSlotSummary>
delete(slot) -> StorageDeleteResult
quota() -> StorageQuotaReport
```

职责：把 SaveGamePackage 或 region package 存入浏览器。C++ WASM 不能直接假设有真实文件系统；P64 需要决定使用 Emscripten IDBFS、JS IndexedDB wrapper 或二者结合。

### 10.5 `WasmContentPort`

接口：

```text
loadManifest(source) -> ContentManifestBytes
loadPackage(package_id) -> ContentPackageBytes
verifyHash(bytes, expected_hash) -> VerifyResult
registerToContentRegistry(bytes) -> ContentRegistryResult
```

职责：只负责把内容 bytes 交给 C++ 内容校验链。禁止让 JS 根据 JSON 内容生成按钮，禁止绕过 ContentRegistry。

## 11. 状态流转

### 11.1 remote_http 当前流转

```text
JS Client
-> RemoteHttpClientTransport
-> /api/client/bootstrap|command|refresh|reset
-> ClientHttpGateway
-> ClientSessionGateway / ClientCommandGateway
-> WorldCommandPipeline / Runtime / Projection
-> JSON Response
-> JS Client apply projection/patch
```

### 11.2 local_wasm 目标流转

```text
JS Client
-> LocalWasmClientTransport
-> WasmRuntimeBridge
-> ClientRuntimeCore
-> ClientSessionGateway / ClientCommandGateway
-> WorldCommandPipeline / Runtime / Projection
-> JSON or buffer Response
-> JS Client apply projection/patch
```

两条流转的中段必须尽量一致。

### 11.3 WASM 初始化流转

```text
加载 wasm module
-> 检测 host capabilities
-> 构造 WasmRuntimeCreateRequest
-> WasmRuntimeBridge create
-> ClientRuntimeCore create
-> ContentRegistry 初始化
-> WorldRuntime / Inventory / Region / Knowledge / Agent 依赖初始化
-> 返回 Ready 或 Failed
```

### 11.4 WASM 命令流转

```text
前端选择 available_commands 中的 option_id
-> LocalWasmClientTransport.submitCommand
-> WasmCommandEnvelope
-> WasmRuntimeBridge 校验 handle / request kind
-> ClientRuntimeCore.execute
-> ClientCommandGateway 校验 projection_version / option snapshot
-> WorldCommandPipeline 执行
-> Runtime 状态变化
-> ProjectionPatch 生成
-> ResponseEnvelope 返回
-> 前端合并 patch 和表现动画
```

### 11.5 WASM 保存流转

```text
玩家触发保存 / 自动保存
-> ClientRuntimeTransport.save
-> ClientRuntimeCore.save
-> SaveGamePackage / RegionSnapshot / Metadata
-> WasmStoragePort.write
-> 返回 save summary
```

## 12. 典型场景

### 12.1 玩家手机本地移动

玩家双击目标格，前端寻路或请求后端可用移动 option，每一步 submitCommand 走 LocalWasmClientTransport；没有 HTTP 往返；Runtime 返回移动 patch；前端播放动画。

验收重点：移动结果仍来自 WorldCommandPipeline。不能出现前端先改坐标再等 WASM 确认的权威错位。如果要做视觉预测，必须标记为表现层，并在响应后校正，不作为状态源。

### 12.2 玩家采集石头并学习

前端选择地块，available_commands 给出 pickup/harvest；玩家提交 option_id；WASM Runtime 执行采集或拾取；Inventory/Region/Experience/Knowledge 更新；返回 patch 和事件。

验收重点：JS 不能看到 stone 就自行加背包；知识形成仍由 P49/P21 链路生成。

### 12.3 NPC 自己行动

前端提交 wait 或 advance；Runtime 内部 Agent tick；NPC 根据知识和目标行动；返回事件和 projection patch。

验收重点：Agent 决策不在 JS；WASM Bridge 只提供 advance，不提供直接 setNpcPosition。

### 12.4 离开区块后再回来

玩家移动导致 active region window 改变；P60 触发 region lifecycle；P59 seal/detach 保存已修改 region；之后玩家回来，ensure restore snapshot。

验收重点：WASM 本地也必须走同一套 region lifecycle，不能因为单机就在 JS 里缓存格子状态。

### 12.5 切换到远程服务器模式

玩家或开发者选择 remote_http；前端创建 RemoteHttpClientTransport；同一套 UI 和协议请求走 1999 或服务器地址。

验收重点：切换 transport 不应该改 UI 业务逻辑；remote_http 和 local_wasm 的 DTO 保持一致。

## 13. 与现有系统关系

### 13.1 与 P53 客户端协议

P61 必须复用 P53 的 DTO 和语义，包括 ClientBootstrapRequest/Response、ClientCommandRequest/Response、ClientRefreshRequest/Response、ClientProjectionPatch、WorldCommandOptionDto。不允许为 WASM 新造一套不兼容的 Web 专属协议。

### 13.2 与 P54 HTTP 网关

P54 的 HTTP 网关变成一种 transport 实现：RemoteHttpClientTransport -> ClientHttpGateway；LocalWasmClientTransport -> WasmRuntimeBridge。P61 不删除 P54。

### 13.3 与 P56 投影桥

WASM 仍使用 P56 的真实 runtime projection 和 command option bridge。不允许 WASM Bridge 自己生成可见格子或命令按钮。

### 13.4 与 P57/P58 无限地图

WASM Runtime 必须支持无限 region 生成和按需扩边，不能为了 WASM 简化成固定地图。

### 13.5 与 P59/P60 区块生命周期

WASM 本地运行后，区块生命周期更重要，因为浏览器内存有限。P61 要求后置 WASM 实现保留 active window、seal region、cached snapshot、restore snapshot、detach runtime region。

### 13.6 与 P30 存档

WASM 存档必须接 P30 SaveGamePackage 思路。P61 不允许本地保存 projection 当存档。

### 13.7 与 ContentRegistry / JSON 内容包

WASM 内容加载必须进入 ContentRegistry。P61 不允许 JS 读取 JSON 后直接生成玩法结果。

### 13.8 与 Agent / Knowledge

Agent、知识、教学、认知仍在 C++ Runtime。WASM 只让它们本地运行，不改变其权威来源。

## 14. 安全边界

WASM Bridge 不得导出以下内容：

```text
hidden_truth
true_trait
real_effect
raw_state
actual_hp
true_hp
hp_delta
death
kill
corpse
loot
drop
random_damage
frontend_unlock
direct_state
runtime_pointer
mutable_runtime
world_grid_mutation
inventory_mutation
knowledge_claim_injection
agent_plan_injection
region_snapshot_raw
content_rule_eval_in_js
```

WASM Diagnostics 只能输出安全信息：版本、状态、耗时、内存估计、投影大小、活跃区块数量、安全错误码。不得输出隐藏资源、未发现实体、NPC 内部目标、真实怪物计划等内容。

## 15. 错误实现禁止清单

以下实现一律禁止：

```text
1. 为了 WASM 在 JS 中实现移动、采集、制作、教学规则。
2. WASM Bridge 直接调用 WorldGridRuntime 数组写入接口完成玩法。
3. WASM Bridge 直接给 Inventory 增减物品绕过 Command。
4. 前端根据 entity_key/object_key/terrain_key 自己生成可用命令。
5. 前端保存 ClientWorldProjection 当作真实存档。
6. 删除 remote_http，导致服务器模式和调试能力丢失。
7. 新建一套 wasm_command DTO，不兼容 P53。
8. 为了性能跳过 projection_version 校验。
9. 为了简化把无限地图退化成固定地图。
10. 为了浏览器存储方便跳过 P59 region snapshot。
11. 把 content JSON 交给 JS 解释成规则。
12. 把 full runtime diagnostics 暴露给前端。
13. 在 P61 直接重构大量无关玩法模块。
14. 只做一个能移动的 WASM demo，却不复用 ClientRuntimeCore。
```

## 16. 子阶段任务拆分

### P61-1：现有 Runtime 依赖扫描

目标：确认哪些模块能进入 WASM，哪些依赖 HTTP、文件系统、线程或平台 API。

交付：WASM compatibility scan 文档、ClientServerRuntimeFactory 依赖图、禁止导出接口清单。

验收：列出必须解耦的模块；列出后置 Runtime Core 解耦写入范围；没有把“能编译”误判为“架构可用”。

### P61-2：ClientRuntimeCore 边界设计

目标：定义 HTTP 和 WASM 共用的运行核心。

交付：ClientRuntimeCore 接口草案、CoreConfig / CoreDependencies 草案、bootstrap/command/refresh/reset/save/load 编排说明。

验收：接口不依赖 HTTP socket；接口不依赖 JS/DOM；接口仍走 P53/P56/P60 链路。

### P61-3：ClientRuntimeTransport 前端抽象设计

目标：让前端依赖统一 transport，而不是散落 fetch 调用。

交付：ClientRuntimeTransport JS interface 草案、RemoteHttpClientTransport 保留方案、LocalWasmClientTransport 目标方案、模式选择策略。

验收：UI 层不需要知道 remote_http/local_wasm 差异；available_commands 权威不变。

### P61-4：WASM Bridge DTO 与导出接口设计

目标：定义 JS 与 WASM 之间的最小安全接口。

交付：WasmRuntimeCreateRequest/Response、WasmCommandEnvelope/ResponseEnvelope、WasmSave/Load Request、SafeDiagnostics。

验收：接口不暴露 mutable runtime；接口能承载 P53 请求响应；非法 handle / stale version / bad JSON 有结构化错误。

### P61-5：本地存档与内容加载方案设计

目标：明确 WASM 本地存档和内容包加载如何接回权威系统。

交付：WasmStoragePort、WasmContentPort、IndexedDB / MemoryOnly / FileSystemAccess fallback 策略、内容 hash / version / migration 策略。

验收：不保存 projection 当 runtime；不让 JS 解释内容规则。

### P61-6：协议一致性测试设计

目标：保证 remote_http 和 local_wasm 结果一致。

交付：ProtocolParityHarness 测试计划、移动/采集/拾取/制作/教学/区块 restore 场景列表、安全扫描规则。

验收：同一 command script 可跑两种 transport；差异必须输出结构化报告。

### P61-7：后续阶段计划更新

目标：把后置 WASM 路线写入研发计划，避免后续直接乱改前端或引擎。

交付：程序设计计划更新、Runtime Core / WASM / 本地存档 / 双模式简要边界。

验收：计划明确先解耦 Runtime Core，再做 WASM 原型。

## 17. 测试计划

P61 是设计阶段，不直接要求代码测试，但必须为后续实现规定测试点。

### 17.1 枚举和 DTO 测试

```text
1. ClientRuntimeMode roundtrip。
2. WasmBridgeStatus roundtrip。
3. WasmStorageBackend roundtrip。
4. WasmContentSourceKind roundtrip。
5. WasmRuntimeCreateRequest validateBasic：空 session/client/world 拒绝。
6. WasmCommandEnvelope validateBasic：非法 handle / Unknown request kind 拒绝。
7. WasmSafeDiagnostics forbidden key 扫描。
```

### 17.2 Runtime Core 测试

```text
8. ClientRuntimeCore bootstrap 返回 full projection 和 available_commands。
9. ClientRuntimeCore command 通过 option_id 执行移动。
10. stale projection_version 被拒绝。
11. forged actor 被拒绝。
12. reset 后 actor 回到初始安全位置。
```

### 17.3 WASM Bridge 测试

```text
13. create -> bootstrap -> command -> refresh -> dispose 最小闭环。
14. dispose 后 command 被拒绝。
15. bad JSON 返回结构化错误。
16. Bridge 不导出 direct mutation symbol 的扫描门禁。
```

### 17.4 协议一致性测试

```text
17. remote_http 和 local_wasm 执行同一移动脚本，projection 关键字段一致。
18. 拾取物品后，两种 transport 的 inventory projection 一致。
19. 采集资源后，两种 transport 的 resource depleted/charges 投影一致。
20. 制作反应后，两种 transport 的生成物品和经验事件一致。
21. 教学后，两种 transport 的 knowledge projection 安全字段一致。
22. 离开 region 再返回，两种 transport 的 region restore 结果一致。
```

### 17.5 存档与内容加载测试

```text
23. MemoryOnly 保存不会标记为持久化。
24. IndexedDB 保存失败时不污染 runtime。
25. content hash mismatch 被拒绝。
26. content version mismatch 输出 migration required。
```

### 17.6 回归测试范围

```text
P53 client_protocol。
P54 client_http。
P56 client_runtime_bridge。
P57 world_generation。
P58 region ensure。
P59 region snapshot/cache/restore。
P60 map interaction lifecycle。
WorldCommandPipeline architecture scan。
```

## 18. 验收标准

P61 设计验收标准：

```text
1. 文档符合 doc/00_设计文档编写要求.md。
2. 明确 P61 不直接实现 WASM。
3. 明确 WASM 是 transport/runtime embedding，不是第二套规则。
4. 明确 ClientRuntimeCore 解耦是后置 WASM 路线前置。
5. 明确 JS 前端只依赖 ClientRuntimeTransport。
6. 明确 remote_http 与 local_wasm 共用 P53 协议。
7. 明确存档不能保存 projection。
8. 明确内容包不能由 JS 解释规则。
9. 明确安全字段和错误实现禁止清单。
10. 程序设计计划已记录 P61 及后置 WASM 路线。
```

后续实现验收标准：编译通过、专项测试通过、P53-P60 相邻测试通过、架构扫描通过、remote_http 不回退、前端不新增玩法规则、WASM Bridge 不导出 direct mutation API。

## 19. 风险与控制

### 19.1 风险：把 WASM 做成第二套小游戏

原因：为了快速证明手机不卡，研发可能只在 JS/WASM demo 里做移动。

后果：破坏 Command、Knowledge、Region、Inventory 闭环。

控制：P61 明确 P63 最小原型也必须复用 ClientRuntimeCore 和 P53 DTO。

### 19.2 风险：Runtime Core 解耦范围过大

原因：当前 ClientServerRuntimeFactory 可能耦合大量初始化逻辑。

后果：Runtime Core 解耦一次改太多，破坏 HTTP 现有功能。

控制：Runtime Core 解耦只做抽取，不改玩法规则；HTTP 测试必须保持通过。

### 19.3 风险：浏览器存储被误用为权威状态

原因：前端更容易保存 JSON projection。

后果：加载后世界状态丢失、知识和区块不一致。

控制：P64 必须接 SaveGamePackage / RegionSnapshot，projection 禁止当存档源。

### 19.4 风险：WASM 内存增长失控

原因：大世界 region 和实体长时间探索后常驻内存。

后果：手机浏览器崩溃。

控制：保留 P59/P60 region lifecycle；增加 diagnostics 中 active/cached region 统计。

### 19.5 风险：内容包加载绕过校验

原因：浏览器容易直接 fetch JSON。

后果：规则不一致、安全边界破坏、mod 冲突不可控。

控制：所有 content bytes 必须进入 ContentRegistry 校验，JS 只负责传输 bytes。

### 19.6 风险：remote_http 被边缘化

原因：单机 WASM 体验更好，研发可能删除 HTTP 路径。

后果：服务器部署、远程调试、未来多人基础丢失。

控制：ClientRuntimeTransport 明确保留 RemoteHttp 实现，协议一致性测试覆盖两种模式。

### 19.7 风险：WASM 调试困难

原因：浏览器内 WASM 错误栈和 C++ 状态不易观察。

后果：问题定位困难，研发乱加前端规则绕过。

控制：设计 SafeDiagnostics、structured error、parity harness，但不泄露 hidden truth。

### 19.8 风险：性能瓶颈从网络转移到渲染

原因：WASM 解决请求延迟，但 projection 太大或前端整屏重画仍会卡。

后果：用户仍觉得卡。

控制：P61 只解决运行位置；后续 P66 需要做 projection scope、增量渲染和二进制/共享内存优化。

## 20. 给实现阶段的硬性约束

后续研发 AI 必须遵守：

```text
1. WASM 路线已后置；恢复时必须先做 Runtime Core 解耦，再做 WASM 原型。
2. 不得在 P61 或后置 WASM 阶段直接重写前端玩法。
3. 不得删除 P54 HTTP 网关。
4. 不得新增绕过 P53 的 WASM 专属玩法协议。
5. 不得在 JS 中实现移动、采集、制作、教学、知识、NPC、区块保存规则。
6. 不得让 WASM Bridge 暴露 runtime mutable pointer 或 direct mutation API。
7. 不得保存 projection 当作真实存档。
8. 不得让 JS 解释 content JSON 形成规则。
9. 不得为了 WASM 固定地图大小。
10. 不得跳过 projection_version、client_sequence、option snapshot 校验。
11. 不得把 hidden truth、raw state、agent plan、region raw snapshot 暴露给前端。
12. 不得为通过测试修改生产规则结果。
```

## 21. 后置 WASM 路线建议

### 后置 WASM-1：Runtime Core 解耦

目标：从当前 HTTP server runtime factory 中抽出可嵌入的 `ClientRuntimeCore`。

范围：抽接口、迁移初始化编排、保持 HTTP 行为不变、补 P53/P54/P56/P60 回归。

不做：不编译 WASM，不改前端 UI。

### 后置 WASM-2：WASM 最小原型

目标：浏览器本地运行 bootstrap、移动一格、refresh。

范围：Emscripten 构建最小目标、WasmRuntimeBridge、LocalWasmClientTransport 原型、最小协议一致性测试。

不做：不做完整存档，不做全部内容包加载，不做性能深度优化。

### 后置 WASM-3：WASM 本地存档与内容加载

目标：本地保存、恢复、内容版本校验。

范围：WasmStoragePort、WasmContentPort、IndexedDB 或 IDBFS、SaveGamePackage / RegionSnapshot、content hash/version。

### 后置 WASM-4：客户端双模式切换

目标：同一前端可选择 remote_http 或 local_wasm。

范围：ClientModeSelector、统一错误提示、模式状态显示、remote_http/local_wasm parity 测试。

### 后置 WASM-5：WASM 性能与体验优化

目标：让 WASM 单机不仅能跑，而且在手机上稳定流畅。

范围：projection scope、增量渲染、命令批处理、路径执行优化、diagnostics overlay、必要时二进制 bridge 或共享内存。

## 22. 文档自检

- [x] 是否写清楚阶段定位。
- [x] 是否写清楚为什么现在做。
- [x] 是否写清楚目标。
- [x] 是否写清楚不做什么。
- [x] 是否写清楚核心原则。
- [x] 是否列出模块划分。
- [x] 是否逐个设计枚举。
- [x] 是否逐个设计 DTO 字段。
- [x] 是否逐个设计服务接口。
- [x] 是否写主流程。
- [x] 是否写典型场景。
- [x] 是否写与前置系统关系。
- [x] 是否写安全边界。
- [x] 是否写错误实现禁止清单。
- [x] 是否拆分子阶段任务。
- [x] 是否写测试计划。
- [x] 是否写验收标准。
- [x] 是否写风险与控制。
- [x] 是否写实现硬性约束。
- [x] 文档体量符合重要基础设施阶段要求。

## 23. 当前路线调整说明

2026-05-23 路线调整：P61 WASM 单机运行架构保留为未来路线，但不作为当前立即实现目标。当前先继续排查现有 remote_http 客户端链路的移动、请求耗时、projection 大小、前端渲染和区块生命周期事件问题。

调整原因：当前更需要先把现有可玩链路修稳，而不是继续增加区域地图或 WASM 等新架构分支。P61 的 remote_http/local_wasm 设计不作废；等当前客户端体验稳定后，再恢复 Runtime Core 解耦和 WASM 原型路线。

硬性边界：不能因为性能优化而绕过 WorldCommandPipeline、available_commands、Runtime、Inventory、Knowledge 或 Region 生命周期；也不能删除 P54 remote_http。
