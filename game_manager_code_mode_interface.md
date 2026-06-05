# GameManager 需要新增/修改的接口（代码执行模式）

下面这些接口是为了配合 MainWindow 的“代码块显示 + 高亮执行”流程。
MainWindow 不直接扣血、不直接执行卡牌效果，只负责动画、高亮、刷新界面。

## 1. 新增结构体

放在 `game_manager.h` 的 `CardView / DrawResult / PlayResult / TurnResult` 附近。

```cpp
struct CodeCommandView {
    QString title;       // 例如："普通攻击"、"敌人行动"
    QStringList lines;   // 展示在代码框中的代码；for/if 可以有多行
    bool executed = false;
};
```

如果 `QStringList` 报错，需要在 `game_manager.h` 顶部加：

```cpp
#include <QStringList>
```

## 2. GameManager 新增公开接口

放在 `class GameManager public:` 里。

```cpp
// ---- 代码执行模式 ----
void beginTurnWithoutDraw();
void prepareTurnCodeBlock();
PlayResult playCardAsCode(int handIndex, Enemy* target = nullptr);

QVector<CodeCommandView> getCodeCommandViews() const;
int pendingCodeCount() const;
void executePendingCode(int index);
TurnResult finishTurnAfterCodeExecution();
```

## 3. GameManager 新增私有结构和成员

放在 `class GameManager private:` 里。

```cpp
struct PendingCodeCommand {
    QString title;
    QStringList lines;
    std::function<void()> effect;
    bool executed = false;
};

QVector<PendingCodeCommand> pendingCommands;

QStringList buildCardCodeLines(const CardView& card) const;
QStringList buildEnemyCodeLines(Enemy* enemy) const;
void insertPlayerCommandBeforeEnemy(PendingCodeCommand cmd);
```

## 4. 推荐实现逻辑

### beginTurnWithoutDraw()

```cpp
void GameManager::beginTurnWithoutDraw()
{
    turnNumber++;
    player.resetActionLimits();
    growMaxEnergy();
    restoreEnergy();

    if (battle.onTurnStart) {
        battle.onTurnStart(turnNumber);
    }
}
```

### prepareTurnCodeBlock()

每回合开始时，先清空上回合代码，然后把敌人将调用的函数写入代码块。

```cpp
void GameManager::prepareTurnCodeBlock()
{
    pendingCommands.clear();

    for (const auto& enemyPtr : battle.enemies) {
        if (!enemyPtr || !enemyPtr->isAlive()) {
            continue;
        }

        Enemy* enemy = enemyPtr.get();

        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(enemy->name);
        cmd.lines = buildEnemyCodeLines(enemy);

        // 当前简化：所有敌人行动作为一个命令执行。
        // 如果有多个敌人，也可以只给最后一个 enemy command 设置 effect，或者拆成逐个 takeTurn。
        cmd.effect = [this]() {
            battle.executeAttackPhase();
        };

        pendingCommands.push_back(std::move(cmd));
        break; // 简化版：只放一个“敌人行动”块，避免 executeAttackPhase 被多次调用
    }
}
```

### playCardAsCode()

玩家打出卡牌时：扣能量、移到弃牌堆、写入代码，但不立即执行 `card->play()`。

```cpp
PlayResult GameManager::playCardAsCode(int handIndex, Enemy* target)
{
    if (handIndex < 0 || handIndex >= static_cast<int>(hand.size())) {
        return {false, handIndex, {}, QStringLiteral("手牌不存在")};
    }

    Card* card = hand[handIndex].get();
    if (!card) {
        return {false, handIndex, {}, QStringLiteral("手牌不存在")};
    }

    if (!card->canPlay(player)) {
        return {false, handIndex, {}, QStringLiteral("当前无法打出此牌")};
    }

    if (player.energy < card->cost) {
        return {false, handIndex, {}, QStringLiteral("能量不足")};
    }

    CardView view{QString::fromStdString(card->name),
                  QString::fromStdString(card->description),
                  card->cost,
                  card->targetMode};

    spendEnergy(card->cost);

    Card* rawCard = card;
    Enemy* rawTarget = target;

    PendingCodeCommand cmd;
    cmd.title = view.name;
    cmd.lines = buildCardCodeLines(view);
    cmd.effect = [this, rawCard, rawTarget]() {
        rawCard->play(player, rawTarget);
    };

    insertPlayerCommandBeforeEnemy(std::move(cmd));

    discardPile.push_back(std::move(hand[handIndex]));
    hand.erase(hand.begin() + handIndex);

    return {true, handIndex, view, ""};
}
```

