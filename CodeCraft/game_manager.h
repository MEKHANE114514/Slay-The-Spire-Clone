#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <functional>
#include "player.h"
#include "battle.h"

// ============================================================
// game_manager.h — 游戏总管理器
// 负责回合流程、能量管理，目前暂缺牌组管理
// ============================================================

class GameManager {
public:
    Player player;
    BattleContext battle;
    int turnNumber = 0;

    GameManager();

    // ---- 回合流程 ----
    void startTurn();
    void endTurn();
    bool isBattleOver() const { return battle.isBattleOver(); }

    // ---- 能量管理 ----
    void growMaxEnergy();               // 随回合增大能量上限
    void restoreEnergy();
    void spendEnergy(int cost);

    // ---- 战斗开始（以后填入具体怪物）----
    void startBattle();

    // ---- Qt 回调 ----
    std::function<void()> onGameStart;
    std::function<void()> onGameEnd;       // 胜利/失败时触发

private:
    void finishBattle();                   // 战斗结束收尾
};

#endif // GAME_MANAGER_H
