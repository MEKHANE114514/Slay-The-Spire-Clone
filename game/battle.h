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
// enemies 永远只包含 hp > 0的敌人（死亡立即移除）
// ============================================================

class BattleContext {
public:
    Player& player;
    std::vector<std::unique_ptr<Enemy>> enemies;  // 永远只存活的
    int turnNumber = 0;

    BattleContext(Player& p) : player(p) {}

    // ---- 敌人管理 ----
    // 添加敌人并绑定其 onDeath 回调（死亡时自动从 enemies 移除）
    void addEnemy(std::unique_ptr<Enemy> e);

    // ---- 查询 ----
    Enemy* getRandomEnemy() const;        // 从存活敌人中随机选一个
    bool allEnemiesDead() const { return enemies.empty(); }
    bool isBattleOver() const {
        return !player.isAlive() || allEnemiesDead();
    }

    // ---- 阶段执行 ----
    void executeAttackPhase();  // 仆从自动攻击 → 敌人依次行动
    void executeEndPhase();     // 全场 tickStatuses

    // ---- UI 回调（Qt 绑定）----
    std::function<void(Enemy*)>      onEnemyAdded;   // 敌人入场 → Qt 播放登场动画
    std::function<void(int turn)>    onTurnStart;    // 回合开始
    std::function<void(int turn)>    onTurnEnd;      // 回合结束
    std::function<void()>            onVictory;      // 胜利
    std::function<void()>            onDefeat;       // 失败
};

#endif // BATTLE_H
