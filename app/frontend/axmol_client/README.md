# Pathfinder Axmol Client

这是新的 Axmol 前端客户端目录，目标是替代卡顿的网页客户端，但不污染后端引擎。

## 定位

```text
backend/                 = 规则、Runtime、Command、Knowledge、Agent 的权威来源
app/frontend/client/      = 现有网页调试客户端，暂时保留
app/frontend/axmol_client/ = 新 Axmol 游戏客户端，只负责表现层
```

Axmol 客户端只能做：

- 地图和实体渲染。
- 玩家输入采集。
- 镜头、动画、粒子、音效。
- 展示 projection / patch / event_feed。
- 从 available_commands 选择 command 并提交。

Axmol 客户端不能做：

- 自己判断移动、采集、制作是否成功。
- 自己增减背包、地图物品、资源节点。
- 自己生成知识、记忆、NPC 行动结果。
- 绕过 Command/available_commands。
- 把红果、狼、火把等内容规则写进前端。

## 接入路线

### Phase A：客户端壳

- 创建 Axmol App / Scene / Layer 基础结构。
- 做地图格子渲染占位。
- 做玩家精灵占位。
- 做输入事件到 `ClientCommandPort` 的转发。

### Phase B：协议适配

- 先复用现有 Client 协议数据结构的前端镜像。
- 只接 `bootstrap`、`refresh`、`submitOption`、`reset`。
- 禁止 UI 自己构造玩法结果。

### Phase C：本地 C++ 直连

- 抽象 `ClientTransport`。
- `LocalRuntimeTransport` 直接链接现有 C++ runtime/client gateway。
- `HttpTransport` 作为远程/调试模式保留。

### Phase D：正式表现

- Tile atlas。
- 角色四方向行走动画。
- 点击格子寻路表现。
- 采集/拾取/制作反馈。
- NPC/野兽表现。

## 依赖原则

Axmol 源码已下载到前端三方目录：

```text
app/frontend/third_party/axmol/
```

`app/frontend/axmol_client/CMakeLists.txt` 默认从这个目录构建，不需要改后端。后续如果要减小仓库体积，可以把该目录改成 Git submodule 或下载脚本。

## 本地运行

当前最小 Axmol 客户端已经可以构建出桌面窗口。

```bash
app/frontend/axmol_client/scripts/build.sh
app/frontend/axmol_client/scripts/run.sh
```

当前窗口只显示占位地图和玩家标记，用于验证 Axmol 前端链路。它还没有接入后端 Runtime，也不会写任何玩法规则。

## AI 绘图资源

Axmol 客户端可以先用程序自绘，也可以使用 Right Code 图片接口批量生成前端资源。生成工具位于：

```text
app/frontend/axmol_client/tools/generate_assets.py
app/frontend/axmol_client/Content/art/asset_manifest.json
app/frontend/axmol_client/docs/AI绘图资源生成.md
```

安全要求：不要把 API Key 写进代码或提交记录。运行前使用环境变量：


如果 Codex 执行环境看不到你在终端 `export` 的变量，可以把 key 放到本地忽略文件：

```bash
cat > app/frontend/axmol_client/.env.local <<'EOF'
RIGHT_CODE_IMAGE_API_KEY=sk-...
EOF
```

```bash
export RIGHT_CODE_IMAGE_API_KEY='sk-...'
app/frontend/axmol_client/tools/generate_assets.py --only tile_grass player_idle_down
```

这些图片只用于客户端表现；物品、反应、采集、掉落和知识仍由后端内容系统与 Command 管线决定。
