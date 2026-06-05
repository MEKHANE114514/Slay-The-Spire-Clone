#include "cards.h"
#include "player.h"
#include "enemy.h"
#include "minion.h"
#include <cstdlib>
#include <algorithm>

// ============================================================
// Card 基类实现
// ============================================================

bool Card::canPlay(const Player& player) const {
    return player.energy >= cost && !player.isDisabled();
}

std::vector<std::string> Card::getCodeLines() const {
    return {"card.play(player, enemy);  // " + name};
}

// ============================================================
// FunctionCard 辅助方法
// ============================================================

std::string FunctionCard::getFunctionName(FunctionTarget ft) {
    switch (ft) {
        case FunctionTarget::ATTACK:       return "attack()";
        case FunctionTarget::TAKE_DAMAGE:  return "takeDamage()";
        case FunctionTarget::SUMMON:       return "summon()";
        case FunctionTarget::COPY_SUMMON:  return "copySummon()";
        case FunctionTarget::MOVE_SUMMON:  return "moveSummon()";
        case FunctionTarget::SACRIFICE:    return "sacrifice()";
        case FunctionTarget::ESCAPE:       return "escape()";
        default:                           return "unknown()";
    }
}

std::vector<std::string> FunctionCard::getCodeLines() const {
    static const std::string names[] = {
        "Attack", "TakeDamage", "Summon",
        "CopySummon", "MoveSummon", "Sacrifice", "Escape"
    };
    int idx = static_cast<int>(target);
    return {"player.set" + names[idx] + "Func(...);  // " + name};
}

// ============================================================
// TemplateCard 基础实现
// ============================================================

void TemplateCard::play(Player& player, Enemy* target) {
    if (!wrappedCard) return;
    applyWrapper(player, target);
}

Card* TemplateCard::clone() const {
    TemplateCard* cloned = cloneTemplate();
    if (wrappedCard) {
        cloned->setWrappedCard(static_cast<FunctionCard*>(wrappedCard->clone()));
    }
    return cloned;
}

std::vector<std::string> TemplateCard::getCodeLines() const {
    if (wrappedCard)
        return {"template.apply(player, enemy);  // " + name + " (" + wrappedCard->name + ")"};
    return {"template.apply(player, enemy);  // " + name};
}

// ============================================================
// 攻击函数牌实现
// ============================================================

void AttackEnhanceCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy) {
        int enhancedDmg = static_cast<int>(player.getEffectiveAttack() * 1.5);
        enemy.takeDamage(enhancedDmg, DamageType::PHYSICAL);
    });
}

void VampireAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy) {
        int oldHp = enemy.hp;
        oldAttack(enemy);
        int damage = oldHp - enemy.hp;
        if (damage > 0) {
            int healAmount = static_cast<int>(damage * 0.3);
            player.heal(healAmount);
        }
    });
}

void ComboAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy) {
        oldAttack(enemy);
        int halfDmg = player.getEffectiveAttack() / 2;
        enemy.takeDamage(halfDmg, DamageType::PHYSICAL);
    });
}

void CritAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy) {
        bool isCrit = (rand() % 100) < 30;
        if (isCrit) {
            int dmg = player.getEffectiveAttack() * 2;
            enemy.takeDamage(dmg, DamageType::PHYSICAL);
        } else {
            oldAttack(enemy);
        }
    });
}

void PoisonAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack](Enemy& enemy) {
        oldAttack(enemy);
        Status poison;
        poison.type = StatusType::POISON;
        poison.value = 3;
        poison.turnsRemaining = 5;
        enemy.addStatus(poison);
    });
}

void BurnAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack](Enemy& enemy) {
        oldAttack(enemy);
        Status burn;
        burn.type = StatusType::BURN;
        burn.value = 5;
        burn.turnsRemaining = 3;
        enemy.addStatus(burn);
    });
}

void ExecuteAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy) {
        float hpPercent = static_cast<float>(enemy.hp) / enemy.maxHp;
        if (hpPercent < 0.3f) {
            int dmg = player.getEffectiveAttack() * 3;
            enemy.takeDamage(dmg, DamageType::PHYSICAL);
        } else {
            oldAttack(enemy);
        }
    });
}

void SynergyAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy) {
        int bonusDmg = static_cast<int>(player.minions.size()) * 3;
        int totalDmg = player.getEffectiveAttack() + bonusDmg;
        enemy.takeDamage(totalDmg, DamageType::PHYSICAL);
    });
}

void BerserkerAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy) {
        int hpLost = player.maxHp - player.hp;
        int bonusDmg = (hpLost / 10) * 5;
        int totalDmg = player.getEffectiveAttack() + bonusDmg;
        enemy.takeDamage(totalDmg, DamageType::PHYSICAL);
    });
}

void MarkAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack](Enemy& enemy) {
        oldAttack(enemy);
        Status mark;
        mark.type = StatusType::MARK;
        mark.value = 50;
        mark.turnsRemaining = 3;
        enemy.addStatus(mark);
    });
}

// ============================================================
// 受击函数牌实现
// ============================================================

void IronWallCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int dmg, DamageType type) {
        int reduced = dmg > 5 ? dmg - 5 : 0;
        oldTakeDamage(reduced, type);
    });
}

void CounterDamageCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int dmg, DamageType type) {
        oldTakeDamage(dmg, type);
        // 注意：需要战斗上下文支持记录攻击者
        // 完整实现需要 Battle 类传递攻击者引用
    });
}

void RegenerationCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int dmg, DamageType type) {
        oldTakeDamage(dmg, type);
        player.heal(3);
    });
}

void DodgeCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int dmg, DamageType type) {
        bool dodged = (rand() % 100) < 30;
        if (!dodged) {
            oldTakeDamage(dmg, type);
        }
    });
}

void ThornsCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int dmg, DamageType type) {
        oldTakeDamage(dmg, type);
        // 注意：需要战斗上下文记录攻击者
    });
}

void RageCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int dmg, DamageType type) {
        oldTakeDamage(dmg, type);
        Status rage;
        rage.type = StatusType::RAGE;
        rage.value = 5;
        rage.turnsRemaining = 1;
        player.addStatus(rage);
    });
}

void FortifyCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int dmg, DamageType type) {
        oldTakeDamage(dmg, type);
        Status fortify;
        fortify.type = StatusType::FORTIFY;
        fortify.value = 2;
        fortify.turnsRemaining = -1;  // 永久
        player.addStatus(fortify);
    });
}

// ============================================================
// 构造/复制/移动/析构/逃跑函数牌实现
// ============================================================

void EnhancedSummonCard::play(Player& player, Enemy* target) {
    player.setSummonFunc([]() -> Minion {
        return Minion("强化仆从", 30, 8);
    });
}

void EliteSummonCard::play(Player& player, Enemy* target) {
    player.setSummonFunc([]() -> Minion {
        return Minion("精英仆从", ELITE_MINION_HP, ELITE_MINION_ATK, MinionType::ELITE);
    });
}

void MassProductionCard::play(Player& player, Enemy* target) {
    auto oldSummon = player.getSummonFunc();
    player.setSummonFunc([oldSummon, &player]() -> Minion {
        Minion m1 = oldSummon();
        Minion m2("复制" + m1.name, m1.maxHp / 2, m1.attack / 2);
        player.addMinion(std::move(m2));
        return m1;
    });
}

void PreciseCopyCard::play(Player& player, Enemy* target) {
    player.setCopySummonFunc([](const Minion& original) -> Minion {
        return Minion(original.name + "精准复制",
                     static_cast<int>(original.maxHp * 0.9),
                     static_cast<int>(original.attack * 0.9));
    });
}

void ProliferateCopyCard::play(Player& player, Enemy* target) {
    auto oldCopy = player.getCopySummonFunc();
    player.setCopySummonFunc([oldCopy, &player](const Minion& original) -> Minion {
        Minion primary = oldCopy(original);
        Minion secondary("次级" + primary.name, primary.maxHp / 2, primary.attack / 2);
        player.addMinion(std::move(secondary));
        return primary;
    });
}

void ExtractMoveCard::play(Player& player, Enemy* target) {
    auto oldMove = player.getMoveSummonFunc();
    player.setMoveSummonFunc([oldMove, &player](Minion&& original) -> Minion {
        int energy = original.attack / 2;
        player.energy += energy;
        return oldMove(std::move(original));
    });
}

void RemainsMoveCard::play(Player& player, Enemy* target) {
    auto oldMove = player.getMoveSummonFunc();
    player.setMoveSummonFunc([oldMove, &player](Minion&& original) -> Minion {
        Minion remains("残骸", 5, 1);
        player.addMinion(std::move(remains));
        return oldMove(std::move(original));
    });
}

