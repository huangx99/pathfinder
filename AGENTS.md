# AGENTS.md

本文件是进入本仓库后的 AI 入口规则。完整研发规范见 `doc/研发手册.md`，设计文档规范见 `doc/00_设计文档编写要求.md`。

## 必读顺序

1. 先读并遵守 `doc/研发手册.md`。
2. 涉及设计文档时，必须遵守 `doc/00_设计文档编写要求.md`。
3. 涉及阶段计划时，必须同步检查 `doc/程序设计计划.md`。
4. 涉及审核时，报告必须写入 `doc/review/`，文件名使用中文。

## 硬性架构规则

- 所有玩法行为必须走 `WorldCommandPipeline` 或对应 Command 管线，不能直接绕过 Command 调用底层服务完成玩法。
- 前端只能负责输入、展示和表现，不能写玩法规则，不能绕过 `available_commands`。
- 状态变更必须交给对应权威模块处理，不能直接改 runtime 数组或临时拼状态。
- 新增物品、反应或内容包，必须能进入 `ContentRegistry -> Command -> Reaction/Runtime -> Experience -> Knowledge Learning` 闭环。
- 如果知识声明为可教学或可供 Agent 使用，不能为单个内容写专属 if，应接入通用教学、知识和 Agent 架构。

## ext 扩展规则

- 后端内容扩展默认放入 `ext/`。
- `ext/content/` 放物品、资源、反应、生态、任务、区域事件和内容包。
- `ext/backend/` 放扩展 adapter 或注册代码，但只能依赖 `backend/` 暴露的通用接口。
- `ext/tests/` 放扩展内容自己的测试。
- `ext/` 不是第二套引擎，不能在里面复制或绕过 `backend/` 的通用架构。
- 删除 `ext/` 后，基础引擎仍应保持干净、可编译、可测试。
- 前端资源不受 `ext/` 约束，但前端仍必须遵守“不写规则、不绕过 Command/available_commands”。

## 当前边界

- `ext/content/` 目前是目录规范和职责边界，不代表 JSON 自动加载已经完成。
- 不得把“测试里手动构造 `ContentRegistry`”误判为“`ext/content/` JSON 已自动加载”。
- 如果要实现“把 JSON 放进 `ext/content/` 就自动生效”，必须单独设计 ext content package loader，并补充加载、校验、冲突检测、版本兼容和回归测试。

## 禁止事项

- 禁止为了完成任务写内容专属硬编码。
- 禁止让前端推导或伪造后端规则结果。
- 禁止新增一套与现有 Command、Reaction、Knowledge、Agent 并行的玩法管线。
- 禁止只验证“产物生成了”就认为新增内容完成；必须验证经验、知识和后续可教学/可使用链路。
