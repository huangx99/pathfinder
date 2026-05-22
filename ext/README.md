# ext 扩展层目录

`ext/` 是后续可删除、可重做、可替换的后端内容扩展层。

目标：

```text
删除 ext/ 后，引擎核心 backend/ 仍应干净、可编译、可测试。
扩展玩法对应的后端内容包、后端注册适配和非核心 adapter 优先放在这里。前端不受 ext/ 限制。
如果扩展需要引擎能力，先在 backend/ 设计通用 port/service，再由 ext/ 使用。
```

建议结构：

```text
ext/content/          扩展内容包：物品、资源、生态、任务、区域事件。
ext/backend/          扩展 adapter/注册代码，只能依赖 backend 暴露的通用接口。
ext/tests/            扩展内容自己的测试，不作为引擎核心测试前置。
```

硬性约束：

```text
backend/ 核心不能 include ext/ 的具体内容头文件。
核心规则不能为 ext/ 的单个内容写专属 if。
前端不能因为 ext/ 内容绕过 available_commands。
删除 ext/ 后，基础引擎不能崩溃；前端资源不由 ext/ 约束。
```

更多规则见：

```text
doc/研发手册.md
```
