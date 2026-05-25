# Pathfinder V3 Axmol Client

V3.0 沙盒客户端。前端只负责输入与展示，玩法状态由后端 `pathfinder_v3_sandbox` 运行时处理。

模块边界：

- `Source/scene/`：场景装配。
- `Source/runtime/`：本地 Command 适配，不写玩法规则。
- `Source/world/`：地图格子和实体展示。
- `Source/ui/`：工具面板、观察面板、事件面板。
- `Source/procedural/`：程序化像素绘制；一个物品/地形一个文件。

运行：

```bash
app/frontend/axmol_client/scripts/run.sh /tmp/pathfinder_axmol_client_build_v3
```
