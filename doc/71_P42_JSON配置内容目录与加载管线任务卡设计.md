# P42 JSON 配置内容目录与加载管线任务卡设计

## 1. 阶段定位

P42 是 JSON 配置内容目录与加载管线阶段。

它接在 P41 智能 Agent 与世界效果通用执行闭环之后，不是重新做规则引擎，也不是临时给 H5 写几个配置文件。

P42 的目标是把当前仍散落在 C++ 内置表、H5 场景构造、EffectExecutionSpec、EffectSemantics、DialogScenario、ReactionRule、危险配置、知识显示文案里的基础内容，逐步迁移到统一 JSON 内容包目录中。

本阶段解决的问题是：以后新增一个物品、一个动作效果、一个反应、一个危险、一个 NPC、一个知识文案，应该主要通过 JSON 增加内容，而不是改核心 C++ 架构。

P42 在总架构中的位置：

- 上游依赖：P1 配置加载基础、P27 条件表达式、P28 对象反应、P28.5 配置化物品与反应流水线、P29 危险、P32 H5 安全投影、P39 推理、P40 执行、P41 通用效果执行闭环。
- 本阶段新增：JSON 内容目录规范、JSON DTO、Schema 校验、ContentRegistry、ContentResolver、加载顺序、跨文件引用校验、版本兼容策略、测试 fixture。
- 下游支撑：基础内容包扩展、地图对象、采集、建造、专家 NPC、野生 NPC、魔法/机械/高级文明内容包、Mod 化内容、H5 内容刷新。

P42 的权威边界：

- JSON 是内容定义来源，不是运行时状态来源。
- 运行时状态仍由 WorldSnapshot、GameState、Memory、Knowledge、EventLog、SaveGame 管理。
- JSON 条件只能使用 P27 受限条件表达式，不允许写脚本代码。
- JSON 效果只能声明已注册的安全 EffectOperation，不允许直接执行任意 C++。
- 前端只能读取投影，不允许直接读取 JSON 来推导规则。

## 2. 为什么现在做

现在必须做 P42。

原因一：P41 已经把执行闭环收束到通用效果执行，但内容仍然分散。

如果继续用 C++ 添加内容，毒蘑菇、普通蘑菇、陷阱、房子、狼、专家 NPC、魔法材料都会不断污染核心代码。

原因二：当前最小可玩已经暴露“内容和架构混在一起”的问题。

例如野兽、火把、火堆、斧头、木头、知识传播、同伴行动，很多内容仍需要程序员在多个文件中同步修改。这样后续低上下文 AI 很容易漏改。

原因三：后续要做大世界地图和多 Agent 行为，必须先让内容可声明、可校验、可组合。

地图做完后，采集对象、地点资源、野生 NPC、生产链、房屋、天气、道路、交通工具都必须能被内容包表达。

如果不先做 P42，后面地图阶段会被迫一边做地图一边补配置系统，风险更高。

## 3. 阶段目标

P42 完成后系统必须具备以下能力。

### 3.1 内容包目录可加载

系统能从固定目录加载一个内容包：

```text
content/core/manifest.json
content/core/objects/*.json
content/core/effects/*.json
content/core/reactions/*.json
content/core/feedbacks/*.json
content/core/actions/*.json
content/core/agents/*.json
content/core/threats/*.json
content/core/knowledge/*.json
content/core/scenarios/*.json
content/core/locales/zh_cn.json
content/core/tests/*.json
```

第一版可以只实现其中一部分，但目录和 manifest 必须一次设计完整，避免后续反复改根结构。

### 3.2 内容定义可注册

JSON 解析后进入 `ContentRegistry`。

注册表至少能按 key 查询：

- 物品定义。
- 行为定义。
- 效果定义。
- 反馈定义。
- 反应规则。
- 威胁定义。
- Agent 模板。
- 知识模板。
- 场景定义。
- 本地化文案。

### 3.3 跨文件引用可校验

例如：

- action 引用的 effect 必须存在。
- feedback 引用的 object/action/effect 必须存在。
- reaction 输入和输出对象必须存在。
- threat countermeasure 引用的 effect 必须存在。
- agent 默认知识引用的 subject/effect 必须存在。
- scenario 初始对象引用必须存在。

错误必须指出 `file_path`、`json_path`、`message`。

### 3.4 JSON 不拥有运行时状态

JSON 只能定义初始值和规则模板。

运行后变化必须写入运行时状态：

- 数量变化写入 WorldObjectInstance / DialogObjectRuntimeState。
- Agent 持有物品写入 actor_quantities / held_object_keys。
- 知识变化写入 KnowledgeClaim。
- 记忆变化写入 MemoryRecord。
- 威胁变化写入 ThreatRuntime。

### 3.5 内容可组合

一个新增内容不应只服务单一演示。

例如毒蘑菇应能同时参与：

