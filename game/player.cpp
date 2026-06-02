#include "player.h"
#include "enemy.h"       // initDefaultFunctions 中调用 target.takeDamage()
#include <algorithm>     // std::min, std::remove_if, std::any_of
#include <cstdlib>       // rand
#include <ctime>         // time（后续在别处初始化随机种子）

Player::Player(std::string playerName, int maxHp, int maxEnergy)
    : name(std::move(playerName))
    , hp(maxHp), maxHp(maxHp)
    , energy(maxEnergy), maxEnergy(maxEnergy)
{
    initDefaultFunctions();
    resetActionLimits();
}

// ============================================================
// 给 7 个 *_Impl 赋默认 lambda —— 函数牌打出前的基础行为
// ============================================================
void Player::initDefaultFunctions()
{
    // ---- 默认攻击：造成 10 点物理伤害 ----
    attackImpl = [this](Enemy& target) {
        target.takeDamage(10, DamageType::PHYSICAL);
    };

    // ---- 默认受击：先扣护盾，再扣生命 ----
    takeDamageImpl = [this](int dmg, DamageType /*type*/) {
        if (shield > 0) {
            int blocked = std::min(shield, dmg);
            shield -= blocked;
            dmg -= blocked;
        }
        hp -= dmg;
    };

    // ---- 默认召唤：创建一个白板普通仆从 ----
    summonImpl = [this]() -> Minion {
        return Minion("基础仆从", MINION_BASE_HP, MINION_BASE_ATK);
    };

    // ---- 默认复制：属性降为 70% ----
    copySummonImpl = [this](const Minion& original) -> Minion {
        return Minion(original.name + "的复制",
                      static_cast<int>(original.maxHp * 0.7),
                      static_cast<int>(original.attack * 0.7));
    };

    // ---- 默认移动：直接转移资源，原对象变为空壳 ----
    moveSummonImpl = [this](Minion&& original) -> Minion {
        return std::move(original);
    };

    // ---- 默认献祭：销毁仆从，无额外效果 ----
    sacrificeImpl = [this](Minion& m) {
        m.hp = 0;  // 直接杀死（后续由 BattleContext 移除）
    };

    // ---- 默认逃跑：50% 概率成功 ----
    escapeImpl = [this]() -> bool {
        return (rand() % 100) < 50;
    };
}
