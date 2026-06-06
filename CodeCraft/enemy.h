#ifndef ENEMY_H
#define ENEMY_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "types.h"
#include "minion.h"

// ============================================================
// enemy.h — 敌人类 + 意图系统
// 使用策略模式：takeTurn() 纯虚函数，子类实现不同 AI
// ============================================================

class Player;  // 前向声明，takeTurn() 需要
class BattleContext;  // 前向声明，Caster 需要

// 意图：显示敌人下回合要做什么（类似杀戮尖塔的意图图标）
struct EnemyIntent {
    enum Type { ATTACK, DEFEND, HEAL, BUFF, SUMMON, NONE };
    Type type = NONE;
    int value = 0;  // 攻击时为伤害值，防御时为护盾值

    std::string name() const {
        switch (type) {
            case ATTACK:  return "攻击";
            case DEFEND:  return "防御";
            case HEAL:    return "回复";
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
    std::vector<Status> statuses;
    EnemyIntent nextIntent;  // 下回合意图（供 UI 显示）

    Enemy(std::string n, int h, int atk)
        : name(std::move(n)), hp(h), maxHp(h), baseAttack(atk) {}
    virtual ~Enemy() = default;

    // ---- 生存 ----
    virtual bool isAlive() const { return hp > 0; }
    virtual void takeDamage(int dmg, DamageType type = DamageType::PHYSICAL);
    void heal(int amount);
    int getEffectiveAttack() const;

    // ---- 状态 ----
    virtual void addStatus(Status s);
    bool hasStatus(StatusType t) const;
    void tickStatuses();
    bool isDisabled() const {
        return hasStatus(StatusType::FREEZE) || hasStatus(StatusType::STUN);
    }

    // ---- AI（子类必须重写）----
    virtual void takeTurn(Player& player) = 0;

    // ---- UI 回调（Qt 绑定）----
    std::function<void(int hp, int maxHp, int delta)>   onHpChanged;      // 生命变化 → 血条动画
    std::function<void(int shield, int delta)> onShieldChanged;  // 护盾变化
    std::function<void(const Status&)>       onStatusAdded;    // 新增 Buff/Debuff
    std::function<void(StatusType)>          onStatusRemoved;  // Buff/Debuff 消失
    std::function<void()>                    onDeath;          // 死亡 → 消失动画
    std::function<void(const EnemyIntent&)>  onIntentChanged;  // 意图切换 → UI 更新图标
    
    // ---- 敌人描述 ----
    virtual std::vector<std::string> getDescription() const { return {"None"}; }

    // ---- 状态描述 ----
    std::vector<std::string> getStatusesCode() const;

protected:
    // 子类在 takeTurn() 中调用，设置本回合意图
    void setIntent(EnemyIntent::Type t, int val = 0) {
        nextIntent.type = t;
        nextIntent.value = val;
        if (onIntentChanged) onIntentChanged(nextIntent);
    }
};

// ============================================================
// 示例敌人：程序猿 — 每回合只会攻击
// ============================================================

class Goblin : public Enemy {
public:
    Goblin() : Enemy("程序猿", 30, 6) {}
    void takeTurn(Player& player) override;
    std::vector<std::string> getDescription() const override;
};

class FireGoblin : public Enemy {
public:
    FireGoblin() : Enemy("炽热程序猿", 30, 6) {}
    void takeTurn(Player& player) override;
    std::vector<std::string> getDescription() const override;
};

class FrozenGoblin : public Enemy {
public:
    FrozenGoblin() : Enemy("冰霜程序猿", 30, 6) {}
    void takeTurn(Player& player) override;
    std::vector<std::string> getDescription() const override;
};

// =============================================================
// Caster（魔法师）
// 每回合有 0.5 的概率攻击，攻击类型随机
// 另外 0.5 的概率：
//   - 如果有 enemy 单位生命值 < 其最大生命值的 1/3，则为全体提供持续 2 回合、效果为 5 的 REGEN
//   - 否则全体获得持续 2 回合、效果为 2 的 STRENGTH
//   - 当前版本先只实现针对自己的增益
// =============================================================

class Caster : public Enemy {
public:
    Caster() : Enemy("魔法师", 50, 3) {}
    void takeTurn(Player& player) override;
    std::vector<std::string> getDescription() const override;
};

// ============================================================
// Boss 1: TemplateKing（程序猿神）
// ============================================================
// 核心机制：
// 1. 多阶段：生命值降低到 66%/33% 时进入新阶段，完全恢复护盾
// 2. 模式切换：每 3 回合切换模式（攻击模式 <-> 防御模式）
// 3. 复制机制：攻击模式下会"复制"玩家的攻击力（获得等量 STRENGTH）
// 4. 模板护盾：防御模式下每回合生成护盾，数量随阶段递增
// 5. 终极技：第三阶段每 4 回合释放一次强力 AOE（对玩家和所有仆从造成伤害）
// ============================================================

class TemplateKing : public Enemy {
public:
    enum class Phase { FIRST, SECOND, THIRD };
    enum class Mode { ATTACK, DEFENSE };

    TemplateKing() : Enemy("程序猿神", 200, 10) {
        currentPhase = Phase::FIRST;
        currentMode = Mode::ATTACK;
        turnsSinceLastSwitch = 0;
        turnsInPhase = 0;
    }

    void takeTurn(Player& player) override;
    std::vector<std::string> getDescription() const override;

private:
    Phase currentPhase;
    Mode currentMode;
    int turnsSinceLastSwitch;
    int turnsInPhase;

    void checkPhaseTransition();
    void switchMode();
    void attackMode(Player& player);
    void defenseMode(Player& player);
    void ultimateAttack(Player& player);
    int getShieldAmount() const;
};

// ============================================================
// Boss 2: ExceptionLord（异常魔王）
// ============================================================
// 核心机制：
// 1. 异常堆栈：每次受到伤害时累积"异常计数"（类似能量蓄积）
// 2. Try-Catch 模式：低血量时进入防御姿态，下一次致命伤害会被"捕获"（类似凤凰）
// 3. Throw 攻击：消耗异常计数，造成成倍伤害（1点计数 = 2倍伤害）
// 4. 异常链：每 5 回合释放连锁攻击，攻击次数等于当前异常计数
// 5. Finally 效果：死亡时对玩家造成固定真实伤害（无法减免）
// 6. 状态免疫：异常计数 >= 5 时免疫所有负面状态
// ============================================================

class ExceptionLord : public Enemy {
public:
    ExceptionLord() : Enemy("崩坏", 150, 8) {
        exceptionCount = 0;
        tryCatchActive = false;
        turnCounter = 0;
    }

    void takeTurn(Player& player) override;

    // 重写 takeDamage 来实现异常计数和 Try-Catch 机制
    void takeDamage(int dmg, DamageType dtype) override;

    // 重写 addStatus 来实现状态免疫
    void addStatus(Status s) override;

    // 重写死亡处理
    bool isAlive() const override;

    // 获取描述
    std::vector<std::string> getDescription() const override;

    // 公开访问器用于测试和 UI 显示
    int getExceptionCount() const { return exceptionCount; }
    bool isTryCatchActive() const { return tryCatchActive; }

private:
    int exceptionCount;        // 异常计数
    bool tryCatchActive;       // Try-Catch 保护是否激活
    int turnCounter;           // 回合计数器

    void gainException(int count);
    void throwAttack(Player& player);
    void chainAttack(Player& player);
    void activateTryCatch();
    void finallyEffect(Player& player);
};

#endif // ENEMY_H
