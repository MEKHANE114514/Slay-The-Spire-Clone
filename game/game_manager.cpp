#include "game_manager.h"
#include <cstdlib>   // rand, srand
#include <ctime>     // time
#include <algorithm> // shuffle, remove_if

// 静态关卡变量定义
int GameManager::currentLevel = 1;

GameManager::GameManager()
    : battle(player)
{
    srand(static_cast<unsigned>(time(nullptr)));
    initLevel();
    initDeck();
}

// ============================================================
// 关卡初始化
// ============================================================

void GameManager::initLevel() {
    switch (currentLevel) {
        case 1:
            battle.addEnemy(std::make_unique<Goblin>());
            break;
        case 2:
            battle.addEnemy(std::make_unique<Goblin>());
            battle.addEnemy(std::make_unique<Goblin>());
            break;
        case 3:
            battle.addEnemy(std::make_unique<FireGoblin>());
            battle.addEnemy(std::make_unique<FrozenGoblin>());
            break;
        case 4:
            battle.addEnemy(std::make_unique<Goblin>());
            battle.addEnemy(std::make_unique<Goblin>());
            battle.addEnemy(std::make_unique<Caster>());
            break;
        case 5:
            battle.addEnemy(std::make_unique<TemplateKing>());
            break;
        case 6:
            battle.addEnemy(std::make_unique<FireGoblin>());
            battle.addEnemy(std::make_unique<Caster>());
            battle.addEnemy(std::make_unique<Goblin>());
            break;
        case 7:
            battle.addEnemy(std::make_unique<Caster>());
            battle.addEnemy(std::make_unique<Caster>());
            battle.addEnemy(std::make_unique<FrozenGoblin>());
            break;
        case 8:
            battle.addEnemy(std::make_unique<ExceptionLord>());
            break;
        default:
            // 循环复用关卡 1
            battle.addEnemy(std::make_unique<Goblin>());
            break;
    }
}

// ============================================================
// 牌组初始化（基础牌组）
// ============================================================

void GameManager::initDeck() {
    // 每种基础卡牌各放几张
    for (int i = 0; i < 3; ++i) drawPile.push_back(std::make_unique<PowerStrikeCard>());
    for (int i = 0; i < 3; ++i) drawPile.push_back(std::make_unique<DefendCard>());
    for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<AttackEnhanceCard>());
    for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<HealCard>());
    for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<StrengthCard>());
    drawPile.push_back(std::make_unique<SummonCard>());

    // 洗牌
    std::random_shuffle(drawPile.begin(), drawPile.end());
}

// ============================================================
// 回合流程（代码执行模式）
// ============================================================

void GameManager::beginTurnWithoutDraw() {
    turnNumber++;
    player.resetActionLimits();
    growMaxEnergy();
    restoreEnergy();
    if (battle.onTurnStart) battle.onTurnStart(turnNumber);
}

void GameManager::prepareTurnCodeBlock() {
    pendingCommands.clear();
    prepareAttackCodeBlock();
    prepareEndCodeBlock();
}

void GameManager::prepareAttackCodeBlock() {
    // 仆从逐个攻击（倒序）
    for (int i = static_cast<int>(player.minions.size()) - 1; i >= 0; --i) {
        auto& m = player.minions[i];
        if (!m.isAlive() || m.isDisabled()) continue;

        Minion* raw = &m;
        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(raw->name);
        cmd.source = CommandSource::MINION;
        cmd.lines = {QString("target = enemy[random()];"),
                     QString("target.takeDamage(%1, PHYSICAL);").arg(raw->getEffectiveAttack())};
        cmd.effect = [this, raw]() {
            if (!raw->isAlive()) return;
            Enemy* t = battle.getRandomEnemy();
            if (t) t->takeDamage(raw->getEffectiveAttack(), DamageType::PHYSICAL);
        };
        pendingCommands.push_back(std::move(cmd));
    }

    // 敌人逐个行动（倒序）
    for (int i = static_cast<int>(battle.enemies.size()) - 1; i >= 0; --i) {
        auto& e = battle.enemies[i];
        if (!e->isAlive() || e->isDisabled()) continue;

        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(e->name);
        cmd.lines = buildEnemyCodeLines(e.get());
        cmd.source = CommandSource::ENEMY;
        cmd.effect = [this, raw = e.get()]() {
            if (!raw->isAlive()) return;
            raw->takeTurn(player);
        };
        pendingCommands.push_back(std::move(cmd));
    }
}

