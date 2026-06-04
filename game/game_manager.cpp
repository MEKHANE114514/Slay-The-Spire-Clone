#include "game_manager.h"
#include <cstdlib>   // rand, srand
#include <ctime>     // time
#include <algorithm> // shuffle, remove_if

GameManager::GameManager()
    : battle(player)
{
    srand(static_cast<unsigned>(time(nullptr)));
}

// ============================================================
// 回合流程
// ============================================================

void GameManager::startTurn() {
    turnNumber++;
    player.resetActionLimits();
    growMaxEnergy();
    restoreEnergy();
    drawCards(DEFAULT_DRAW_PER_TURN);
    if (battle.onTurnStart) battle.onTurnStart(turnNumber);
}

TurnResult GameManager::endTurn() {
    discardHand();

    battle.executeAttackPhase();
    if (isBattleOver()) { finishBattle(isPlayerWin()); return {true, isPlayerWin()}; }

    battle.executeEndPhase();
    if (isBattleOver()) { finishBattle(isPlayerWin()); return {true, isPlayerWin()}; }

    if (battle.onTurnEnd) battle.onTurnEnd(turnNumber);

    startTurn();
    return {false, false};
}

void GameManager::drawCards(int count) {
    for (int i = 0; i < count; ++i)
        drawOneCard();  // 回调在 drawOneCard 内部触发
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

    CardView view{QString::fromStdString(card->name),
                  QString::fromStdString(card->description),
                  card->cost, card->targetMode};
    int handIndex = static_cast<int>(hand.size());
    hand.push_back(std::move(card));

    if (onCardDrawn) onCardDrawn(handIndex, view);
    return {true, false, handIndex, view};
}

void GameManager::recycleDiscardToDrawPile() {
    for (auto& card : discardPile)
        drawPile.push_back(std::move(card));
    discardPile.clear();

    // 洗牌
    std::random_shuffle(drawPile.begin(), drawPile.end());

    if (onCardsRecycled) onCardsRecycled();
}

PlayResult GameManager::playCard(int handIndex, Enemy* target) {
    if (handIndex < 0 || handIndex >= static_cast<int>(hand.size()))
        return {false, handIndex, {}, QStringLiteral("手牌不存在")};

    Card* card = hand[handIndex].get();
    if (!card) return {false, handIndex, {}, QStringLiteral("手牌不存在")};

    if (!card->canPlay(player))
        return {false, handIndex, {}, QStringLiteral("当前无法打出此牌")};

    if (player.energy < card->cost)
        return {false, handIndex, {}, QStringLiteral("能量不足")};

    // 扣能量 + 执行卡牌
    spendEnergy(card->cost);
    card->play(player, target);

    // 移到弃牌堆
    CardView view{QString::fromStdString(card->name),
                  QString::fromStdString(card->description),
                  card->cost, card->targetMode};
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
        if (card)
            result.push_back({QString::fromStdString(card->name),
                              QString::fromStdString(card->description),
                              card->cost, card->targetMode});
        else
            result.push_back({});
    }
    return result;
}

QString GameManager::getEnemyIntentText() const {
    QString text;
    for (auto& e : battle.enemies) {
        if (!text.isEmpty()) text += "\n";
        text += QString::fromStdString(e->name + "：" + e->nextIntent.name());
        if (e->nextIntent.value > 0)
            text += " " + QString::number(e->nextIntent.value);
    }
    return text.isEmpty() ? QStringLiteral("无敌人") : text;
}

// ============================================================
// 战斗
// ============================================================

void GameManager::startBattle() {
    if (onGameStart) onGameStart();
    startTurn();
}

void GameManager::finishBattle(bool playerWin) {
    if (onGameEnd) onGameEnd();
}