- 被吃。
- 被采集。
- 被晒干。
- 被制药。
- 被 NPC 误食。
- 被玩家教给同伴。
- 被族群知识传播。
- 被危险/谋害识别系统引用。

## 4. 本阶段不解决的问题

P42 不做以下内容。

1. 不做完整 Mod 市场。

本阶段只做本地内容包，不做下载、签名、权限沙箱和在线更新。

2. 不做热更新运行中替换。

第一版只要求启动时加载。运行中热更新放到后续工具阶段。

3. 不做完整 JSON Schema 第三方依赖强绑定。

可以先用 C++ validator 手写结构校验。后续再决定是否引入 JSON Schema 库。

4. 不做地图寻路。

JSON 可以声明地点和资源，但不实现地图寻路。

5. 不做前端读取 JSON。

H5 仍只读后端投影。

6. 不迁移所有历史内容。

第一版只迁移最小闭环内容，保留 C++ fallback，但必须建立迁移门禁和 TODO 清单。

## 5. 核心方案

P42 采用“内容包 Manifest + 分目录 JSON + 强类型 DTO + Registry + Resolver + Validator + Adapter”的结构。

加载流程：

```text
ConfigLoader
  -> ContentManifestLoader
  -> JsonContentParser
  -> ContentDraftRegistry
  -> StructuralValidator
  -> ReferenceValidator
  -> ContentRegistry
  -> RuntimeAdapters
```

解释：

- `ConfigLoader` 只负责安全读文件、hash、路径限制。
- `ContentManifestLoader` 读取 manifest，决定加载哪些目录和文件。
- `JsonContentParser` 把 JSON 文本解析为 DTO，不做业务推理。
- `ContentDraftRegistry` 临时收集所有 DTO，允许先加载后校验引用。
- `StructuralValidator` 校验字段类型、必填项、枚举合法性。
- `ReferenceValidator` 校验跨文件引用。
- `ContentRegistry` 提供只读查询。
- `RuntimeAdapters` 把内容定义接入现有系统，不让旧系统直接读 JSON。

## 6. 目录结构设计

### 6.1 根目录

建议目录：

```text
content/
  core/
    manifest.json
    objects/
    actions/
    effects/
    feedbacks/
    reactions/
    agents/
    threats/
    knowledge/
    scenarios/
    locales/
    tests/
  dev/
    manifest.json
```

`core` 是官方核心内容包。

`dev` 是开发测试内容包，可在测试中启用，不允许默认进入正式运行。

### 6.2 manifest.json

示例：

```json
{
  "package_key": "core",
  "display_name": "先驱者核心内容包",
  "content_version": "0.1.0",
  "schema_version": "content_schema_1",
  "locale_default": "zh_cn",
  "load_order": 0,
  "files": [
    { "path": "objects/basic_survival.json", "category": "objects", "required": true },
    { "path": "effects/basic_effects.json", "category": "effects", "required": true },
    { "path": "feedbacks/basic_feedbacks.json", "category": "feedbacks", "required": true },
    { "path": "reactions/basic_reactions.json", "category": "reactions", "required": true },
    { "path": "agents/basic_agents.json", "category": "agents", "required": true },
    { "path": "threats/basic_threats.json", "category": "threats", "required": true },
    { "path": "scenarios/first_night.json", "category": "scenarios", "required": true },
    { "path": "locales/zh_cn.json", "category": "locales", "required": true }
  ]
}
```

约束：

- `package_key` 必须是稳定 id，不允许中文。
- `display_name` 可本地化，但 manifest 第一版允许中文。
- `schema_version` 用于兼容校验。
- `files.path` 必须是相对路径，禁止 `..` 和绝对路径。
- `category` 必须属于 `ContentFileCategory` 枚举。

## 7. 枚举设计

### 7.1 ContentFileCategory

```cpp
enum class ContentFileCategory {
    Objects,
    Actions,
    Effects,
    Feedbacks,
    Reactions,
    Agents,
    Threats,
    Knowledge,
    Scenarios,
    Locales,
    Tests
};
```

中文说明：

- `Objects`：世界对象定义。
- `Actions`：玩家或 Agent 可执行动作定义。
- `Effects`：动作导致的世界效果定义。
- `Feedbacks`：行为反馈和学习输入定义。
- `Reactions`：对象组合反应定义。
- `Agents`：Agent 模板定义。
- `Threats`：危险和威胁定义。
- `Knowledge`：默认知识、可传播知识模板。
- `Scenarios`：场景初始内容。
- `Locales`：本地化文案。
- `Tests`：内容包自带测试用例。

### 7.2 ContentDefinitionKind

```cpp
enum class ContentDefinitionKind {
    Object,
    Action,
    Effect,
    Feedback,
    Reaction,
    Agent,
    Threat,
    KnowledgeTemplate,
    Scenario,
    Locale
};
```

