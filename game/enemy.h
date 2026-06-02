#ifndef ENEMY_H
#define ENEMY_H

#include <string>
#include <vector>
#include "types.h"
#include "minion.h"

// ============================================================
// enemy.h — 敌人类 + 意图系统
// 使用策略模式：takeTurn() 纯虚函数，子类实现不同 AI
// ============================================================

class Player;  // 前向声明，takeTurn() 需要

// 意图：显示敌人下回合要做什么（类似杀戮尖塔的意图图标）
struct EnemyIntent {
    enum Type { ATTACK, DEFEND, BUFF, SUMMON, NONE };
    Type type = NONE;
    int value = 0;  // 攻击时为伤害值，防御时为护盾值

    std::string name() const {
        switch (type) {
            case ATTACK:  return "攻击";
            case DEFEND:  return "防御";
            case BUFF:    return "强化";
            case SUMMON:  return "召唤";
            default:      return "等待";
        }
    }
};

class Enemy {
public:
    std::string name;
    int hp, maxHp;
    int shield = 0;
    int baseAttack;
    Faction faction = Faction::NONE;
    EntityState state = EntityState::NORMAL;
    std::vector<Status> statuses;
    EnemyIntent nextIntent;  // 下回合意图（供 UI 显示）

    Enemy(std::string n, int h, int atk)
        : name(std::move(n)), hp(h), maxHp(h), baseAttack(atk) {}
    virtual ~Enemy() = default;

    // ---- 生存 ----
    bool isAlive() const { return hp > 0; }
    void takeDamage(int dmg, DamageType type = DamageType::PHYSICAL);
    void heal(int amount);
    int getEffectiveAttack() const;

    // ---- 状态 ----
    void addStatus(Status s);
    bool hasStatus(StatusType t) const;
    void tickStatuses();
    bool isDisabled() const {
        return state == EntityState::STUNNED || state == EntityState::FROZEN;
    }

    // ---- AI（子类必须重写）----
    virtual void takeTurn(Player& player) = 0;

protected:
    // 子类在 takeTurn() 中调用，设置本回合意图
    void setIntent(EnemyIntent::Type t, int val = 0) {
        nextIntent = {t, val};
    }
};

// ============================================================
// 示例子类
// ============================================================

// 普通小怪：每回合平砍
class Goblin : public Enemy {
public:
    Goblin() : Enemy("哥布林", 40, 8) {}

    void takeTurn(Player& player) override {
        if (isDisabled()) { setIntent(EnemyIntent::NONE); return; }
        setIntent(EnemyIntent::ATTACK, getEffectiveAttack());
        player.takeDamage(getEffectiveAttack(), DamageType::PHYSICAL);
    }
};

// 蓄力怪：奇数回合蓄力，偶数回合爆发
class Charger : public Enemy {
    bool charged = false;
public:
    Charger() : Enemy("蓄力兽", 60, 6) {}

    void takeTurn(Player& player) override {
        if (isDisabled()) { setIntent(EnemyIntent::NONE); return; }
        if (charged) {
            int dmg = getEffectiveAttack() * 3;
            setIntent(EnemyIntent::ATTACK, dmg);
            player.takeDamage(dmg, DamageType::PHYSICAL);
            charged = false;
        } else {
            setIntent(EnemyIntent::BUFF, 0);
            charged = true;
        }
    }
};

// 防御兵：攻击和防御交替
class Defender : public Enemy {
    int turnCount = 0;
public:
    Defender() : Enemy("盾卫", 50, 7) {}

    void takeTurn(Player& player) override {
        if (isDisabled()) { setIntent(EnemyIntent::NONE); return; }
        if (turnCount % 2 == 0) {
            setIntent(EnemyIntent::ATTACK, getEffectiveAttack());
            player.takeDamage(getEffectiveAttack(), DamageType::PHYSICAL);
        } else {
            setIntent(EnemyIntent::DEFEND, 10);
            shield += 10;
        }
        turnCount++;
    }
};

#endif // ENEMY_H
