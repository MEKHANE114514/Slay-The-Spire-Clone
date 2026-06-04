#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <functional>
#include <vector>
#include <memory>
#include <QString>
#include <QVector>
#include "player.h"
#include "battle.h"
#include "cards.h"

// ============================================================
// game_manager.h — 游戏总管理器
// 回合流程、能量管理、牌组管理
// ============================================================

// ---- Qt 桥梁结构体 ----
struct CardView {
    QString name;
    QString description;
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
    QString failReason;
};

struct TurnResult {
    bool gameOver = false;
    bool playerWin = false;
};

class GameManager {
public:
    // ---- 关卡系统（静态，跨实例）----
    static int currentLevel;
    static void setLevel(int level) { currentLevel = level; }
    static int getLevel() { return currentLevel; }

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
    QVector<CardView> getHandView() const;
    int getDrawPileCount() const { return static_cast<int>(drawPile.size()); }
    int getDiscardPileCount() const { return static_cast<int>(discardPile.size()); }
    QString getEnemyIntentText() const;

    // ---- 战斗开始（以后填入具体怪物）----
    void startBattle();

    // ---- Qt 回调 ----
    std::function<void(int handIndex, CardView card)> onCardDrawn;   // 单张抽牌动画
    std::function<void()> onCardsRecycled;  // 弃牌洗回动画
    std::function<void()> onGameStart;
    std::function<void()> onGameEnd;

private:
    void initLevel();                      // 根据 currentLevel 生成敌人
    void initDeck();                       // 初始化基础牌组
    void finishBattle(bool playerWin);
};

#endif // GAME_MANAGER_H
