# P49 Kimi 实现任务单

## 1. 任务

请按 P49 设计实现“探索行为产生知识 / 世界经验学习桥接”。

必须先阅读：

```text
doc/00_设计文档编写要求.md
doc/79_P49探索行为产生知识任务卡设计.md
doc/79A_P49世界经验学习风险头脑风暴.md
doc/79B_P49世界经验学习测试文档.md
doc/78_P48制作与世界反应接入任务卡设计.md
```

## 2. 实现范围

新增建议目录：

```text
backend/include/pathfinder/world_learning/
backend/src/world_learning/
backend/tests/unit/world_learning/
backend/tests/integration/world_learning/
```

必须实现：

```text
WorldLearningTypes
WorldExperienceExtractor
WorldKnowledgeTemplateResolver
WorldSafeFeedbackBuilder
WorldLearningLoopBridge
WorldKnowledgeStorePort / repository adapter
WorldKnowledgeLearningService
WorldKnowledgeProjectionBridge
```

可以根据现有代码风格合并小类，但职责边界必须保留。

## 3. 必须遵守

```text
不依赖 H5。
不硬编码红果、火把、木头、野兽等具体内容 key。
不直接修改 WorldRuntime / InventoryRuntime 状态。
不绕过 LearningLoopService 直接伪造最终知识。
不读取前端 parameters 里的知识结论。
不泄露 hidden truth / raw internal state。
缺 knowledge template 时跳过学习，不生成假知识，不回滚原 command。
```

## 4. 测试要求

至少实现并通过：

```text
world_learning_enum_roundtrip
world_learning_extracts_experience_from_command_result
world_learning_resolves_template_from_reason_keys
world_learning_resolves_template_by_action_effect_fallback
world_learning_skips_missing_template_without_world_mutation
world_learning_rejects_unsafe_experience
world_learning_builds_safe_feedback
world_learning_direct_experience_forms_claim
world_learning_repeated_experience_reinforces_claim
world_learning_no_frontend_knowledge_input
world_learning_craft_experience_uses_p48_knowledge_template
world_learning_harvest_experience_uses_p47_output
world_learning_command_result_merges_learning_events
world_learning_missing_template_does_not_fail_craft
```

如果因为旧 LearningLoopService 限制无法完整实现某个测试，必须先用最小适配修复，不要绕过学习系统。

## 5. 验证命令

请运行：

```bash
cmake -S backend -B build/backend
cmake --build build/backend --target pathfinder_tests_world_learning pathfinder_tests_world_learning_integration -j 2
ctest --test-dir build/backend -R '^world_learning_' --output-on-failure
```

再运行相关回归：

```bash
cmake --build build/backend -j 2
ctest --test-dir build/backend -R '^(world_command_|world_runtime_|world_inventory_|world_generation_|world_harvest_|world_reaction_|world_learning_|learning_|knowledge_|memory_|cognition_)' --output-on-failure
```

## 6. 完成后汇报

请汇报：

```text
修改了哪些文件。
新增了哪些模块。
哪些测试通过。
是否有未解决风险。
```

不要提交，不要推送。提交推送必须等我审核通过后再做。
