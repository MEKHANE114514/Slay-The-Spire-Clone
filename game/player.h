#ifndef PLAYER_H
#define PLAYER_H

#include <functional>
#include <vector>
#include <string>
#include "types.h"
#include "minion.h"

// ============================================================
// player.h — 玩家类
// 7 个核心成员函数（attack/takeDamage/summon/...）通过
// std::function 存储，函数牌可替换其实现
// ============================================================

class Enemy;  // 前向声明，attack() 需要

class Player {
public:
    // ===== 函数类型别名（using = 类型缩写，不是默认函数）=====
    using AttackFunc      = std::function<void(Enemy&)>;
    using TakeDamageFunc  = std::function<void(int, DamageType)>;
    using SummonFunc      = std::function<Minion()>;
    using CopySummonFunc  = std::function<Minion(const Minion&)>;
    using MoveSummonFunc  = std::function<Minion(Minion&&)>;
    using SacrificeFunc   = std::function<void(Minion&)>;
    using EscapeFunc      = std::function<bool()>;

    // ===== 构造 =====
    Player(std::string playerName = "Player",
           int maxHp = DEFAULT_MAX_HP,
           int maxEnergy = DEFAULT_MAX_ENERGY);

    // ===== 对外调用入口（永远不变，内部转发到 *_Impl）=====
    void attack(Enemy& target);
    void takeDamage(int dmg, DamageType type = DamageType::PHYSICAL);
    Minion summon();
    Minion copySummon(const Minion& original);
    Minion moveSummon(Minion&& original);
    void sacrifice(Minion& m);
    bool tryEscape();

    // ===== 函数牌 setter（替换 *_Impl 实现）=====
    void setAttackFunc(AttackFunc f) {
        attackImpl = std::move(f);
        if (onFunctionModified) onFunctionModified(FunctionTarget::ATTACK);
    }
    void setTakeDamageFunc(TakeDamageFunc f) {
        takeDamageImpl = std::move(f);
        if (onFunctionModified) onFunctionModified(FunctionTarget::TAKE_DAMAGE);
    }
    void setSummonFunc(SummonFunc f) {
        summonImpl = std::move(f);
        if (onFunctionModified) onFunctionModified(FunctionTarget::SUMMON);
    }
    void setCopySummonFunc(CopySummonFunc f) {
        copySummonImpl = std::move(f);
        if (onFunctionModified) onFunctionModified(FunctionTarget::COPY_SUMMON);
    }
    void setMoveSummonFunc(MoveSummonFunc f) {
        moveSummonImpl = std::move(f);
        if (onFunctionModified) onFunctionModified(FunctionTarget::MOVE_SUMMON);
    }
    void setSacrificeFunc(SacrificeFunc f) {
        sacrificeImpl = std::move(f);
        if (onFunctionModified) onFunctionModified(FunctionTarget::SACRIFICE);
    }
    void setEscapeFunc(EscapeFunc f) {
        escapeImpl = std::move(f);
        if (onFunctionModified) onFunctionModified(FunctionTarget::ESCAPE);
    }

    // getter（用于函数牌叠加：取旧函数 + 包装新逻辑）
    AttackFunc     getAttackFunc() const      { return attackImpl; }
    TakeDamageFunc getTakeDamageFunc() const  { return takeDamageImpl; }
    SummonFunc     getSummonFunc() const      { return summonImpl; }
    CopySummonFunc getCopySummonFunc() const  { return copySummonImpl; }
    MoveSummonFunc getMoveSummonFunc() const  { return moveSummonImpl; }
    SacrificeFunc  getSacrificeFunc() const   { return sacrificeImpl; }
    EscapeFunc     getEscapeFunc() const      { return escapeImpl; }

    // ===== 基础动作（受 ActionLimits 限制）=====
    bool canBasicAttack() const     { return actions.attacks > 0 && !isDisabled(); }
    bool canBasicDefend() const     { return actions.defends > 0 && !isDisabled(); }
    bool canBasicSummon() const     { return actions.summons > 0 && !isDisabled(); }
    bool canSacrifice() const       { return actions.sacrifices > 0 && !isDisabled(); }
    bool canCopySummon() const      { return actions.copies > 0 && !isDisabled(); }
    bool canMoveSummon() const      { return actions.moves > 0 && !isDisabled(); }
    bool canEscape() const          { return !actions.escapeUsed && !isDisabled(); }

    void basicAttack(Enemy& target);   // 消耗一次攻击机会
    void basicDefend();                // 进入防御姿态
    void basicSummon();                // 消耗一次召唤机会

    // ===== 状态管理 =====
    void heal(int amount);
    void addShield(int amount);
    void addStatus(Status s);
    void removeStatus(StatusType type);
    bool hasStatus(StatusType type) const;
    void tickStatuses();            // 回合结束：倒计时、灼烧扣血……
    void resetActionLimits();       // 新回合重置次数

    // ===== 攻击力计算 =====
    int baseAttack = 10;            // 基础攻击力
    int getEffectiveAttack() const; // 考虑状态加成后的实际攻击力

    // ===== 仆从管理 =====
    bool addMinion(Minion m);       // 加入场上，超过 MAX_MINIONS 返回 false
    void removeMinion(int index);

    // ===== 查询 =====
    bool isAlive() const            { return hp > 0; }
    bool isDisabled() const;        // 眩晕/冻结时返回 true

    // ===== 公共属性 =====
    std::string name;
    int hp, maxHp;
    int shield = 0;
    int energy, maxEnergy;
    Faction faction = Faction::NONE;

    ActionLimits actions;
    std::vector<Status> statuses;
    std::vector<Minion> minions;

    // ---- UI 回调（Qt 绑定，纯 C++ 接口，游戏逻辑不依赖 Qt）----
    // 仆从
    std::function<void(int)>             onMinionAdded;       // 仆从登场（index）
    std::function<void(int)>             onMinionRemoved;      // 仆从退场（index）
    // 属性变化
    std::function<void(int hp, int maxHp, int delta)> onHpChanged;    // 生命变化（delta：正=回血，负=扣血）
    std::function<void(int shield, int delta)>  onShieldChanged; // 护盾变化（delta：正=获得，负=消耗）
    std::function<void(int energy, int maxEnergy, int delta)> onEnergyChanged; // 能量变化（delta：正=获得，负=消耗）
    // 状态
    std::function<void(const Status&)>    onStatusAdded;    // 新增 Buff/Debuff
    std::function<void(StatusType)>       onStatusRemoved;  // Buff/Debuff 消失
    // 数值飘字
    std::function<void(int amount)>       onHealed;         // 回血飘字
    std::function<void(int dmg, DamageType)> onDamageReceived; // 受击飘字
    // 函数牌
    std::function<void(FunctionTarget)>   onFunctionModified; // 函数牌替换时播放特效

private:
    // ===== 7 个可被函数牌替换的核心实现 =====
    AttackFunc      attackImpl;
    TakeDamageFunc  takeDamageImpl;
    SummonFunc      summonImpl;
    CopySummonFunc  copySummonImpl;
    MoveSummonFunc  moveSummonImpl;
    SacrificeFunc   sacrificeImpl;
    EscapeFunc      escapeImpl;

    // 初始化 7 个默认函数（构造函数中调用）
    void initDefaultFunctions();
};

#endif // PLAYER_H