void GameManager::prepareEndCodeBlock() {
    // 玩家状态结算
    {
        PendingCodeCommand cmd;
        cmd.title = QStringLiteral("玩家状态结算");
        cmd.lines = {"player.tickStatuses();  // 灼烧/中毒/再生"};
        cmd.source = CommandSource::END;
        cmd.effect = [this]() { player.tickStatuses(); };
        pendingCommands.push_back(std::move(cmd));
    }
    // 仆从状态结算（倒序）
    for (int i = static_cast<int>(player.minions.size()) - 1; i >= 0; --i) {
        auto& m = player.minions[i];
        if (!m.isAlive()) continue;
        Minion* raw = &m;
        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(raw->name);
        cmd.lines = {QString("%1.tickStatuses();").arg(cmd.title)};
        cmd.source = CommandSource::MINION;
        cmd.effect = [raw]() { if (raw->isAlive()) raw->tickStatuses(); };
        pendingCommands.push_back(std::move(cmd));
    }
    // 敌人状态结算（倒序）
    for (int i = static_cast<int>(battle.enemies.size()) - 1; i >= 0; --i) {
        auto& e = battle.enemies[i];
        if (!e->isAlive()) continue;
        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(e->name);
        cmd.lines = {QString("%1.tickStatuses();").arg(cmd.title)};
        cmd.source = CommandSource::ENEMY;
        cmd.effect = [raw = e.get()]() { if (raw->isAlive()) raw->tickStatuses(); };
        pendingCommands.push_back(std::move(cmd));
    }
}

TurnResult GameManager::finishTurnAfterCodeExecution() {
    if (isBattleOver()) { if (onGameEnd) onGameEnd(); return {true, isPlayerWin()}; }

    if (battle.onTurnEnd) battle.onTurnEnd(turnNumber);

    if (battle.onTurnEnd) battle.onTurnEnd(turnNumber);

    return {false, false};
}

// ============================================================
// 能量管理
// ============================================================

void GameManager::growMaxEnergy() {
    player.maxEnergy = DEFAULT_MAX_ENERGY + turnNumber - 1;
}

void GameManager::restoreEnergy() {
    int oldEnergy = player.energy;
    player.energy = player.maxEnergy;
    if (player.onEnergyChanged)
        player.onEnergyChanged(player.energy, player.maxEnergy,
                               player.energy - oldEnergy);
}

void GameManager::spendEnergy(int cost) {
    int oldEnergy = player.energy;
    player.energy -= cost;
    if (player.onEnergyChanged)
        player.onEnergyChanged(player.energy, player.maxEnergy,
                               player.energy - oldEnergy);
}

// ============================================================
// 牌组操作
// ============================================================

DrawResult GameManager::drawOneCard() {
    if (hand.size() >= MAX_HAND_SIZE)
        return {false, false, -1, {}};

    // 牌堆空但弃牌堆有牌 → 通知 Qt 先播洗牌动画
    if (drawPile.empty() && !discardPile.empty())
        return {false, true, -1, {}};

    // 牌堆空且弃牌堆也空 → 没牌可抽
    if (drawPile.empty())
        return {false, false, -1, {}};

    // 随机抽一张
    int idx = rand() % drawPile.size();
    std::unique_ptr<Card> card = std::move(drawPile[idx]);
    drawPile.erase(drawPile.begin() + idx);

    FunctionCard* fc = dynamic_cast<FunctionCard*>(card.get());
    CardView view{QString::fromStdString(card->name),
                  QString::fromStdString(card->description),
                  card->cost, card->type,
                  fc ? fc->target : FunctionTarget::ATTACK,
                  card->targetMode};
    int handIndex = static_cast<int>(hand.size());
    hand.push_back(std::move(card));

    return {true, false, handIndex, view};
}

void GameManager::recycleDiscardToDrawPile() {
    for (auto& card : discardPile)
        drawPile.push_back(std::move(card));
    discardPile.clear();

    // 洗牌
    std::random_shuffle(drawPile.begin(), drawPile.end());
}

