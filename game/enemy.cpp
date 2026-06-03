#include "enemy.h"
#include "player.h"
#include <algorithm>  // std::min, std::remove_if, std::any_of

// ============================================================
// 生存
// ============================================================

void Enemy::takeDamage(int dmg, DamageType /*dtype*/) {
    int oldHp = hp;
    int oldShield = shield;
    // 先扣护盾
    if (shield > 0) {
        int blocked = std::min(shield, dmg);
        shield -= blocked;
        dmg -= blocked;
    }
    hp -= dmg;
    // 回调
    if (onShieldChanged && shield != oldShield) onShieldChanged(shield, shield - oldShield);
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp, hp - oldHp);
    if (!isAlive() && onDeath)       onDeath();
}

void Enemy::heal(int amount) {
    int oldHp = hp;
    hp = std::min(hp + amount, maxHp);
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp, hp - oldHp);
}

int Enemy::getEffectiveAttack() const {
    int atk = baseAttack;
    for (auto& s : statuses) {
        if (s.type == StatusType::STRENGTH) atk += s.value;
        if (s.type == StatusType::WEAKEN)   atk -= s.value;
    }
    return atk > 0 ? atk : 0;
}

// ============================================================
// 状态管理
// ============================================================

void Enemy::addStatus(Status s) {
    statuses.push_back(s);
    if (onStatusAdded) onStatusAdded(s);
}

bool Enemy::hasStatus(StatusType t) const {
    return std::any_of(statuses.begin(), statuses.end(),
        [t](const Status& s) { return s.type == t; });
}

void Enemy::tickStatuses() {
    int oldHp = hp;

    for (auto& s : statuses) {
        switch (s.type) {
            case StatusType::BURN:    hp -= s.value;            break;
            case StatusType::POISON:  hp -= s.value; s.value++; break;
            case StatusType::REGEN:   heal(s.value);            break;
            default: break;
        }
        if (s.turnsRemaining > 0) s.turnsRemaining--;
    }

    // 清除到期状态
    auto it = std::remove_if(statuses.begin(), statuses.end(),
        [this](const Status& s) {
            if (s.turnsRemaining == 0 && s.type != StatusType::SHIELD) {
                if (onStatusRemoved) onStatusRemoved(s.type);
                return true;
            }
            return false;
        });
    statuses.erase(it, statuses.end());

    // 回调
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp, hp - oldHp);
    if (!isAlive() && onDeath)       onDeath();
}
