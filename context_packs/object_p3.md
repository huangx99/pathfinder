# object P3 上下文包

## 1. P3 目标

实现 object 最小模块，承载 P3 unknown fruit + eat 单条规则闭环所需的对象定义和运行时状态。

P3 完成后必须满足：

```text
ObjectDefinition 可表达 unknown_fruit 的静态定义。
WorldObject 可持有一个 unknown fruit 实例。
ObjectStore 可管理定义和对象。
所有结构可测试。
不做完整对象库。
不做完整地图索引。
不做完整背包系统。
不做完整 ConfigRegistry。
```

## 2. P3 边界

### 2.1 允许实现

```text
ObjectCategory: Unknown / Food / Material / Tool / Hazard
ObjectRuntimeFlag: None / Consumed / Depleted
EdibleEffectKind: Unknown / Edible / Harmful / Neutral
ObjectEdibleProfile: effect_kind / hunger_delta / health_delta / risk_level
ObjectDefinition: definition_id / category / allowed_action_ids / edible_profile
WorldObject: object_id / definition_id / quantity / flags
ObjectStore: definitions / objects / addDefinition / addObject / findDefinition / findObject / markConsumed / updateObject
validateBasic 检查
toString / fromString for enums
```

### 2.2 禁止实现

```text
完整对象库。
完整地图索引。
完整背包系统。
完整对象组合反应。
完整 ConfigRegistry。
ObjectFactory。
ObjectSnapshot。
ObjectContextProvider。
RegionObjectIndex。
ObjectQuery。
ObjectRelation。
颜色、形状、外观 reveal 字段。
```

## 3. 依赖约束

### 3.1 允许依赖

```text
foundation: ErrorCode / ErrorDetail / Result<T> / StrongId / Tick / StateVersion / HashValue / isValidIdString
```

### 3.2 禁止依赖

```text
config
command
state
event
pipeline
engine
rules
agent
protocol
frontend
```

## 4. 允许修改的目录

```text
backend/include/pathfinder/object/
backend/src/object/
backend/tests/unit/object/
backend/dev_notes/object.md
backend/dev_notes/foundation.md (如果需要新增 ID 类型)
backend/include/pathfinder/foundation/id.h (如果需要新增 ID 类型)
```

## 5. 禁止修改的目录

```text
backend/include/pathfinder/state/ (除非编译必须)
backend/include/pathfinder/event/
backend/include/pathfinder/pipeline/
backend/include/pathfinder/config/
backend/include/pathfinder/command/
backend/include/pathfinder/engine/
backend/include/pathfinder/rules/
backend/include/pathfinder/agent/
backend/include/pathfinder/protocol/
frontend/
```

## 6. 测试要求

```text
每个枚举 toString 稳定。
合法字符串可解析。
非法字符串解析失败。
合法 ObjectDefinition 可加入 store。
缺 definition_id 失败。
缺 object_id 失败。
object 引用不存在 definition 失败或在 store 校验中失败。
findObject 可找到对象。
markConsumed 后对象标记 consumed。
ObjectEdibleProfile 默认值安全。
```

## 7. 验收命令

```bash
cmake --build build/backend
ctest --test-dir build/backend -R object --output-on-failure
ctest --test-dir build/backend -R foundation --output-on-failure
```

## 8. 常见错误

```text
在 WorldObject 中复制完整 ObjectDefinition。
在 WorldObject 中保存玩家认知。
加入颜色、形状、外观 reveal 字段。
设计完整物品系统。
实现 ConfigRegistry。
实现 ObjectFactory。
实现 RegionObjectIndex。
```

## 9. 任务执行顺序

```text
TASK-P3-002: 实现 Object 基础类型 (ObjectCategory / ObjectRuntimeFlag / EdibleEffectKind / ObjectEdibleProfile)
TASK-P3-003: 实现 WorldObject / ObjectStore (ObjectDefinition / WorldObject / ObjectStore)
```

## 10. 前置文档索引

```text
doc/06_世界对象设计.md: ObjectDefinition / WorldObject 关系 (第 1-5 章)
doc/28_P3未知对象首条规则闭环任务卡设计.md: P3 任务卡
```

## 11. 设计关键约束

```text
ObjectDefinition 定义"这类东西是什么"。
WorldObject 表示"世界中这一具体东西现在怎么样"。
WorldObject 不保存完整 ObjectDefinition，只保存 definition_id。
WorldObject 不保存玩家认知。
对象状态变化必须通过 StateChange。
认知保存在 CognitionState，不保存在 WorldObject。
```
