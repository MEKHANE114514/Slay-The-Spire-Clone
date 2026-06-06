#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <functional>
#include <vector>
#include <memory>
#include <QString>
#include <QVector>
#include <QStringList>
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

struct CodeCommandView {
    QString title;
    QStringList lines;
    bool executed = false;
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
    void beginTurnWithoutDraw();
    void prepareTurnCodeBlock();
    void prepareAttackCodeBlock();
    void prepareEndCodeBlock();
    std::vector<std::string> getPlayerCodeLines() const;
    std::vector<std::string> getEnemyCodeLines(Enemy* enemy) const;
    std::vector<std::string> getEnemyDescription() const;
    TurnResult finishTurnAfterCodeExecution();
    bool isBattleOver() const { return battle.isBattleOver(); }
    bool isPlayerWin() const { return battle.allEnemiesDead(); }

    // ---- 能量管理 ----
    void growMaxEnergy();
    void restoreEnergy();
    void spendEnergy(int cost);

    // ---- 牌组操作 ----
    DrawResult drawOneCard();
    void recycleDiscardToDrawPile();
    PlayResult playCardAsCode(int handIndex, Enemy* target = nullptr);
    void discardHand();

    // ---- 代码执行模式 ----
    QVector<CodeCommandView> getCodeCommandViews() const;
    int pendingCodeCount() const;
    void executePendingCode(int index);

    // ---- 查询（Qt 只读）----
    QVector<CardView> getHandView() const;
    int getDrawPileCount() const { return static_cast<int>(drawPile.size()); }
    int getDiscardPileCount() const { return static_cast<int>(discardPile.size()); }

    // ---- Qt 回调 ----
    std::function<void()> onGameEnd;

private:
    void initLevel();
    void initDeck();

    // ---- 代码执行模式内部 ----
    enum class CommandSource { PLAYER, MINION, ENEMY, END };

    struct PendingCodeCommand {
        QString title;
        QStringList lines;
        std::function<void()> effect;
        bool executed = false;
        CommandSource source = CommandSource::PLAYER;
    };
    QVector<PendingCodeCommand> pendingCommands;

    QStringList buildEnemyCodeLines(Enemy* enemy) const;
    void insertPlayerCommandBeforeEnemy(PendingCodeCommand cmd);
};

#endif // GAME_MANAGER_H

