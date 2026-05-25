# Frontend Boundary

V3.0 客户端将重新基于 Axmol 构建。

当前目录约定：

```text
app/frontend/third_party/axmol
```

保留为 Axmol 第三方引擎依赖。

旧的 H5 客户端和旧的 Axmol 原型客户端已经废弃并删除，后续不要基于旧客户端继续改造。新的 V3.0 客户端应重新建立独立工程目录，并只通过后端 Command / Projection 协议交互，不能在前端写玩法规则。
