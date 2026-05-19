# P37 最小剧情闭环 Context Pack

## 任务目标

实现 P37“第一天生存”最小剧情闭环。

本阶段不是大地图、正式 UI、完整战斗或自由文本系统。

目标是把已有系统串成 5-10 分钟可玩流程：

```text
观察 -> 尝试 -> 学习 -> 修正 -> 教同伴 -> 制作/使用火源或火把 -> 解决夜晚危险 -> 撑过第一夜
```

## 必读文档

```text
doc/00_设计文档编写要求.md
doc/66_P37最小剧情闭环任务卡设计.md
doc/65_P36行为目标知识最小升级设计.md
doc/64_P35状态化对象规则后端最小实现设计.md
doc/62A_P34前置认知因果归因系统任务卡设计.md
doc/56_P28_5配置化物品与反应流水线任务卡设计.md
doc/55_P28对象反应系统任务卡设计.md
doc/57_P29危险与探索压力任务卡设计.md
doc/60_P32协议投影扩展任务卡设计.md
doc/61_P33H5极简可玩验证壳任务卡设计.md
```

## 关键约束

1. H5 前端不是规则层。
2. 知识必须走 P21/P36。
3. 反应产物必须走 P28/P28.5。
4. 危险必须走 P29。
5. 工具状态必须走 P35。
6. 因果归因必须兼容 P34 时间窗口。
7. Player audience 不得输出 hidden_truth/raw_state/actual_hp/death/loot/drop/debug expression。
8. P37 野兽是危险事件，不是完整战斗实体。

## 最小内容闭环

```text
物品：red_berry、decayed_red_berry、bitter_leaf、stone_flake、axe、wood、whetstone、dry_grass、fire_seed、camp_fire、torch。
危险：approaching_beast，必要时可附带 cold_night。
目标：探索食物、准备火源、教同伴关键知识、撑过夜晚。
行动：观察、吃、使用、对目标使用、组合、等待、教同伴、查看知识。
```

## 推荐实现顺序

1. 枚举和 DTO。
2. 场景定义和校验。
3. 运行状态和时间推进。
4. 目标评估。
5. 反应桥接。
6. 危险桥接。
7. H5 projection 补充。
8. 专项与集成测试。

## 禁止实现

```text
前端写死火把能驱赶野兽。
直接添加 torch 到前端状态。
直接扣 HP 或写 death。
直接修改知识仓库。
直接修改 axe.sharpness。
绕过 ReactionResolver。
绕过 DangerResolver。
新增完整地图、背包、战斗系统。
```