用途：统一注册表中的定义类型。

### 7.3 ContentLoadStage

```cpp
enum class ContentLoadStage {
    ReadManifest,
    ReadFiles,
    ParseJson,
    ValidateStructure,
    ValidateReferences,
    BuildRegistry,
    BindRuntimeAdapters
};
```

用途：错误报告和 trace。

### 7.4 ContentOverridePolicy

```cpp
enum class ContentOverridePolicy {
    Forbidden,
    AllowSamePackagePatch,
    AllowHigherPriorityPackage
};
```

第一版默认 `Forbidden`。

原因：避免两个包定义同一个 `red_berry`，导致知识和存档引用混乱。

### 7.5 ContentVisibility

```cpp
enum class ContentVisibility {
    RuntimeVisible,
    HiddenUntilDiscovered,
    SystemOnly,
    TestOnly
};
```

用途：控制投影和测试内容。

## 8. ID 与命名规则

### 8.1 基础规则

所有 JSON key 必须是稳定英文蛇形命名：

```text
red_berry
poison_mushroom
make_torch
repel_beast
beast_shadow
first_night
```

禁止：

- 中文 id。
- 空格。
- 随机 uuid 当内容 id。
- 用显示名当 id。
- 运行时自动生成基础定义 id。

### 8.2 ID 类型映射

建议新增或复用：

```cpp
using ContentPackageKey = StrongId<ContentPackageKeyTag>;
using ContentDefinitionKey = StrongId<ContentDefinitionKeyTag>;
using ObjectDefinitionKey = StrongId<ObjectDefinitionKeyTag>;
using ActionDefinitionKey = StrongId<ActionDefinitionKeyTag>;
using EffectDefinitionKey = StrongId<EffectDefinitionKeyTag>;
using FeedbackDefinitionKey = StrongId<FeedbackDefinitionKeyTag>;
using ReactionDefinitionKey = StrongId<ReactionDefinitionKeyTag>;
using AgentTemplateKey = StrongId<AgentTemplateKeyTag>;
using ThreatDefinitionKey = StrongId<ThreatDefinitionKeyTag>;
using ScenarioDefinitionKey = StrongId<ScenarioDefinitionKeyTag>;
```

如果已有对应 StrongId，必须复用，不重复造类型。

### 8.3 显示名与本地化

JSON 内容定义里只允许存：

```json
"display_key": "object.red_berry.name"
```

中文文案放到 `locales/zh_cn.json`：

```json
{
  "object.red_berry.name": "红果",
  "object.red_berry.desc": "一种鲜红的果实。你不知道它是否能吃。"
}
```

第一版为了迁移方便，可以允许 `display_name_zh_cn` 临时字段，但必须标注 deprecated，并在 validator 中给 warning。

## 9. JSON DTO 设计

### 9.1 ContentManifestDto

字段：

```cpp
struct ContentManifestDto {
    std::string package_key;
    std::string display_name;
    std::string content_version;
    std::string schema_version;
    std::string locale_default;
    int load_order = 0;
    std::vector<ContentFileRefDto> files;
};

struct ContentFileRefDto {
    std::string path;
    ContentFileCategory category;
    bool required = true;
};
```

校验：

- `package_key` 非空且合法。
- `schema_version` 必须被当前程序支持。
- `files` 至少包含 objects/effects/feedbacks/scenarios。
- `path` 必须安全。

### 9.2 ObjectDefinitionDto

示例：

```json
{
  "objects": [
    {
      "object_key": "red_berry",
      "display_key": "object.red_berry.name",
      "description_key": "object.red_berry.desc",
      "kind": "consumable",
      "visibility": "runtime_visible",
      "safe_tags": ["food_like", "plant", "fruit"],
      "allowed_actions": ["eat", "use", "inspect"],
      "initial_state": {
        "quantity": 3,
        "numeric": {},
        "tags": []
      },
      "knowledge_hints": ["can_be_eaten_unknown"],
      "projection": {
        "safe_trait_keys": ["可尝试食用", "植物果实"],
        "unknown_use_text_key": "object.common.unknown_use"
      }
    }
  ]
}
```

字段说明：

- `object_key`：稳定对象定义 id。
- `kind`：对象实例类型，映射 WorldObjectInstanceKind。
- `safe_tags`：玩家初始可见安全标签，不是隐藏真相。
- `allowed_actions`：可尝试动作集合，不代表已知效果。
- `initial_state.quantity`：场景未覆盖时的默认数量。
- `projection.safe_trait_keys`：投影安全线索。

禁止：

- 在对象定义里直接写“吃了恢复饥饿”这种已知结论。
- 在对象定义里写运行时持有者。
- 在对象定义里写玩家已经知道的知识状态。

### 9.3 ActionDefinitionDto

示例：

