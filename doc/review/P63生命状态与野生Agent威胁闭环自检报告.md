# P63 生命状态与野生 Agent 威胁闭环自检报告

## 1. 自检结论

本阶段实现通过自检：P63 没有新增前端规则分支，没有让前端判断野兽行动，也没有绕过 `WorldCommandPipeline` 修改野兽移动或攻击结果。

核心闭环已经接通：

```text
ContentRegistry agent/object
-> WorldRuntime actor + health
-> BeastEcologyCoordinator
-> WorldCommandPipeline
-> Move / Attack handler
-> projection patch / event feed
-> Axmol client projection display
```

## 2. 架构符合性检查

| 检查项 | 结论 | 说明 |
|---|---|---|
| 野兽不是可拾取物 | 通过 | `beast_shadow` 以 actor 进入 runtime，pickup 仍过滤 `creature/danger/predator` |
| 野兽行动走 Command | 通过 | `BeastCommandCompiler` 生成 `WorldCommandDto(source=BeastDecision)`，由 `WorldCommandPipeline` 执行 |
| 攻击不直接改数组 | 通过 | `AttackCommandHandler` 调 `IWorldRuntime::applyActorHealthDelta`，不直接改 runtime 数组 |
| 生命状态有权威接口 | 通过 | `WorldActorRuntime` 增加 `health/max_health/alive`，修改走 runtime 接口 |
| 前端不写规则 | 通过 | Axmol 只解析 projection 中的 `health/max_health/alive` 并画血条 |
| NPC 社交按钮不污染野兽 | 通过 | 社交/教学/跟随/代工只对 `AgentTemplate` 中 social/humanoid/can_teach actor 生成 |
| P52 是否符合架构 | 基本通过 | P52 的 coordinator/policy/compiler 方向正确；问题是之前未接入正式 runtime，本阶段已补线 |
| P62 内容统一是否保持 | 通过 | 野兽 actor entity 使用 `ContentRegistry` 的 `object_key/display/tags/numeric` |

## 3. 本次实现内容

- 新增 actor 生命字段和 `applyActorHealthDelta` runtime 权威接口。
- 新增 runtime `Attack` command handler，输出 `ActorDamaged/ActorDowned` 事件、health state delta 和投影 patch。
- 将初始 `beast_shadow` 从纯地图实体改为 wild actor，同时保留 `scenario_beast_shadow` 实体 ID 兼容旧威胁反制测试。
- 将 P52 `BeastEcologyCoordinator` 接入 `ClientServerRuntimeFactory` 的后端运行时适配层。
- 将野生生态推进收敛为 `Wait` 命令触发，避免教学、代工、跟随等非时间命令被野兽 tick 干扰。
- Axmol 前端读取 projection 的生命字段并显示简单血条。
- 补充 P63 集成测试：等待后 wild actor 追击并攻击玩家或同伴，生命投影变化可见。

## 4. 发现并修复的问题

### 4.1 野兽 tick 不能在所有命令后无条件推进

初版接入时，野兽在教学、代工、跟随等命令后也推进，导致旧 NPC 测试被干扰。

修复：

```text
只在 Wait 命令后推进 wild Agent tick。
```

这符合当前最小原型：等待代表时间推进。后续如果要“移动/采集也消耗世界时间”，应先设计统一 Turn/TimeCost，再扩大 tick 触发范围。

### 4.2 P52 设计之前没有真正进入可玩 runtime

P52 模块已有 coordinator、perception、decision、compiler，但正式客户端工厂没有 profile/world/knowledge/pipeline port，所以网页/Axmol 看到的野兽不会自主行动。

修复：

```text
ClientServerRuntimeFactory 内接入 P52 ports，并复用已有 WorldCommandPipeline。
```

### 4.3 NPC 社交候选必须过滤 wildlife

野兽改为 actor 后，如果不区分社交 NPC 与 wildlife，教学、跟随、代工按钮会错误出现在野兽身上。

修复：

```text
ClientRuntimeCommandOptionBridge 根据 AgentTemplate 过滤 social/humanoid/can_teach。
```

## 5. 测试结果