void ExplodeSacrificeCard::play(Player& player, Enemy* target) {
    player.setSacrificeFunc([](Minion& m) {
        int damage = m.hp;
        // 注意：需要战斗上下文来对全体敌人造成伤害
        m.hp = 0;
    });
}

void InheritSacrificeCard::play(Player& player, Enemy* target) {
    player.setSacrificeFunc([&player](Minion& m) {
        if (m.hasStatus(StatusType::STRENGTH)) {
            for (auto& s : m.statuses) {
                if (s.type == StatusType::STRENGTH) {
                    player.addStatus(s);
                    break;
                }
            }
        }
        m.hp = 0;
    });
}

void RebirthSacrificeCard::play(Player& player, Enemy* target) {
    player.setSacrificeFunc([](Minion& m) {
        bool rebirth = (rand() % 100) < 30;
        if (rebirth) {
            m.hp = m.maxHp / 2;
        } else {
            m.hp = 0;
        }
    });
}

void CunningEscapeCard::play(Player& player, Enemy* target) {
    player.setEscapeFunc([]() -> bool {
        return (rand() % 100) < 70;  // 70% 成功率
    });
}

void EmergencyEscapeCard::play(Player& player, Enemy* target) {
    auto oldEscape = player.getEscapeFunc();
    player.setEscapeFunc([oldEscape, &player]() -> bool {
        bool success = oldEscape();
        if (!success) {
            int dmg = player.maxHp / 4;
            player.takeDamage(dmg, DamageType::PHYSICAL);
        }
        return true;  // 总是成功，但失败时扣血
    });
}

void RearguardEscapeCard::play(Player& player, Enemy* target) {
    auto oldEscape = player.getEscapeFunc();
    player.setEscapeFunc([oldEscape, target]() -> bool {
        bool success = oldEscape();
        if (success && target) {
            target->takeDamage(10, DamageType::PHYSICAL);
        }
        return success;
    });
}

// ============================================================
// 指令牌实现
// ============================================================

void PowerStrikeCard::play(Player& player, Enemy* target) {
    if (!target) return;
    int damage = player.getEffectiveAttack() * 2;
    target->takeDamage(damage, DamageType::PHYSICAL);
}

void SweepCard::play(Player& player, Enemy* target) {
    // 注意：需要战斗上下文来获取全体敌人
    // 这里简化为单体
    if (!target) return;
    int damage = player.getEffectiveAttack() / 2;
    target->takeDamage(damage, DamageType::PHYSICAL);
}

void DefendCard::play(Player& player, Enemy* target) {
    player.addShield(10);
}

void StrengthCard::play(Player& player, Enemy* target) {
    Status strengthBuff;
    strengthBuff.type = StatusType::STRENGTH;
    strengthBuff.value = 2;
    strengthBuff.turnsRemaining = 3;
    player.addStatus(strengthBuff);
}

void SummonCard::play(Player& player, Enemy* target) {
    Minion minion = player.summon();
    player.addMinion(std::move(minion));
}

void HealCard::play(Player& player, Enemy* target) {
    player.heal(15);
}

void PurifyCard::play(Player& player, Enemy* target) {
    player.statuses.clear();
}

void SacrificeCard::play(Player& player, Enemy* target) {
    if (!player.minions.empty()) {
        player.sacrifice(player.minions.front());
    }
}

void BloodSacrificeCard::play(Player& player, Enemy* target) {
    if (!player.minions.empty()) {
        int healAmount = player.minions.front().hp;
        player.sacrifice(player.minions.front());
        player.heal(healAmount);
    }
}

void LambdaCard::play(Player& player, Enemy* target) {
    // Lambda 注入：创建临时效果
    player.addShield(5);
    player.heal(5);
}

// ============================================================
// 模板牌实现
// ============================================================

void TripleEffectCard::applyWrapper(Player& player, Enemy* target) {
    if (!wrappedCard) return;
    wrappedCard->play(player, target);
    wrappedCard->play(player, target);
    wrappedCard->play(player, target);
}

void ForceInlineCard::applyWrapper(Player& player, Enemy* target) {
    if (!wrappedCard) return;
    int oldCost = wrappedCard->cost;
    wrappedCard->cost = oldCost > 1 ? oldCost - 1 : 0;
    wrappedCard->play(player, target);
    wrappedCard->cost = oldCost;
}

void DoubleEffectCard::applyWrapper(Player& player, Enemy* target) {
    if (!wrappedCard) return;
    wrappedCard->play(player, target);
    wrappedCard->play(player, target);
}