```json
{
  "actions": [
    {
      "action_key": "eat",
      "display_key": "action.eat.name",
      "intent_kind": "try_eat",
      "targeting": "object_self",
      "allowed_actor_scales": ["small_agent", "human", "civilization"],
      "default_time_cost": 1,
      "safety": {
        "requires_player_confirmation": false,
        "can_target_other_actor": false
      }
    }
  ]
}
```

用途：统一玩家按钮、Agent 行动、命令解析之间的动作定义。

### 9.4 EffectDefinitionDto

示例：

```json
{
  "effects": [
    {
      "effect_key": "restore_hunger",
      "display_key": "effect.restore_hunger.name",
      "semantic_kind": "actor_need_delta",
      "goal_kinds": ["satisfy_need"],
      "target_kind": "actor_self",
      "preconditions": [
        "condition:test:eq:object.quantity.$object.gte.1"
      ],
      "operations": [
        {
          "op": "consume_object_quantity",
          "target": "$object",
          "quantity": 1,
          "summary_key": "effect.restore_hunger.consume"
        },
        {
          "op": "change_actor_need",
          "target": "$actor",
          "need_key": "hunger",
          "delta": -30,
          "summary_key": "effect.restore_hunger.need"
        }
      ],
      "learning": {
        "knowledge_effect_key": "restore_hunger",
        "confidence_delta": 0.35,
        "teachable": true
      },
      "agent": {
        "usable_by_ai": true,
        "risk_score": 1,
        "time_cost": 1
      }
    }
  ]
}
```

关键原则：

- `preconditions` 必须使用 P27 条件表达式。
- `operations.op` 必须来自白名单枚举。
- `$object`、`$target`、`$actor` 是受限占位符，不允许任意表达式。
- effect 只声明世界变化，不直接写知识状态。
- learning 只提供知识形成模板，实际形成仍由学习系统决定。

### 9.5 FeedbackDefinitionDto

示例：

```json
{
  "feedbacks": [
    {
      "feedback_key": "red_berry_eat_restore_hunger",
      "object_key": "red_berry",
      "action_key": "eat",
      "target_object_key": "",
      "effect_key": "restore_hunger",
      "priority": 100,
      "conditions": [
        "condition:test:eq:object.quantity.red_berry.gte.1"
      ],
      "reply_key": "feedback.red_berry.eat.restore",
      "utility_delta": 0.6,
      "risk_delta": 0,
      "causal_tags": ["food", "hunger"],
      "knowledge": {
        "subject_object_key": "red_berry",
        "action_key": "eat",
        "effect_key": "restore_hunger"
      }
    }
  ]
}
```

用途：替代 H5/Dialog 中硬编码 feedback 查找。

### 9.6 ReactionDefinitionDto

示例：

```json
{
  "reactions": [
    {
      "reaction_key": "wood_plus_fire_make_torch",
      "inputs": [
        { "object_key": "wood", "state": "processed", "min": 1 },
        { "object_key": "camp_fire", "quantity": 1 }
      ],
      "action_key": "use",
      "effect_key": "make_torch",
      "outputs": [
        { "object_key": "torch", "quantity_delta": 1 }
      ],
      "consume": [
        { "object_key": "wood", "state": "processed", "delta": -1 }
      ],
      "summary_key": "reaction.make_torch.summary",
      "knowledge_templates": ["knowledge.make_torch"]
    }
  ]
}
```

要求：

- 输入对象必须存在。
- 输出对象必须存在。
- 消耗不能导致负数。
- 反应可以被 Agent 推理用作前置链。

### 9.7 AgentTemplateDto

示例：

```json
{
  "agents": [
    {
      "agent_key": "beast_shadow",
      "display_key": "agent.beast_shadow.name",
      "scale": "small_agent",
      "cognition_band": "instinctive",
      "embodiment": "wildlife",
      "default_needs": {
        "fear": 0,
        "hunger": 50
      },
      "instinct_effect_keys": ["fire_is_dangerous"],
      "default_policy_key": "wildlife_cautious_probe",
      "can_learn": true,
      "can_teach": false
    }
  ]
}
```

要求：

- 野兽、蜘蛛、人类、外星人都走 Agent 模板。
- 模板定义能力边界，不定义每回合运行时状态。
- `default_policy_key` 只能引用已注册策略，不允许 JSON 写脚本。

### 9.8 ThreatDefinitionDto

示例：

