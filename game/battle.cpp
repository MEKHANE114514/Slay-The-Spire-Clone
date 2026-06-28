#include "battle.h"
#include <cstdlib>   // rand
#include <algorithm> // find_if

// ============================================================
// 敌人管理
// ============================================================

void BattleContext::addEnemy(std::unique_ptr<Enemy> e) {
    Enemy* raw = e.get();
    enemies.push_back(std::move(e));

    // ① 先让 Qt 绑定自己的 onDeath（动画）
    if (onEnemyAdded) onEnemyAdded(raw);

    // ② 包装 onDeath：只播放动画，不立即移除
    // 敌人会在 cleanupDeadEnemies() 中统一清理
    auto oldOnDeath = std::move(raw->onDeath);
    raw->onDeath = [oldDeath = std::move(oldOnDeath)]() {
        if (oldDeath) oldDeath();  // Qt 死亡动画
        // 不再立即 erase，等待回合结束时统一清理
    };
}

void BattleContext::cleanupDeadEnemies() {
    // 移除所有已死亡的敌人
    auto it = std::remove_if(enemies.begin(), enemies.end(),
        [](const auto& ep) { return !ep->isAlive(); });
    enemies.erase(it, enemies.end());
}

// ============================================================
// 查询
// ============================================================

bool BattleContext::allEnemiesDead() const {
    return std::all_of(enemies.begin(), enemies.end(),
        [](const auto& ep) { return !ep->isAlive(); });
}

Enemy* BattleContext::getRandomEnemy() const {
    // 只从存活的敌人中随机选择
    std::vector<Enemy*> alive;
    for (const auto& e : enemies) {
        if (e && e->isAlive()) {
            alive.push_back(e.get());
        }
    }
    if (alive.empty()) return nullptr;
    return alive[rand() % alive.size()];
}
