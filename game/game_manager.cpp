#include "game_manager.h"
#include <cstdlib>   // rand, srand
#include <ctime>     // time

GameManager::GameManager()
    : battle(player)
{
    srand(static_cast<unsigned>(time(nullptr)));  // 初始化随机种子
}

// ============================================================
// 回合流程
// ============================================================

void GameManager::startTurn() {
    turnNumber++;
    player.resetActionLimits();
    growMaxEnergy();                   // 随回合增大能量上限
    restoreEnergy();
    if (battle.onTurnStart) battle.onTurnStart(turnNumber);
}

void GameManager::endTurn() {
    // ATTACK 阶段：仆从攻击 + 敌人行动
    battle.executeAttackPhase();
    if (isBattleOver()) { finishBattle(); return; }

    // END 阶段：全场状态结算
    battle.executeEndPhase();
    if (isBattleOver()) { finishBattle(); return; }

    if (battle.onTurnEnd) battle.onTurnEnd(turnNumber);

    // 下一回合
    startTurn();
}

// ============================================================
// 能量管理
// ============================================================

void GameManager::growMaxEnergy() {
    // 起始 = DEFAULT_MAX_ENERGY，之后每回合 +1
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
// 战斗
// ============================================================

void GameManager::startBattle() {
    // 以后在此创建具体怪物并调用 battle.addEnemy(...)
    if (onGameStart) onGameStart();
    startTurn();
}

// ============================================================
// 内部
// ============================================================

void GameManager::finishBattle() {
    if (onGameEnd) onGameEnd();
}
