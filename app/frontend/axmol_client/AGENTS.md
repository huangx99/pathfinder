# Axmol 客户端开发规则

本目录是 Axmol 前端客户端实验目录，只能做客户端输入、渲染、动画、音效、UI 和协议适配。

## 硬性边界

- 禁止在本目录实现玩法规则、资源增减、知识学习、NPC 决策或地图生成规则。
- 禁止直接修改 `backend/`、`content/` 或运行时状态数组。
- 客户端只能消费后端/引擎输出的 projection、patch、event_feed、available_commands。
- 客户端提交行为必须通过 Command/Client 协议适配层，不能伪造规则结果。
- Axmol 源码位于 `app/frontend/third_party/axmol/`，客户端只能把它当三方前端依赖使用，禁止修改 Axmol 源码来实现项目玩法。

## 当前状态

这是前端骨架阶段，不接入后端核心库，不要求能独立构建完整游戏。
后续阶段需要单独设计 Axmol 与现有 Client Protocol / Runtime Core 的桥接方式。
