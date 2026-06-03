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

// ============================================================
// 阶段执行
// ============================================================

void BattleContext::executeAttackPhase() {
    // ① 仆从自动攻击（倒序遍历，防止中途死亡导致索引错乱）
    for (int i = static_cast<int>(player.minions.size()) - 1; i >= 0; --i) {
        if (i >= static_cast<int>(player.minions.size())) continue;
        auto& m = player.minions[i];
        if (m.isDisabled()) continue;
        Enemy* target = getRandomEnemy();
        if (target) target->takeDamage(m.getEffectiveAttack(), DamageType::PHYSICAL);
    }

    // ② 敌人依次行动（倒序遍历）
    for (int i = static_cast<int>(enemies.size()) - 1; i >= 0; --i) {
        if (i >= static_cast<int>(enemies.size())) continue;
        auto& e = enemies[i];
        if (e->isDisabled()) continue;
        e->takeTurn(player);
    }

    // ③ 战斗结果检测
    if (isBattleOver()) {
        if (allEnemiesDead()) { if (onVictory) onVictory(); }
        else                  { if (onDefeat)  onDefeat();  }
    }
}

void BattleContext::executeEndPhase() {
    // ① Player 状态结算
    player.tickStatuses();
    if (isBattleOver()) {
        if (onDefeat) onDefeat();
        return;  // 玩家死了，不需要继续
    }

    // ② 仆从状态结算（倒序）
    for (int i = static_cast<int>(player.minions.size()) - 1; i >= 0; --i) {
        if (i >= static_cast<int>(player.minions.size())) continue;
        player.minions[i].tickStatuses();
    }

    // ③ 敌人状态结算（倒序）
    for (int i = static_cast<int>(enemies.size()) - 1; i >= 0; --i) {
        if (i >= static_cast<int>(enemies.size())) continue;
        enemies[i]->tickStatuses();
    }

    // ④ 战斗结果检测
    if (isBattleOver()) {
        if (allEnemiesDead()) { if (onVictory) onVictory(); }
        else                  { if (onDefeat)  onDefeat();  }
    }
}
