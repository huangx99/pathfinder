# AI 绘图资源生成

本目录只服务 Axmol 客户端表现层，生成出来的是图片资源，不代表任何玩法规则。

## 安全要求

- 不要把 API Key 写入代码、文档、manifest 或提交记录。
- 使用环境变量 `RIGHT_CODE_IMAGE_API_KEY` 提供鉴权。
- 如果 Key 曾经发到聊天、日志或截图里，建议到服务商后台轮换一次。

## 目录

```text
app/frontend/axmol_client/Content/art/asset_manifest.json  # 资源清单
app/frontend/axmol_client/tools/generate_assets.py          # 生成脚本
app/frontend/axmol_client/Content/art/tiles/                # 地块图片
app/frontend/axmol_client/Content/art/actors/               # 玩家/NPC/野兽
app/frontend/axmol_client/Content/art/items/                # 地图物品/资源节点
app/frontend/axmol_client/Content/art/effects/              # 选择框/路径/点击反馈
```

## 使用

先只预览请求，不调用接口：

```bash
app/frontend/axmol_client/tools/generate_assets.py --dry-run --only tile_grass player_idle_down
```

设置 Key 后生成指定资源：


也可以写入本地忽略文件，适合 Codex 工具环境读取：

```bash
cat > app/frontend/axmol_client/.env.local <<'EOF'
RIGHT_CODE_IMAGE_API_KEY=sk-...
EOF
```

`.env.local` 已被 `app/frontend/axmol_client/.gitignore` 忽略，不能提交。

```bash
export RIGHT_CODE_IMAGE_API_KEY='sk-...'
app/frontend/axmol_client/tools/generate_assets.py --only tile_grass player_idle_down
```

生成全部资源：

```bash
export RIGHT_CODE_IMAGE_API_KEY='sk-...'
app/frontend/axmol_client/tools/generate_assets.py
```

覆盖已存在图片：

```bash
app/frontend/axmol_client/tools/generate_assets.py --overwrite --only tile_grass
```

## 扩展方式

新增图片时，只修改 `asset_manifest.json`：

```json
{
  "id": "item_new_resource",
  "category": "items",
  "output": "items/item_new_resource.png",
  "prompt": "small collectible resource, top-down pixel art, transparent background"
}
```

注意：这里新增的是前端表现资源。真正的物品、采集、掉落、反应和知识，仍然必须由后端内容定义、Command 管线、Reaction/Runtime、Experience 和 Knowledge 系统负责。
