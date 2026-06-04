#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <functional>
#include <vector>
#include <memory>
#include <string>
#include "player.h"
#include "battle.h"
#include "cards.h"

// ============================================================
// game_manager.h — 游戏总管理器
// 回合流程、能量管理、牌组管理
// ============================================================

// ---- Qt 桥梁结构体（纯 C++，Qt 端用 QString::fromStdString 转换）----
struct CardView {
    std::string name;
    std::string description;
    int cost = 0;
    TargetMode targetMode = TargetMode::NONE;
};

struct DrawResult {
    bool success = false;
    bool needRecycle = false;
    int handIndex = -1;
    CardView card;
};

struct PlayResult {
    bool success = false;
    int handIndex = -1;
    CardView card;
    std::string failReason;
};

struct TurnResult {
    bool gameOver = false;
    bool playerWin = false;
};

class GameManager {
public:
    Player player;
    BattleContext battle;
    int turnNumber = 0;

    // ---- 牌组 ----
    std::vector<std::unique_ptr<Card>> drawPile;
    std::vector<std::unique_ptr<Card>> hand;
    std::vector<std::unique_ptr<Card>> discardPile;

    GameManager();

    // ---- 回合流程 ----
    void startTurn();
    TurnResult endTurn();
    bool isBattleOver() const { return battle.isBattleOver(); }
    bool isPlayerWin() const { return battle.allEnemiesDead(); }

    // ---- 能量管理 ----
    void growMaxEnergy();
    void restoreEnergy();
    void spendEnergy(int cost);

    // ---- 牌组操作 ----
    void drawCards(int count);            // 批量抽牌（内部调 drawOneCard）
    DrawResult drawOneCard();
    void recycleDiscardToDrawPile();
    PlayResult playCard(int handIndex, Enemy* target = nullptr);
    void discardHand();

    // ---- 查询（Qt 只读）----
    std::vector<CardView> getHandView() const;
    int getDrawPileCount() const { return static_cast<int>(drawPile.size()); }
    int getDiscardPileCount() const { return static_cast<int>(discardPile.size()); }
    std::string getEnemyIntentText() const;

    // ---- 战斗开始（以后填入具体怪物）----
    void startBattle();

    // ---- Qt 回调 ----
    std::function<void(int handIndex, CardView card)> onCardDrawn;   // 单张抽牌动画
    std::function<void()> onCardsRecycled;  // 弃牌洗回动画
    std::function<void()> onGameStart;
    std::function<void()> onGameEnd;

private:
    void finishBattle(bool playerWin);
};

#endif // GAME_MANAGER_H
