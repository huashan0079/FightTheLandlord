# 斗地主牌型识别与手牌分析系统设计文档

## 1. 项目概述

本项目是一个**斗地主（Chinese Poker/Doudizhu）游戏的牌型识别与手牌分析系统**，核心功能包括：
- 标准扑克牌（54张）的表示与等级映射
- 合法牌型的识别与验证（单张、对子、顺子、炸弹、火箭等16种牌型）
- 手牌的组合分析（自动拆分单张、对子、三条、顺子等）
- 出牌模拟与代价评估（用于AI决策）

**作者意图**：为斗地主AI提供底层牌型处理框架，支持手牌分析、出牌合法性验证、出牌策略评估。

---

## 2. 数学结构建模

### 2.1 牌面编码系统

每张牌用一个整数 `CardNum` 表示（0-53）：
- **0-51**：普通牌（每种花色4张），等级计算：`level = card / 4`（0-12对应3-K，13对应A，14对应2）
- **52**：小王（Joker），等级 `level_joker = 13`
- **53**：大王（JOKER），等级 `level_JOKER = 14`

**等级范围**：`Level` 类型为 `short`，值域 0-14：
- 0-12：3到K
- 13：A
- 14：2
- 13：小王（与A同等级）
- 14：大王（与2同等级）

### 2.2 牌型分类与分值系统

| 牌型 | 枚举值 | 基础分 | 说明 |
|------|--------|--------|------|
| PASS | 0 | 0 | 过牌 |
| SINGLE | 1 | 1 | 单张 |
| PAIR | 2 | 2 | 对子 |
| STRAIGHT | 3 | 6 | 顺子（≥5张连续单牌） |
| STRAIGHT2 | 4 | 6 | 双顺（≥3对连续对子） |
| TRIPLET | 5 | 4 | 三条 |
| TRIPLET1 | 6 | 4 | 三带一 |
| TRIPLET2 | 7 | 4 | 三带二 |
| BOMB | 8 | 10 | 炸弹（四张相同） |
| QUADRUPLE2 | 9 | 8 | 四带二（单） |
| QUADRUPLE4 | 10 | 8 | 四带二（对） |
| PLANE | 11 | 8 | 飞机（≥2连续三条） |
| PLANE1 | 12 | 8 | 飞机带小翼（单牌） |
| PLANE2 | 13 | 8 | 飞机带大翼（对子） |
| SSHUTTLE | 14 | 10 | 航天飞机（≥2连续四张） |
| SSHUTTLE2 | 15 | 10 | 航天飞机带小翼 |
| SSHUTTLE4 | 16 | 10 | 航天飞机带大翼 |
| ROCKET | 17 | 16 | 火箭（王炸） |

### 2.3 牌力评分算法

每种牌型的 `power()` 函数计算方式：

```cpp
// 单张：100 + 等级
Single::power() = 100 + level

// 对子：200 + 等级
Pair::power() = 200 + level

// 三条：300 + 等级
Triple::power() = 300 + level

// 炸弹：1000 + 等级
Bomb::power() = 1000 + level

// 顺子：长度×10 + 最大等级
Straight::power() = levels.size() * 10 + levels.back()

// 连对：长度×5 + 最大等级
ConsecutivePairs::power() = levels.size() * 5 + levels.back()

// 飞机：长度×3 + 最大等级
Plane::power() = levels.size() * 3 + levels.back()
```

### 2.4 出牌代价模型

`simulatePlay()` 函数评估出牌策略的**代价**（cost），代价越小越好：

```
cost = 
    + (拆炸弹代价) ? 500 : 0
    + (拆飞机代价) ? 300 : 0
    + (顺子牌力损失) = Σ(原顺子power) - Σ(新顺子power)
    + (单张增加代价) = 新增单张数量 × 30
```

**设计理念**：炸弹和飞机是高价值组合，拆分会付出高代价；单张增加会增加手牌离散度，也应惩罚。

---

## 3. 模块用途与作者意图