### insertPlayerCommandBeforeEnemy()

玩家代码应该出现在敌人代码之前。

```cpp
void GameManager::insertPlayerCommandBeforeEnemy(PendingCodeCommand cmd)
{
    int insertPos = pendingCommands.size();

    for (int i = 0; i < pendingCommands.size(); ++i) {
        // 简化判断：敌人代码 title 不是卡牌名，也可以加 source 字段更严谨
        if (pendingCommands[i].title.contains(QStringLiteral("哥布林")) ||
            pendingCommands[i].title.contains(QStringLiteral("敌人")) ||
            pendingCommands[i].title.contains(QStringLiteral("Goblin"))) {
            insertPos = i;
            break;
        }
    }

    pendingCommands.insert(insertPos, std::move(cmd));
}
```

更严谨的版本可以给 `PendingCodeCommand` 增加 `bool isEnemyCommand`。

### getCodeCommandViews()

```cpp
QVector<CodeCommandView> GameManager::getCodeCommandViews() const
{
    QVector<CodeCommandView> result;

    for (const PendingCodeCommand& cmd : pendingCommands) {
        result.push_back({cmd.title, cmd.lines, cmd.executed});
    }

    return result;
}
```

### pendingCodeCount()

```cpp
int GameManager::pendingCodeCount() const
{
    return pendingCommands.size();
}
```

### executePendingCode()

```cpp
void GameManager::executePendingCode(int index)
{
    if (index < 0 || index >= pendingCommands.size()) {
        return;
    }

    PendingCodeCommand& cmd = pendingCommands[index];

    if (cmd.executed) {
        return;
    }

    if (cmd.effect) {
        cmd.effect();
    }

    cmd.executed = true;
}
```

### finishTurnAfterCodeExecution()

```cpp
TurnResult GameManager::finishTurnAfterCodeExecution()
{
    if (isBattleOver()) {
        finishBattle(isPlayerWin());
        return {true, isPlayerWin()};
    }

    battle.executeEndPhase();

    if (isBattleOver()) {
        finishBattle(isPlayerWin());
        return {true, isPlayerWin()};
    }

    if (battle.onTurnEnd) {
        battle.onTurnEnd(turnNumber);
    }

    pendingCommands.clear();
    return {false, false};
}
```

### buildCardCodeLines()

先用卡牌名做简单映射，后面可以改成 Card 类的虚函数。

```cpp
QStringList GameManager::buildCardCodeLines(const CardView& card) const
{
    if (card.name.contains(QStringLiteral("攻击")) || card.name.contains(QStringLiteral("Strike"))) {
        return {"player.attack(enemy);"};
    }

    if (card.name.contains(QStringLiteral("防御")) || card.name.contains(QStringLiteral("Defend"))) {
        return {"player.basicDefend();"};
    }

    if (card.name.contains(QStringLiteral("治疗")) || card.name.contains(QStringLiteral("Heal"))) {
        return {"player.heal(10);"};
    }

    if (card.name.contains(QStringLiteral("三连"))) {
        return {"for (int i = 0; i < 3; ++i) {",
                "    player.attack(enemy);",
                "}"};
    }

    return {QString("// %1").arg(card.name), "card.play(player, enemy);"};
}
```

### buildEnemyCodeLines()

```cpp
QStringList GameManager::buildEnemyCodeLines(Enemy* enemy) const
{
    if (!enemy) {
        return {"// enemy_function...", "enemy.wait();"};
    }

    QString name = QString::fromStdString(enemy->name);
    QString action = QString::fromStdString(enemy->nextIntent.name());

    if (enemy->nextIntent.type == EnemyIntent::ATTACK) {
        return {"// enemy_function...",
                QString("%1.attack(player); // %2").arg(name).arg(enemy->nextIntent.value)};
    }

    if (enemy->nextIntent.type == EnemyIntent::DEFEND) {
        return {"// enemy_function...",
                QString("%1.defend(); // %2").arg(name).arg(enemy->nextIntent.value)};
    }

    return {"// enemy_function...",
            QString("%1.%2(player);").arg(name).arg(action)};
}
```

## 5. 原有接口仍然保留

`startTurn()` 和 `endTurn()` 可以保留给纯逻辑测试用，但 Qt 主界面不要直接调用它们，因为它们会自动抽牌/自动进入下一回合，无法配合动画和代码高亮。
