#include "minion.h"
#include <algorithm>  // std::min, std::remove_if, std::any_of

// ============================================================
// 构造
// ============================================================

Minion::Minion(std::string n, int h, int a, MinionType t)
    : name(std::move(n)), hp(h), maxHp(h), attack(a), type(t) {}

// ============================================================
// 生存
// ============================================================

void Minion::takeDamage(int dmg, DamageType /*dtype*/) {
    int oldHp = hp;
    // 先扣护盾
    if (shield > 0) {
        int blocked = std::min(shield, dmg);
        shield -= blocked;
        dmg -= blocked;
    }
    hp -= dmg;
    // 回调
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp);
    if (!isAlive() && onDeath)       onDeath();
}

void Minion::heal(int amount) {
    int oldHp = hp;
    hp = std::min(hp + amount, maxHp);
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp);
}

// ============================================================
// 状态管理
// ============================================================

void Minion::addStatus(Status s) {
    statuses.push_back(s);
    if (onStatusAdded) onStatusAdded(s);

    // 冻结/眩晕 → 直接改状态
    if (s.type == StatusType::FREEZE || s.type == StatusType::STUN) {
        state = (s.type == StatusType::FREEZE) ? EntityState::FROZEN
                                               : EntityState::STUNNED;
    }
}

bool Minion::hasStatus(StatusType t) const {
    return std::any_of(statuses.begin(), statuses.end(),
        [t](const Status& s) { return s.type == t; });
}

void Minion::tickStatuses() {
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
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp);
    if (!isAlive() && onDeath)       onDeath();
}

// ============================================================
// 实际攻击力（考虑状态加成）
// ============================================================

int Minion::getEffectiveAttack() const {
    int atk = attack;
    for (auto& s : statuses) {
        if (s.type == StatusType::STRENGTH) atk += s.value;
        if (s.type == StatusType::WEAKEN)   atk -= s.value;
    }
    return atk > 0 ? atk : 0;  // 攻击力不低于 0
}