| 模块 | 用途 | 作者意图 |
|------|------|----------|
| `card` 类 | 单张牌的表示，支持等级比较 | 统一牌面编码，简化等级获取 |
| `CardComboType` 枚举 | 定义16种斗地主合法牌型 | 分类所有可能的出牌组合 |
| `Meld` 类族 | 各牌型结构体（Single, Pair, Triple等） | 面向对象封装牌型行为（power, canPlay, play） |
| `MeldVariant` | 使用 `std::variant` 存储多态牌型 | 类型安全的联合体，避免虚函数开销 |
| `Gamer` 类 | 手牌容器，管理牌堆和分析 | 核心数据结构，支持手牌快照和回溯 |
| `analyze()` | 自动拆分手牌为各牌型组合 | 为AI提供手牌结构视图 |
| `findSegments()` | 查找连续区间（顺子/连对/飞机） | 通用算法，复用性强 |
| `simulatePlay()` | 模拟出牌并计算代价 | 供AI评估不同出牌策略的优劣 |

**作者意图总结**：  
构建一个**可扩展、高性能**的斗地主牌型处理库，支持：
1. 手牌自动分析（拆解为基本牌型）
2. 出牌合法性验证
3. 出牌策略量化评估（代价模型）
4. 方便集成到更高层的AI决策系统中

---

## 4. 使用方法

### 4.1 基本初始化

```cpp
#include "Gamer.h"  // 包含所有必要头文件

// 创建玩家手牌
Gamer player;

// 添加手牌（使用牌面编号0-53）
std::vector<CardNum> hand = {
    0, 1, 2, 3,      // 4张3
    13, 14, 15, 16,  // 4张4
    26, 27,          // 2张5
    52, 53           // 大小王
};
player.addCards(hand);
```

### 4.2 手牌分析

```cpp
player.analyze();

// 查看分析结果
for (auto& s : player.singles) {
    std::cout << "单张: 等级" << s.level << std::endl;
}
for (auto& p : player.pairs) {
    std::cout << "对子: 等级" << p.level << std::endl;
}
for (auto& t : player.triples) {
    std::cout << "三条: 等级" << t.level << std::endl;
}
for (auto& b : player.bombs) {
    std::cout << "炸弹: 等级" << b.level << std::endl;
}
for (auto& s : player.straights) {
    std::cout << "顺子: ";
    for (auto l : s.levels) std::cout << l << " ";
    std::cout << std::endl;
}
```

### 4.3 创建并验证出牌

```cpp
// 创建单张牌型
Single single{5};  // 等级5的牌
if (single.canPlay(player)) {
    // 合法，执行出牌
    single.play(player);
}

// 创建顺子
Straight straight{ {3, 4, 5, 6, 7} };
if (straight.canPlay(player)) {
    straight.play(player);
}

// 统一使用 Meld 容器
Meld meld = Pair{10};  // 对10
if (meld.canPlay(player)) {
    meld.play(player);
    std::cout << "牌型: " << (int)meld.type() << std::endl;
    std::cout << "牌力: " << meld.power() << std::endl;
}
```

### 4.4 出牌策略评估

```cpp
Gamer hand;
hand.addCards(hand_cards);
hand.analyze();

// 准备要出的牌（多个Meld组合）
std::vector<Meld> move = {
    Straight{{3,4,5,6,7}},
    Pair{8}
};

int cost = hand.simulatePlay(hand, move);
std::cout << "出牌代价: " << cost << std::endl;
// 代价越小越好，可配合AI决策算法（如Minimax）
```

### 4.5 手牌快照与回溯

```cpp
// 保存当前手牌状态
auto snapshot = player.save();

// 执行一系列操作...
player.tryPlay(5);  // 打出一张等级5的牌

// 恢复手牌
player.load(snapshot);
```

---

## 5. 主要函数说明

### 5.1 `Gamer` 类

| 函数 | 说明 |
|------|------|
| `addCard(CardNum)` | 添加单张牌 |
| `addCards(const vector<CardNum>&)` | 批量添加手牌 |
| `clear()` | 清空所有手牌 |
| `count(Level)` | 返回某等级牌的数量 |
| `analyze()` | 分析手牌，填充 `singles/pairs/triples/bombs/straights/consec_pairs/planes` |
| `tryPlay(Level)` | 尝试打出一张牌（返回是否成功） |
| `tryPlay(const vector<Level>&)` | 尝试打出多张牌 |
| `save()` | 保存手牌快照 |
| `load(const Snapshot&)` | 恢复手牌快照 |
| `simulatePlay(Gamer&, vector<Meld>&)` | 模拟出牌并计算代价 |
| `findSegments(min_count, min_length, start, end)` | 查找连续区间（内部使用） |