```json
{
  "threats": [
    {
      "threat_key": "beast_shadow",
      "display_key": "threat.beast_shadow.name",
      "agent_key": "beast_shadow",
      "initial_level": 0,
      "progression": {
        "wait_delta": 25,
        "phases": [
          { "min": 25, "phase": "foreshadowing", "summary_key": "threat.beast.foreshadow" },
          { "min": 50, "phase": "approaching", "summary_key": "threat.beast.approach" },
          { "min": 75, "phase": "confronting", "summary_key": "threat.beast.confront" },
          { "min": 100, "phase": "attack", "summary_key": "threat.beast.attack" }
        ]
      },
      "countermeasures": [
        { "effect_key": "repel_beast", "level_delta": -100 }
      ],
      "reentry": {
        "enabled": true,
        "waits": 3,
        "level": 50,
        "tag": "flanking_probe"
      }
    }
  ]
}
```

说明：

- 迂回、袭击、恐惧、避火都应从 threat/agent 配置中逐步迁出。
- 本阶段可以先只读取 reentry 参数，不必重做完整威胁 AI。

### 9.9 KnowledgeTemplateDto

示例：

```json
{
  "knowledge_templates": [
    {
      "knowledge_key": "knowledge.red_berry.eat.restore_hunger",
      "subject_object_key": "red_berry",
      "action_key": "eat",
      "effect_key": "restore_hunger",
      "target_object_key": "",
      "default_status": "hypothesis",
      "teachable": true,
      "usable_by_ai": true,
      "summary_key": "knowledge.red_berry.eat.restore_hunger.summary"
    }
  ]
}
```

要求：

- 知识模板不是知识实例。
- 玩家或 NPC 是否知道，仍由 KnowledgeClaim 决定。
- 默认知识可以由 scenario 显式授予。

### 9.10 ScenarioDefinitionDto

示例：

```json
{
  "scenario_key": "first_night",
  "display_key": "scenario.first_night.name",
  "initial_objects": [
    { "object_key": "red_berry", "quantity": 3, "visible": true },
    { "object_key": "decayed_red_berry", "quantity": 6, "visible": true },
    { "object_key": "axe", "quantity": 1, "visible": true, "numeric": { "sharpness": 3 } },
    { "object_key": "torch", "quantity": 0, "visible": true }
  ],
  "initial_agents": [
    { "agent_key": "companion", "visible": true, "trust": 0.8 },
    { "agent_key": "beast_shadow", "visible": true }
  ],
  "initial_threats": [
    { "threat_key": "beast_shadow", "level": 0 }
  ],
  "default_player_knowledge": [],
  "default_agent_knowledge": []
}
```

## 10. 新增类与接口设计

### 10.1 ContentPackageManifest

```cpp
struct ContentPackageManifest {
    ContentPackageKey package_key;
    std::string display_name;
    ConfigVersion content_version;
    SchemaVersion schema_version;
    std::string locale_default;
    int load_order = 0;
    std::vector<ContentFileEntry> files;
};
```

职责：表示 manifest 解析后的强类型结构。

### 10.2 ContentLoadOptions

```cpp
struct ContentLoadOptions {
    std::string root_path;
    std::vector<std::string> enabled_package_keys;
    bool allow_test_content = false;
    bool allow_deprecated_fields = true;
    ContentOverridePolicy override_policy = ContentOverridePolicy::Forbidden;
};
```

### 10.3 ContentLoadResult

```cpp
struct ContentLoadResult {
    std::shared_ptr<const ContentRegistry> registry;
    ConfigValidationReport validation_report;
    std::vector<std::string> trace_keys;
};
```

### 10.4 ContentRegistry

```cpp
class ContentRegistry {
public:
    Result<const ObjectDefinitionContent*> findObject(std::string_view key) const;
    Result<const ActionDefinitionContent*> findAction(std::string_view key) const;
    Result<const EffectDefinitionContent*> findEffect(std::string_view key) const;
    Result<const FeedbackDefinitionContent*> findFeedback(std::string_view key) const;
    Result<const ReactionDefinitionContent*> findReaction(std::string_view key) const;
    Result<const AgentTemplateContent*> findAgent(std::string_view key) const;
    Result<const ThreatDefinitionContent*> findThreat(std::string_view key) const;
    Result<const ScenarioDefinitionContent*> findScenario(std::string_view key) const;

    std::vector<const FeedbackDefinitionContent*> feedbacksFor(std::string_view object_key,
                                                               std::string_view action_key,
                                                               std::string_view target_key) const;

    std::vector<const ReactionDefinitionContent*> reactionsProducing(std::string_view object_key) const;
    std::vector<const EffectDefinitionContent*> effectsForGoal(AgentGoalKind goal) const;
};
```

约束：

- `ContentRegistry` 构建完成后只读。
- 返回 const 指针或 Result，不允许运行时修改定义。
- 不暴露 JSON 原文给业务系统。

### 10.5 JsonContentLoader

```cpp
class JsonContentLoader {
public:
    Result<ContentLoadResult> load(const ContentLoadOptions& options) const;
};
```

职责：完整加载入口。

### 10.6 ContentReferenceValidator

```cpp
class ContentReferenceValidator {
public:
    ConfigValidationReport validate(const ContentDraftRegistry& draft) const;
};
```

