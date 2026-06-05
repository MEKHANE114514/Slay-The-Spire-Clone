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

    // ② 包装 onDeath：先 Qt 动画 → 再从 vector 移除
    auto oldOnDeath = std::move(raw->onDeath);
    raw->onDeath = [this, raw, oldDeath = std::move(oldOnDeath)]() {
        if (oldDeath) oldDeath();           // Qt 死亡动画
        auto it = std::find_if(enemies.begin(), enemies.end(),
            [raw](auto& ep) { return ep.get() == raw; });
        if (it != enemies.end()) enemies.erase(it);
    };
}

// ============================================================
// 查询
// ============================================================

Enemy* BattleContext::getRandomEnemy() const {
    if (enemies.empty()) return nullptr;
    return enemies[rand() % enemies.size()].get();
}