### 5.2 牌型结构体（统一接口）

每种牌型都实现以下方法：

| 方法 | 说明 |
|------|------|
| `power()` | 返回牌力值（用于比较大小） |
| `required_levels()` | 返回所需牌等级列表 |
| `canPlay(const Gamer&)` | 检查当前手牌能否打出 |
| `play(Gamer&)` | 从手牌中移除这些牌 |

### 5.3 `Meld` 包装类

| 方法 | 说明 |
|------|------|
| `Meld(T&&)` | 构造函数，接受任意牌型 |
| `power()` | 转发到内部牌型的 `power()` |
| `type()` | 返回 `MeldType` 枚举 |
| `canPlay(Gamer&)` | 转发合法性检查 |
| `play(Gamer&)` | 转发出牌操作 |

---

## 6. 内部工作机制

```
用户代码
    │
    ├─► 创建 Gamer 对象
    │     └─► addCards() 填充手牌
    │
    ├─► 调用 analyze()
    │     ├─► 遍历0-12级，统计数量
    │     ├─► 填充 singles/pairs/triples/bombs
    │     ├─► 调用 findSegments(1,5,0,11) → 单顺
    │     ├─► 调用 findSegments(2,3,0,11) → 连对
    │     ├─► 调用 findSegments(3,2,0,11) → 飞机
    │     └─► 检查火箭（大小王）
    │
    ├─► 创建 Meld 牌型
    │     └─► canPlay() 验证合法性
    │          └─► 检查手牌中是否有足够数量的对应等级牌
    │
    ├─► 执行 play()
    │     └─► tryPlay() 从 levels 中删除对应牌
    │
    └─► simulatePlay() 评估策略
          ├─► clone() 深拷贝手牌
          ├─► 依次执行 move 中的 Meld
          ├─► 重新 analyze()
          ├─► 对比前后状态变化
          └─► 返回代价分数
```

---

## 7. 扩展与优化建议

### 7.1 支持的牌型扩展

当前框架已预留扩展接口，可轻松添加：
- 四带两单（`QUADRUPLE2`）
- 四带两对（`QUADRUPLE4`）
- 飞机带翅膀（`PLANE1/PLANE2`）
- 航天飞机系列（`SSHUTTLE*`）

只需在 `MeldVariant` 中添加新类型，实现对应结构体即可。

### 7.2 性能优化

- 使用 `std::variant` 替代虚函数，零开销多态
- `levels` 使用 `vector<vector<card>>` 固定大小，索引 O(1)
- 快照使用 `vector<size_t>` 轻量级存储

### 7.3 待完善功能

- `simulatePlay()` 中的代价模型较简单，可引入机器学习优化
- 缺少对"三带一/三带二"的具体实现（需要额外的单牌/对子参数）
- 缺少对"四带二"的实现

### 7.4 集成到AI系统

```cpp
// 简单贪心AI示例
Meld selectBestMove(Gamer& hand) {
    hand.analyze();
    
    // 优先出炸弹（最大牌力）
    if (!hand.bombs.empty()) {
        return Meld{hand.bombs[0]};
    }
    // 其次出顺子
    if (!hand.straights.empty()) {
        return Meld{hand.straights[0]};
    }
    // 最后出单张
    if (!hand.singles.empty()) {
        return Meld{hand.singles[0]};
    }
    return Meld{Pass{}};
}
```

---

## 8. 注意事项

- **等级范围**：`Level` 值 0-14，其中13同时对应A和小王，14同时对应2和大王，使用时需注意区分。
- **手牌一致性**：执行 `play()` 前务必调用 `canPlay()` 验证，否则可能因牌数不足而断言失败（当前版本无错误处理）。
- **快照有效性**：`save()` 保存的是各等级牌的数量，恢复时假设 `levels` 结构未改变。
- **线程安全**：当前设计非线程安全，多线程环境需外部加锁。
- **依赖项**：需要 C++17 或更高版本（`std::variant`, `std::visit`）。