职责：跨文件引用校验。

### 10.7 ContentRuntimeAdapter

```cpp
class ContentRuntimeAdapter {
public:
    Result<DialogScenario> buildDialogScenario(const ContentRegistry& registry,
                                               ScenarioDefinitionKey scenario_key) const;

    Result<EffectExecutionSpecRegistry> buildEffectSpecs(const ContentRegistry& registry) const;

    Result<EffectSemanticRegistry> buildEffectSemantics(const ContentRegistry& registry) const;

    Result<ReactionRuleSet> buildReactionRules(const ContentRegistry& registry) const;
};
```

职责：把配置内容适配进旧系统。

重要边界：旧系统只能读 adapter 产物，不能自己解析 JSON。

## 11. 加载流程

### 11.1 启动加载

```text
1. 读取 ContentLoadOptions。
2. 定位 enabled_package_keys。
3. 读取每个 manifest.json。
4. 校验 manifest 基础字段和文件路径。
5. 按 load_order 排序。
6. 读取 manifest.files 中声明的 JSON。
7. 对每个文件按 category 解析到 DTO。
8. 放入 ContentDraftRegistry。
9. 执行结构校验。
10. 执行跨引用校验。
11. 检查重复 key 和 override 策略。
12. 构建只读 ContentRegistry。
13. 构建 RuntimeAdapters 所需注册表。
14. 返回 ContentLoadResult。
```

### 11.2 错误处理

错误分级：

- Fatal：manifest 无法读取、schema 不兼容、JSON 解析失败。
- Error：必填字段缺失、引用不存在、重复 key、非法枚举。
- Warning：deprecated 字段、未使用定义、测试内容未启用。
- Info：加载成功摘要。

运行时启动策略：

- Fatal / Error：正式运行拒绝启动内容包。
- Warning：允许启动，但测试中必须可断言。
- Dev 模式：允许加载 test content，但仍不允许 Error。

## 12. 与现有系统的接入

### 12.1 接入 ConfigLoader

P1 的 `ConfigLoader` 保留职责：读取文件、hash、路径安全。

P42 不应该把 JSON 解析塞回 `ConfigLoader`。

新增 `JsonContentLoader` 调用 `ConfigLoader`。

### 12.2 接入 DialogScenario

第一版可以通过 `ContentRuntimeAdapter::buildDialogScenario` 生成现有 `DialogScenario`。

这样 H5 最小可玩不需要大改。

迁移策略：

```text
旧：DialogScenarioCatalog::defaultScenario() 手写对象和反馈
新：DialogScenarioCatalog::defaultScenario() 优先读取 ContentRegistry 中 first_night
兜底：如果内容包不可用，使用内置最小场景并输出 warning trace
```

### 12.3 接入 EffectExecutionSpecRegistry

当前内置 `builtInEffectExecutionSpecs()` 保留为 fallback。

P42 新增：

```text
builtInEffectExecutionSpecs()
+ contentRegistry.effectSpecs()
```

冲突时第一版不允许覆盖。

### 12.4 接入 AgentReasoner

AgentReasoner 不直接读 JSON。

它读取 adapter 生成的：

- EffectSemantics。
- Preconditions。
- Reaction planning data。
- KnowledgeClaim。

### 12.5 接入 H5

H5 不读 JSON。

后端投影中可以新增：

- content_package_key。
- content_version。
- scenario_key。

用于调试和版本提示。

## 13. 最小迁移内容

P42 最小实现建议迁移以下内容：

### 13.1 Objects

- `red_berry`
- `decayed_red_berry`
- `bitter_leaf`
- `stone_flake`
- `axe`
- `wood`
- `whetstone`
- `dry_grass`
- `fire_seed`
- `camp_fire`
- `torch`
- `beast_shadow`

### 13.2 Effects

- `restore_hunger`
- `poison`
- `no_visible_effect`
- `use_hint`
- `cut_wood`
- `restore_sharpness`
- `ignite_fire`
- `make_torch`
- `repel_beast`
- `fire_is_dangerous`

### 13.3 Feedbacks

- 红果吃。
- 腐烂红果吃。
- 苦叶吃。
- 石片使用。
- 斧头砍木头。
- 磨石打磨斧头。
- 火种点燃干草。
- 制作火把。
- 火把驱赶野兽。

### 13.4 Threats

- `beast_shadow` 威胁进度。
- `beast_shadow` countermeasure。
- `beast_shadow` reentry。

## 14. 示例：新增毒蘑菇需要写哪些 JSON

新增毒蘑菇不应该改核心 C++。

需要新增：

1. `objects/mushrooms.json`

