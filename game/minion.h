#ifndef MINION_H
#define MINION_H

#include <string>
#include <vector>
#include "types.h"

// ============================================================
// minion.h — 仆从类 & Status 状态结构体
// ============================================================

// 状态效果实例（通用：Player 和 Minion 共用）
struct Status {
    StatusType type;
    int turnsRemaining;  // 剩余回合数，-1 表示永久
    int value;           // 强度值（灼烧伤害、力量加成……）
};

class Minion {
public:
    std::string name;
    int hp, maxHp;
    int attack;
    int shield;
    MinionType type = MinionType::NORMAL;
    std::vector<Status> statuses;
    EntityState state = EntityState::NORMAL;

    Minion() = default;
    Minion(std::string n, int h, int a, MinionType t = MinionType::NORMAL);

    bool isAlive() const { return hp > 0; }

    void takeDamage(int dmg, DamageType dtype = DamageType::PHYSICAL);
    void heal(int amount);
    void addStatus(Status s);
    bool hasStatus(StatusType t) const;
    void tickStatuses();  // 回合结束时结算状态效果

    // 考虑状态加成后的实际攻击力
    int getEffectiveAttack() const;
};

#endif // MINION_H