PlayResult GameManager::playCardAsCode(int handIndex, Enemy* target) {
    if (handIndex < 0 || handIndex >= static_cast<int>(hand.size()))
        return {false, handIndex, {}, QStringLiteral("手牌不存在")};

    Card* card = hand[handIndex].get();
    if (!card) return {false, handIndex, {}, QStringLiteral("手牌不存在")};

    if (!card->canPlay(player))
        return {false, handIndex, {}, QStringLiteral("当前无法打出此牌")};

    if (player.energy < card->cost)
        return {false, handIndex, {}, QStringLiteral("能量不足")};

    spendEnergy(card->cost);

    FunctionCard* fc = dynamic_cast<FunctionCard*>(card);
    CardView view{QString::fromStdString(card->name),
                  QString::fromStdString(card->description),
                  card->cost, card->type,
                  fc ? fc->target : FunctionTarget::ATTACK,
                  card->targetMode};

    // 不立即执行，而是挂入代码队列
    Card* rawCard = card;
    Enemy* rawTarget = target;

    PendingCodeCommand cmd;
    cmd.title = view.name;
    cmd.lines = buildCardCodeLines(view);
    cmd.effect = [this, rawCard, rawTarget]() { rawCard->play(player, rawTarget); };

    insertPlayerCommandBeforeEnemy(std::move(cmd));

    discardPile.push_back(std::move(hand[handIndex]));
    hand.erase(hand.begin() + handIndex);

    return {true, handIndex, view, ""};
}

void GameManager::discardHand() {
    for (auto& card : hand)
        discardPile.push_back(std::move(card));
    hand.clear();
}

// ============================================================
// 查询（Qt 只读）
// ============================================================

QVector<CardView> GameManager::getHandView() const {
    QVector<CardView> result;
    for (auto& card : hand) {
        if (card) {
            FunctionCard* fc = dynamic_cast<FunctionCard*>(card.get());
            result.push_back({QString::fromStdString(card->name),
                              QString::fromStdString(card->description),
                              card->cost, card->type,
                              fc ? fc->target : FunctionTarget::ATTACK,
                              card->targetMode});
        } else {
            result.push_back({});
        }
    }
    return result;
}

// ============================================================
// 代码执行模式
// ============================================================

QVector<CodeCommandView> GameManager::getCodeCommandViews() const {
    QVector<CodeCommandView> result;
    for (auto& cmd : pendingCommands)
        result.push_back({cmd.title, cmd.lines, cmd.executed});
    return result;
}

int GameManager::pendingCodeCount() const {
    return static_cast<int>(pendingCommands.size());
}

void GameManager::executePendingCode(int index) {
    if (index < 0 || index >= pendingCommands.size()) return;
    auto& cmd = pendingCommands[index];
    if (cmd.executed) return;
    if (cmd.effect) cmd.effect();
    cmd.executed = true;
}

void GameManager::insertPlayerCommandBeforeEnemy(PendingCodeCommand cmd) {
    // 插入到最后一个玩家操作之后、第一个仆从/敌人操作之前
    int insertPos = 0;
    for (int i = static_cast<int>(pendingCommands.size()) - 1; i >= 0; --i) {
        if (pendingCommands[i].source == CommandSource::PLAYER) {
            insertPos = i + 1;
            break;
        }
    }
    pendingCommands.insert(insertPos, std::move(cmd));
}

QStringList GameManager::buildCardCodeLines(const CardView& card) const {
    if (card.type == CardType::FUNCTION) {
        // 函数牌：显示修改哪个函数的代码
        switch (card.funcTarget) {
            case FunctionTarget::ATTACK:
                return {QString("player.setAttackFunc(...);  // %1").arg(card.name)};
            case FunctionTarget::TAKE_DAMAGE:
                return {QString("player.setTakeDamageFunc(...);  // %1").arg(card.name)};
            case FunctionTarget::SUMMON:
                return {QString("player.setSummonFunc(...);  // %1").arg(card.name)};
            case FunctionTarget::COPY_SUMMON:
                return {QString("player.setCopySummonFunc(...);  // %1").arg(card.name)};
            case FunctionTarget::MOVE_SUMMON:
                return {QString("player.setMoveSummonFunc(...);  // %1").arg(card.name)};
            case FunctionTarget::SACRIFICE:
                return {QString("player.setSacrificeFunc(...);  // %1").arg(card.name)};
            case FunctionTarget::ESCAPE:
                return {QString("player.setEscapeFunc(...);  // %1").arg(card.name)};
        }
    }
    if (card.type == CardType::TEMPLATE)
        return {QString("template.apply(player, enemy);  // %1").arg(card.name)};
    // 指令牌
    return {QString("card.play(player, enemy);  // %1").arg(card.name)};
}

QStringList GameManager::buildEnemyCodeLines(Enemy* enemy) const {
    if (!enemy)
        return {"// enemy_action", "enemy.wait();"};
    return {QString("// enemy_action"),
            QString("%1.takeTurn(player);").arg(QString::fromStdString(enemy->name))};
}

// ============================================================
// 战斗收尾
// ============================================================

void GameManager::finishBattle(bool playerWin) {
    if (onGameEnd) onGameEnd();
}