### 5.1 构建

```text
cmake --build /tmp/pathfinder_axmol_client_build_local --target pathfinder_tests_client_runtime_bridge_integration PathfinderAxmolClient -j2
结果：通过
```

```text
cmake --build /tmp/pathfinder_axmol_client_build_local --target pathfinder_tests_world_beast_ecology pathfinder_tests_world_beast_ecology_integration -j2
结果：通过
```

### 5.2 定向回归

```text
ctest --test-dir /tmp/pathfinder_axmol_client_build_local/pathfinder_backend -R "world_beast_ecology|client_runtime_bridge_(wildlife_actor_chases_and_attacks|pickup_options_exclude_predator|npc_follow_command_moves_companion|npc_order_torch_counters_threat_after_teaching)" --output-on-failure
结果：16/16 passed
```

### 5.3 Client runtime bridge 完整集成

```text
ctest --test-dir /tmp/pathfinder_axmol_client_build_local/pathfinder_backend -R "client_runtime_bridge_" --output-on-failure
结果：30/30 passed
```

### 5.4 Axmol 客户端构建

```text
cmake --build /tmp/pathfinder_axmol_client_build_local --target PathfinderAxmolClient -j2
结果：通过
```

## 6. 遗留风险

- 当前 wild tick 只在 `Wait` 后触发；后续要扩大到移动/采集/制作，需要先做统一时间消耗规则，不能简单全命令触发。
- `RuntimeBeastProfilePort` 里仍有通用标签映射（prey/fire/deterrent），后续更复杂生态应迁到 content profile 配置，而不是继续加 C++ 标签列表。
- 本阶段只实现生命扣减，没有完整战斗、死亡掉落、尸体、武器伤害和护甲。
- 野兽学习“火危险”的长期知识链保留接口，完整学习曲线需要后续阶段继续接 P49/P52。
- 大世界区块卸载后 wild actor 的休眠/恢复还没有做，后续要接 P59/P60 生命周期。

## 7. 验收建议

P63 可以作为最小威胁闭环验收：野兽已从“地图物品”升级为后端 wild actor，等待会推进野兽追击/攻击，生命状态能进入安全投影。下一阶段建议设计统一时间/回合推进规则，再决定移动、采集、制作是否都会触发生态 tick。

## 8. 追加自检：内容数值是否真正进入攻击

复查时发现一个可改进点：P63 初版 `Attack` 支持 `damage` 参数，`beast_shadow` 内容里也配置了 `attack_damage`，但 `RuntimeBeastProfilePort -> BeastDecisionPolicy -> BeastCommandCompiler` 没有把内容数值传到命令参数，实际仍会使用 handler 默认伤害 1。

已修复为：

```text
entity.numeric_states["attack_damage"]
-> BeastAgentProfile.attack_damage
-> BeastActionIntent.attack_damage
-> WorldCommandDto.parameters["damage"]
-> AttackCommandHandler safeParse damage
-> applyActorHealthDelta
```

这不是内容专属硬编码，而是把通用数值字段接入通用野生攻击命令。后续熊、机械哨兵、召唤物可以通过内容数值改变基础伤害，不需要修改前端或新增专属 if。

## 9. 追加架构结论

P63 当前仍有一个后续风险：`RuntimeBeastProfilePort` 里的 `prey_tags/danger_tags/deterrent_tags` 仍是通用标签列表，适合最小原型，但不适合长期堆复杂生态。该问题已经在设计文档中明确列为后续配置化迁移项，不能继续在 C++ 中无限添加标签分支。

## 10. 追加自检：威慑后游走

复查试玩反馈后补充了野生 Agent 的非攻击状态：

```text
近距离威慑 -> Flee。
可见威慑但不贴近 -> Wander。
无目标 -> Wander。
```

自检结果：Wander 不是前端表现动画，也不是直接改坐标；它由 `BeastDecisionPolicy` 生成 `Wander` intent，由 `BeastCommandCompiler` 编译为 `Move` command，再通过 `WorldCommandPipeline` 执行。测试已覆盖 policy、compiler、coordinator 和 client runtime bridge 回归。