```json
{
  "objects": [
    {
      "object_key": "poison_mushroom",
      "display_key": "object.poison_mushroom.name",
      "description_key": "object.poison_mushroom.desc",
      "kind": "consumable",
      "safe_tags": ["plant", "mushroom", "food_like"],
      "allowed_actions": ["eat", "use", "inspect"],
      "initial_state": { "quantity": 2 }
    }
  ]
}
```

2. `feedbacks/mushrooms.json`

```json
{
  "feedbacks": [
    {
      "feedback_key": "poison_mushroom_eat_poison",
      "object_key": "poison_mushroom",
      "action_key": "eat",
      "effect_key": "poison",
      "reply_key": "feedback.poison_mushroom.eat.poison",
      "utility_delta": -0.8,
      "risk_delta": 0.9,
      "knowledge": {
        "subject_object_key": "poison_mushroom",
        "action_key": "eat",
        "effect_key": "poison"
      }
    }
  ]
}
```

3. `scenarios/first_night.json` 增加初始对象。

4. `locales/zh_cn.json` 增加中文文案。

如果要支持晒干入药，再新增 reaction 和 effect，不改已有 poison_mushroom 定义。

## 15. 示例：新增斧头耐久扩展

后续如果要让斧头用几次变钝，不应该写死在 `WorldInteractionService`。

应通过 effect operation 表达：

```json
{
  "effect_key": "cut_wood",
  "operations": [
    { "op": "consume_object_quantity", "target": "wood", "quantity": 1 },
    { "op": "add_object_state_number", "target": "wood", "state_key": "processed", "delta": 1 },
    { "op": "add_object_state_number", "target": "axe", "state_key": "sharpness", "delta": -1 }
  ],
  "preconditions": [
    "condition:test:eq:object.quantity.wood.gte.1",
    "condition:test:eq:object.quantity.axe.gte.1",
    "condition:test:eq:object.state.axe.sharpness.gte.1"
  ]
}
```

打磨斧头：

```json
{
  "effect_key": "restore_sharpness",
  "operations": [
    { "op": "set_object_state_number", "target": "axe", "state_key": "sharpness", "value": 3 }
  ]
}
```

## 16. 测试设计

### 16.1 Loader 测试

- manifest 可读取。
- 文件路径禁止 `..`。
- 缺失 required 文件报错。
- 空文件 warning。
- hash 稳定。

### 16.2 Parser 测试

- objects JSON 转 DTO。
- effects JSON 转 DTO。
- feedbacks JSON 转 DTO。
- reactions JSON 转 DTO。
- threats JSON 转 DTO。
- 非法枚举报错。
- 字段类型错误报错。

### 16.3 Reference 测试

- feedback 引用不存在 object 报错。
- effect operation 引用非法 op 报错。
- reaction 输出不存在 object 报错。
- scenario 引用不存在 object 报错。
- threat countermeasure 引用不存在 effect 报错。

### 16.4 Runtime Adapter 测试

- JSON red_berry 能生成 DialogScenarioObject。
- JSON restore_hunger 能生成 EffectExecutionSpec。
- JSON make_torch 能进入 ReactionPlanningAdapter。
- JSON beast_shadow reentry 能影响 ThreatProgressInput。

### 16.5 端到端测试

使用 JSON core 内容包跑最小 H5 流程：

```text
bootstrap
吃红果
吃腐烂红果
用斧头砍木头
用火种点燃干草
制作火把
用火把驱赶野兽
等待三次
教同伴
```

验收：

- 不出现 feedback_not_found。
- 知识能形成。
- 物品数量能变化。
- 反应产物可用。
- 同伴不会凭空使用玩家物品。
- 野兽状态来自 threat 配置。

## 17. 实现任务拆分

### TASK-P42-001：内容目录与 fixture

新增：

```text
content/core/manifest.json
content/core/objects/basic_survival.json
content/core/effects/basic_effects.json
content/core/feedbacks/basic_feedbacks.json
content/core/reactions/basic_reactions.json
content/core/threats/basic_threats.json
content/core/scenarios/first_night.json
content/core/locales/zh_cn.json
```

只写最小内容，先让测试能读。

### TASK-P42-002：DTO 与枚举

新增：

```text
backend/include/pathfinder/content/content_types.h
backend/src/content/content_types.cpp
```

包含枚举、DTO、toString/fromString、validateBasic。

### TASK-P42-003：JsonContentLoader

新增：

```text
backend/include/pathfinder/content/json_content_loader.h
backend/src/content/json_content_loader.cpp
```

第一版可使用项目已有 JSON 解析策略；如果没有通用 JSON parser，必须先实现最小安全 parser 或引入明确依赖，不允许用字符串拼接解析业务字段。

### TASK-P42-004：ContentRegistry

新增：

```text
backend/include/pathfinder/content/content_registry.h
backend/src/content/content_registry.cpp
```

支持按 key 查询和重复 key 检测。

### TASK-P42-005：结构校验与引用校验

新增：

