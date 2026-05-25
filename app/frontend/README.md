# Frontend Boundary

V3.0 客户端重新基于 Axmol 构建：

```text
app/frontend/axmol_client
app/frontend/third_party/axmol
```

边界规则：

- `third_party/axmol` 只作为第三方引擎依赖保留。
- `axmol_client` 是 V3.0 新客户端，不恢复旧 H5/旧 Axmol 玩法架构。
- 前端只负责输入、展示和表现，不能写玩法规则，不能绕过后端 Command/Runtime。
- 界面必须模块化：地图、工具面板、观察面板、事件面板分别独立类。
- 程序化绘制放在 `Source/procedural/`，一个物品或地形一个文件，后续方便替换美术。
