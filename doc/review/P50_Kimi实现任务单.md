# P50 Kimi 实现任务单

## 1. 任务

请实现 P50：教学与无知 NPC 基础行动。

必须先阅读：

```text
doc/00_设计文档编写要求.md
doc/80_P50教学与无知NPC基础行动任务卡设计.md
doc/80A_P50教学与无知NPC风险头脑风暴.md
doc/80B_P50教学与无知NPC测试文档.md
doc/79_P49探索行为产生知识任务卡设计.md
```

## 2. 实现范围

新增建议目录：

```text
backend/include/pathfinder/world_teaching/
backend/src/world_teaching/
backend/tests/unit/world_teaching/
backend/tests/integration/world_teaching/
```

新增 CMake target：

```text
pathfinder_world_teaching
pathfinder_tests_world_teaching
pathfinder_tests_world_teaching_integration
```

## 3. 必须实现

```text
1. WorldTeachingTypes：枚举、DTO、validateBasic、toString/fromString。
2. WorldActorKnowledgeOwnerResolver：actor_key -> KnowledgeOwner。
3. IWorldActorQueryPort：只读 actor 坐标接口，可用 fake 测试实现。
4. WorldTeachingEligibilityService：教师/接收者/距离/source claim/teachable 校验。
5. WorldTeachingLoopBridge：TeachingToRecipient 学习桥。
6. WorldTeachingStorePort：写入 recipient claims。
7. WorldTeachingProjectionBridge：events/state_deltas/projection_patch。
8. TeachCommandHandler：WorldCommandKind::Teach handler。
9. NpcBasicActionKnowledgeGate：NPC 单步行动知识门禁。
10. 单元测试和集成测试。
```

## 4. 禁止事项

```text
不要改旧 H5 dialog 教学。
不要写死 companion。
不要写死 torch/berry/beast/mushroom/wood。
不要复制 source claim 后只改几个字段，必须走 LearningLoop 或 KnowledgePropagation。
不要让前端传 recipient claim json。
不要失败后写 repository。
不要直接访问 WorldGridRuntime private map。
不要修改 P15-P21 学习核心，除非必须且写明理由。
```

## 5. 建议实现路线

```text
第一步：类型和枚举，跑 enum 测试。
第二步：owner resolver 和 fake actor query port。
第三步：eligibility service，覆盖拒绝测试。
第四步：loop bridge + store port，覆盖 owner 转移。
第五步：projection bridge。
第六步：TeachCommandHandler 集成。
第七步：NpcBasicActionKnowledgeGate。
第八步：扫描和回归。
```

## 6. 必跑测试

```bash
cmake --build build/backend --target pathfinder_tests_world_teaching pathfinder_tests_world_teaching_integration -j 2
ctest --test-dir build/backend -R '^world_teaching_' --output-on-failure
cmake --build build/backend -j 2
ctest --test-dir build/backend -R '^(world_command_|world_learning_|learning_|knowledge_|memory_|cognition_)' --output-on-failure
```

## 7. 完成后回复内容

请在最终回复中列出：

```text
修改的文件。
新增的模块。
测试命令和结果。
是否有未解决风险。
```