```text
backend/include/pathfinder/content/content_validation.h
backend/src/content/content_validation.cpp
```

输出 ConfigValidationReport。

### TASK-P42-006：Runtime Adapter

新增：

```text
backend/include/pathfinder/content/content_runtime_adapter.h
backend/src/content/content_runtime_adapter.cpp
```

第一版至少支持：

- buildDialogScenario。
- buildEffectSpecs。
- buildEffectSemantics。

### TASK-P42-007：H5 最小接入

`DialogScenarioCatalog::defaultScenario()` 优先使用 core content。

保留 fallback。

### TASK-P42-008：测试与门禁

新增测试：

```text
backend/tests/unit/content/content_loader_test.cpp
backend/tests/unit/content/content_registry_test.cpp
backend/tests/unit/content/content_validation_test.cpp
backend/tests/unit/content/content_runtime_adapter_test.cpp
backend/tests/integration/p42/json_content_h5_flow_test.cpp
```

## 18. 风险与防线

### 18.1 风险：JSON 变成第二套规则引擎

防线：JSON 只能声明数据、条件和白名单操作。

不允许脚本、不允许动态代码、不允许前端执行 JSON。

### 18.2 风险：内容 key 改名导致存档坏掉

防线：内容 key 一旦进入存档和知识，禁止改名。

如果必须改名，必须提供 alias/migration 表。

第一版只设计 `deprecated_aliases` 字段，不实现复杂迁移。

### 18.3 风险：配置错误运行到一半才爆炸

防线：启动时完成结构和引用校验。

Error 级别禁止进入正式运行。

### 18.4 风险：低级 AI 新增内容漏写文案或测试

防线：validator 检查 display_key/summary_key 是否存在 locale；每个新增内容包必须有 tests JSON 或 C++ fixture。

### 18.5 风险：H5 旧场景和 JSON 场景双轨

防线：H5 只通过 DialogScenario/Projection 接收内容，不直接读取旧场景常量。

旧 C++ 场景只作为 fallback，并输出 trace。

## 19. 验收标准

P42 设计完成验收：

- 文档写清目录结构、DTO、枚举、接口、加载流程、测试点。
- 明确 JSON 不是运行时状态源。
- 明确前端不读 JSON。
- 明确 P27 条件表达式是条件权威。
- 明确 EffectOperation 白名单是执行权威。
- 明确新增内容示例。

P42 实现完成验收：

- 可以从 `content/core/manifest.json` 加载最小内容包。
- 可以构建 `ContentRegistry`。
- 可以发现非法引用。
- 可以把 JSON 内容适配到最小 H5 场景。
- 当前 H5 最小流程不退化。
- 新增毒蘑菇只需要 JSON + locale + 测试，不需要改核心执行代码。

## 20. 禁止清单

实现 P42 时禁止：

- 禁止前端读取 JSON 推导按钮结果。
- 禁止 JSON 中出现 C++ 函数名或脚本代码。
- 禁止通过中文显示名做 key 匹配。
- 禁止新增内容时修改核心 if/else 作为常规路径。
- 禁止让 JSON 直接写 KnowledgeClaim 实例。
- 禁止让 JSON 直接写 SaveGame 运行时状态。
- 禁止同一个 object_key 在多个包中静默覆盖。
- 禁止配置错误时只打日志继续运行。
- 禁止测试只测 parser，不测端到端内容链。

## 21. 后续阶段

P42 完成后，建议后续阶段调整为：

- P43：基础内容包扩展，基于 JSON 增加毒蘑菇、普通蘑菇、狼、陷阱、房子、基础天气。
- P44：存档读档与内容版本兼容。
- P45：地图与采集资源点。
- P46：专家 NPC 与教学内容包。
- P47：族群自动生产与资源调度。
- P48：训练环境前置。

## 22. 给实现 AI 的 Context Pack

实现本阶段前必须阅读：

```text
doc/00_设计文档编写要求.md
doc/05_配置系统设计.md
doc/26_P1配置与命令任务卡设计.md
doc/54_P27受限条件表达式全量重构任务卡设计.md
doc/56_P28_5配置化物品与反应流水线任务卡设计.md
doc/69_P40动态目标执行与外界打断系统任务卡设计.md
doc/70_P41智能NPC通用执行闭环重构任务卡设计.md
```

必须先跑或补齐配置相关测试，再接入 H5。

## 23. 自检清单

- [x] 写清楚阶段定位。
- [x] 写清楚为什么现在做。
- [x] 写清楚不解决的问题。
- [x] 写清楚目录结构。
- [x] 写清楚枚举。
- [x] 写清楚 DTO。
- [x] 写清楚接口。
- [x] 写清楚加载流程。
- [x] 写清楚权威边界。
- [x] 写清楚测试。
- [x] 写清楚禁止清单。
- [x] 写清楚新增内容示例。
