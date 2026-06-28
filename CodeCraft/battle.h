#ifndef BATTLE_H
#define BATTLE_H

#include <vector>
#include <memory>
#include <functional>
#include "player.h"
#include "enemy.h"
 
// ============================================================
// battle.h — 战斗上下文
// 负责将 Player、Minion、Enemy 串联执行回合阶段
// enemies 包含所有敌人，死亡的敌人在回合结束时统一清理
// ============================================================

class BattleContext {
public:
    Player& player;
    std::vector<std::unique_ptr<Enemy>> enemies;  // 包含所有敌人（含死亡但未清理的）
    int turnNumber = 0;

    BattleContext(Player& p) : player(p) {}

    // ---- 敌人管理 ----
    // 添加敌人并绑定其 onDeath 回调（死亡时标记为待清理）
    void addEnemy(std::unique_ptr<Enemy> e);

    // 清理本回合死亡的敌人（在回合结束时调用）
    void cleanupDeadEnemies();

    // ---- 查询 ----
    Enemy* getRandomEnemy() const;        // 从存活敌人中随机选一个
    bool allEnemiesDead() const;
    bool isBattleOver() const {
        return !player.isAlive() || allEnemiesDead();
    }

    // ---- UI 回调（Qt 绑定）----
    std::function<void(Enemy*)>      onEnemyAdded;
    std::function<void(int turn)>    onTurnStart;
    std::function<void(int turn)>    onTurnEnd;
};

#endif // BATTLE_H
