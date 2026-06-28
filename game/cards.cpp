#include "cards.h"
#include "cards_full.h"
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
    player.setAttackFunc([oldAttack, &player](Enemy& enemy, std::function<int(int)> mod) {
        auto enhancedMod = [mod](int atk) -> int {
            int base = mod ? mod(atk) : atk;
            return static_cast<int>(base * 1.5f);
        };
        oldAttack(enemy, enhancedMod);
    });
}

std::vector<std::string> AttackEnhanceCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack](Enemy& enemy, mod) {",
        "    auto enhancedMod = [mod](int atk) { return 1.5 * (mod ? mod(atk) : atk); };",
        "    oldAttack(enemy, enhancedMod);",
        "});"
    };
}

void VampireAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy, std::function<int(int)> mod) {
        int oldHp = enemy.hp;
        oldAttack(enemy, mod);
        int damage = oldHp - enemy.hp;
        if (damage > 0) {
            int healAmount = static_cast<int>(damage * 0.3);
            player.heal(healAmount);
        }
    });
}

std::vector<std::string> VampireAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack, &player](Enemy& enemy, mod) {",
        "    int oldHp = enemy.hp;",
        "    oldAttack(enemy, mod);",
        "    int damage = oldHp - enemy.hp;",
        "    if (damage > 0) player.heal(damage * 0.3);",
        "});"
    };
}

void ComboAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy, std::function<int(int)> mod) {
        oldAttack(enemy, mod);
        int halfDmg = mod ? mod(player.getEffectiveAttack()) / 2 : player.getEffectiveAttack() / 2;
        enemy.takeDamage(halfDmg, DamageType::PHYSICAL);
    });
}


std::vector<std::string> ComboAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack, &player](Enemy& enemy, mod) {",
        "    oldAttack(enemy, mod);",
        "    int halfDmg = mod ? mod(player.getEffectiveAttack()) / 2 : player.getEffectiveAttack() / 2;",
        "    enemy.takeDamage(halfDmg, PHYSICAL);",
        "});"
    };
}

void CritAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy, std::function<int(int)> mod) {
        bool isCrit = (rand() % 100) < 50;
        if (isCrit) {
            auto critMod = [mod](int atk) -> int {
                int base = mod ? mod(atk) : atk;
                return base * 2;
            };
            oldAttack(enemy, critMod);
        } else {
            oldAttack(enemy, mod);
        }
    });
}

std::vector<std::string> CritAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack, &player](Enemy& enemy, mod) {",
        "    bool isCrit = (rand() \% 100) < 50;",
        "    if (isCrit) {",
        "        auto critMod = [mod](int atk) { return 2 * (mod ? mod(atk) : atk); };",
        "        oldAttack(enemy, critMod);",
        "    } else oldAttack(enemy, mod);",
        "});"
    };
}

void PoisonAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack](Enemy& enemy, std::function<int(int)> mod) {
        oldAttack(enemy, mod);
        Status poison;
        poison.type = StatusType::POISON;
        poison.value = 3;
        poison.turnsRemaining = 5;
        for (int i = 0; i < 6; ++i) enemy.addStatus(poison);
    });
}

std::vector<std::string> PoisonAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack](Enemy& enemy, mod) {",
        "    oldAttack(enemy, mod);",
        "    Status poison(POISON, 3, 5);",
        "    for (int i = 0; i < 6; ++i) enemy.addStatus(poison);",
        "});"
    };
}

void BurnAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack](Enemy& enemy, std::function<int(int)> mod) {
        oldAttack(enemy, mod);
        Status burn;
        burn.type = StatusType::BURN;
        burn.value = 5;
        burn.turnsRemaining = 3;
        for (int i = 1; i <= 4; ++i) enemy.addStatus(burn);
    });
}

std::vector<std::string> BurnAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack](Enemy& enemy, mod) {",
        "    oldAttack(enemy, mod);",
        "    Status burn(BURN, 5, 3);",
        "    for (int i = 1; i <= 4; ++i) enemy.addStatus(burn);",
        "});"
    };
}

void ExecuteAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy, std::function<int(int)> mod) {
        float hpPercent = static_cast<float>(enemy.hp) / enemy.maxHp;
        if (hpPercent < 0.3f) {
            auto execMod = [mod](int atk) -> int {
                int base = mod ? mod(atk) : atk;
                return base * 3;
            };
            oldAttack(enemy, execMod);
        } else {
            oldAttack(enemy, mod);
        }
    });
}

std::vector<std::string> ExecuteAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack, &player](Enemy& enemy, mod) {",
        "    float hpPercent = enemy.hp / enemy.maxHp;",
        "    if (hpPercent < 0.3f) {",
        "        auto execMod = [mod](int atk) { return 3 * (mod ? mod(atk) : atk); };",
        "        oldAttack(enemy, execMod);",
        "    } else oldAttack(enemy, mod);",
        "});"
    };
}

void SynergyAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy, std::function<int(int)> mod) {
        auto synergyMod = [mod, &player](int atk) -> int {
            int base = mod ? mod(atk) : atk;
            int bonus = static_cast<int>(player.minions.size()) * 3;
            return base + bonus;
        };
        oldAttack(enemy, synergyMod);
    });
}

std::vector<std::string> SynergyAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack, &player](Enemy& enemy, mod) {",
        "    auto synergyMod = [mod, &player](int atk) {",
        "        return (mod ? mod(atk) : atk) + player.minions.size() * 3;",
        "    };",
        "    oldAttack(enemy, synergyMod);",
        "});"
    };
}

void BerserkerAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack, &player](Enemy& enemy, std::function<int(int)> mod) {
        auto berserkMod = [mod, &player](int atk) -> int {
            int base = mod ? mod(atk) : atk;
            int hpLost = player.maxHp - player.hp;
            int bonus = (hpLost / 10) * 5;
            return base + bonus;
        };
        oldAttack(enemy, berserkMod);
    });
}

std::vector<std::string> BerserkerAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack, &player](Enemy& enemy, mod) {",
        "    auto berserkMod = [mod, &player](int atk) {",
        "        int hpLost = player.maxHp - player.hp;",
        "        return (mod ? mod(atk) : atk) + (hpLost / 10) * 5;",
        "    };",
        "    oldAttack(enemy, berserkMod);",
        "});"
    };
}

void MarkAttackCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([oldAttack](Enemy& enemy, std::function<int(int)> mod) {
        oldAttack(enemy, mod);
        Status mark;
        mark.type = StatusType::MARK;
        mark.value = 50;
        mark.turnsRemaining = 3;
        enemy.addStatus(mark);
    });
}

std::vector<std::string> MarkAttackCard::getCodeLines() const {
    return {
        "auto oldAttack = player.getAttackFunc();",
        "player.setAttackFunc([oldAttack](Enemy& enemy, mod) {",
        "    oldAttack(enemy, mod);",
        "    Status mark(MARK, 50, 3);",
        "    enemy.addStatus(mark);",
        "});"
    };
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

std::vector<std::string> IronWallCard::getCodeLines() const {
    return {
        "auto oldTakeDamage = player.getTakeDamageFunc();",
        "player.setTakeDamageFunc([oldTakeDamage](int dmg, DamageType type) {",
        "    int reduced = dmg > 5 ? dmg - 5 : 0;",
        "    oldTakeDamage(reduced, type);",
        "});"
    };
}

void RegenerationCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int dmg, DamageType type) {
        oldTakeDamage(dmg, type);
        player.heal(3);
    });
}

std::vector<std::string> RegenerationCard::getCodeLines() const {
    return {
        "auto oldTakeDamage = player.getTakeDamageFunc();",
        "player.setTakeDamageFunc([oldTakeDamage, &player](int dmg, DamageType type) {",
        "    oldTakeDamage(dmg, type);",
        "    player.heal(3);",
        "});"
    };
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
        return (rand() % 100) < 70;  // 70\% 成功率
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

void StrikeCard::play(Player& player, Enemy* target) {
    if (!target) return;
    player.attack(*target);
}

void PowerStrikeCard::play(Player& player, Enemy* target) {
    if (!target) return;
    player.attack(*target, [](int atk) { return atk * 2; });
}

void DefendCard::play(Player& player, Enemy* target) {
    player.addShield(10);
}

void StrengthCard::play(Player& player, Enemy* target) {
    int strengthValue = static_cast<int>(player.getEffectiveAttack() * 0.5);
    Status strengthBuff;
    strengthBuff.type = StatusType::STRENGTH;
    strengthBuff.value = strengthValue;
    strengthBuff.turnsRemaining = 3;
    player.addStatus(strengthBuff);
}

void SummonCard::play(Player& player, Enemy* target) {
    if (player.minions.size() < 2) {
        Minion minion = player.summon();
        player.addMinion(std::move(minion));
    }
}

void HealCard::play(Player& player, Enemy* target) {
    player.heal(20);
}

void PurifyCard::play(Player& player, Enemy* target) {
    player.statuses.clear();
}

void BloodSacrificeCard::play(Player& player, Enemy* target) {
    if (!player.minions.empty()) {
        int healAmount = player.minions.front().hp;
        player.sacrifice(player.minions.front());
        player.heal(healAmount);
    }
}

void FortressCard::play(Player& player, Enemy* /*target*/) {
    player.addShield(20);
}

void EmergencyDodgeCard::play(Player& player, Enemy* /*target*/) {
    Status dodge;
    dodge.type = StatusType::DODGE;
    dodge.value = 1;
    dodge.turnsRemaining = 1;
    player.addStatus(dodge);
}

void QuickCopyCard::play(Player& player, Enemy* /*target*/) {
    if (player.minions.size() < 2 && !player.minions.empty()) {
        Minion copy = player.copySummon(player.minions[0]);
        player.addMinion(std::move(copy));
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

// ============================================================
// 补充所有缺失的 getCodeLines 实现
// ============================================================

std::vector<std::string> DodgeCard::getCodeLines() const {
    return {
        "auto oldTakeDamage = player.getTakeDamageFunc();",
        "player.setTakeDamageFunc([oldTakeDamage](int dmg, DamageType type) {",
        "    bool dodged = (rand() \% 100) < 30;",
        "    if (!dodged) oldTakeDamage(dmg, type);",
        "});"
    };
}

std::vector<std::string> RageCard::getCodeLines() const {
    return {
        "auto oldTakeDamage = player.getTakeDamageFunc();",
        "player.setTakeDamageFunc([oldTakeDamage, &player](int dmg, DamageType type) {",
        "    oldTakeDamage(dmg, type);",
        "    Status rage(RAGE, 5, 1);",
        "    player.addStatus(rage);",
        "});"
    };
}

std::vector<std::string> EnhancedSummonCard::getCodeLines() const {
    return {
        "player.setSummonFunc([]() -> Minion {",
        "    return Minion(\"强化仆从\", 30, 8);",
        "});"
    };
}

std::vector<std::string> EliteSummonCard::getCodeLines() const {
    return {
        "player.setSummonFunc([]() -> Minion {",
        "    return Minion(\"精英仆从\", ELITE_MINION_HP, ELITE_MINION_ATK, ELITE);",
        "});"
    };
}

std::vector<std::string> MassProductionCard::getCodeLines() const {
    return {
        "auto oldSummon = player.getSummonFunc();",
        "player.setSummonFunc([oldSummon, &player]() -> Minion {",
        "    Minion m1 = oldSummon();",
        "    Minion m2(\"复制\" + m1.name, m1.maxHp / 2, m1.attack / 2);",
        "    player.addMinion(std::move(m2));",
        "    return m1;",
        "});"
    };
}

std::vector<std::string> PreciseCopyCard::getCodeLines() const {
    return {
        "player.setCopySummonFunc([](const Minion& original) -> Minion {",
        "    return Minion(original.name + \"精准复制\", original.maxHp * 0.9, original.attack * 0.9);",
        "});"
    };
}

std::vector<std::string> ProliferateCopyCard::getCodeLines() const {
    return {
        "auto oldCopy = player.getCopySummonFunc();",
        "player.setCopySummonFunc([oldCopy, &player](const Minion& original) -> Minion {",
        "    Minion primary = oldCopy(original);",
        "    Minion secondary(\"次级\" + primary.name, primary.maxHp / 2, primary.attack / 2);",
        "    player.addMinion(std::move(secondary));",
        "    return primary;",
        "});"
    };
}

std::vector<std::string> ExtractMoveCard::getCodeLines() const {
    return {
        "auto oldMove = player.getMoveSummonFunc();",
        "player.setMoveSummonFunc([oldMove, &player](Minion&& original) -> Minion {",
        "    int energy = original.attack / 2;",
        "    player.energy += energy;",
        "    return oldMove(std::move(original));",
        "});"
    };
}

std::vector<std::string> RemainsMoveCard::getCodeLines() const {
    return {
        "auto oldMove = player.getMoveSummonFunc();",
        "player.setMoveSummonFunc([oldMove, &player](Minion&& original) -> Minion {",
        "    Minion remains(\"残骸\", 5, 1);",
        "    player.addMinion(std::move(remains));",
        "    return oldMove(std::move(original));",
        "});"
    };
}

std::vector<std::string> ExplodeSacrificeCard::getCodeLines() const {
    return {
        "player.setSacrificeFunc([](Minion& m) {",
        "    int damage = m.hp;",
        "    // 对全体敌人造成伤害",
        "    m.hp = 0;",
        "});"
    };
}

std::vector<std::string> InheritSacrificeCard::getCodeLines() const {
    return {
        "player.setSacrificeFunc([&player](Minion& m) {",
        "    if (m.hasStatus(STRENGTH)) {",
        "        for (auto& s : m.statuses) {",
        "            if (s.type == STRENGTH) {",
        "                player.addStatus(s);",
        "                break;",
        "            }",
        "        }",
        "    }",
        "    m.hp = 0;",
        "});"
    };
}

std::vector<std::string> RebirthSacrificeCard::getCodeLines() const {
    return {
        "player.setSacrificeFunc([](Minion& m) {",
        "    bool rebirth = (rand() \% 100) < 30;",
        "    if (rebirth) m.hp = m.maxHp / 2;",
        "    else m.hp = 0;",
        "});"
    };
}

std::vector<std::string> CunningEscapeCard::getCodeLines() const {
    return {
        "player.setEscapeFunc([]() -> bool {",
        "    return (rand() \% 100) < 70;",
        "});"
    };
}

std::vector<std::string> EmergencyEscapeCard::getCodeLines() const {
    return {
        "auto oldEscape = player.getEscapeFunc();",
        "player.setEscapeFunc([oldEscape, &player]() -> bool {",
        "    bool success = oldEscape();",
        "    if (!success) {",
        "        int dmg = player.maxHp / 4;",
        "        player.takeDamage(dmg, PHYSICAL);",
        "    }",
        "    return true;",
        "});"
    };
}

std::vector<std::string> RearguardEscapeCard::getCodeLines() const {
    return {
        "auto oldEscape = player.getEscapeFunc();",
        "player.setEscapeFunc([oldEscape, target]() -> bool {",
        "    bool success = oldEscape();",
        "    if (success && target) {",
        "        target->takeDamage(10, PHYSICAL);",
        "    }",
        "    return success;",
        "});"
    };
}

std::vector<std::string> StrikeCard::getCodeLines() const {
    return {"player.attack(enemy);"};
}

std::vector<std::string> PowerStrikeCard::getCodeLines() const {
    return {"player.attack(enemy, 2 * player.baseAttack);"};
}

std::vector<std::string> DefendCard::getCodeLines() const {
    return {"player.shield += 10;"};
}

std::vector<std::string> FortressCard::getCodeLines() const {
    return {"player.shield += 20;"};
}

std::vector<std::string> FortifyCard::getCodeLines() const {
    return {
        "auto oldTakeDamage = player.getTakeDamageFunc();",
        "player.setTakeDamageFunc([oldTakeDamage, &player](int dmg, DamageType type) {",
        "    oldTakeDamage(dmg, type);",
        "    Status fortify(FORTIFY, 2, -1);",
        "    player.addStatus(fortify);",
        "});"
    };
}

std::vector<std::string> EmergencyDodgeCard::getCodeLines() const {
    return {"player.addStatus(DODGE(1, 1));"};
}

std::vector<std::string> HealCard::getCodeLines() const {
    return {"player.heal(20);"};
}

std::vector<std::string> SummonCard::getCodeLines() const {
    return {"if (player.minions.size() < 2) player.summon();"};
}

std::vector<std::string> QuickCopyCard::getCodeLines() const {
    return {
        "if (player.minions.size() < 2 && !player.minions.empty())",
        "    player.copySummon(player.minions[0]);"
    };
}

std::vector<std::string> BloodSacrificeCard::getCodeLines() const {
    return {
        "if (!player.minions.empty()) {",
        "    player.heal(player.minions[0].hp);",
        "    player.sacrifice(player.minions[0]);",
        "}"
    };
}

std::vector<std::string> StrengthCard::getCodeLines() const {
    return {"player.addStatus(STRENGTH(3, 2));"};
}

std::vector<std::string> PurifyCard::getCodeLines() const {
    return {"player.clearDebuffs();"};
}

std::vector<std::string> LambdaCard::getCodeLines() const {
    return {
        "player.shield += 5;",
        "player.heal(5);"
    };
}

std::vector<std::string> ForceInlineCard::getCodeLines() const {
    return {"// Cost -1 for wrapped card"};
}

std::vector<std::string> DoubleEffectCard::getCodeLines() const {
    return {"// Execute wrapped card twice"};
}

std::vector<std::string> TripleEffectCard::getCodeLines() const {
    return {"// Execute wrapped card 3 times"};
}

// ============================================================
// 实现 cards_full.h 中所有缺失的卡牌
// ============================================================

// 攻击函数·暴击 (AttackCritCard from cards_full.h)
void AttackCritCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int damage = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        if (rand() % 100 < 30) {  // 30\% 概率
            damage *= 2;
        }
        enemy.takeDamage(damage, DamageType::PHYSICAL);
    });
}

std::vector<std::string> AttackCritCard::getCodeLines() const {
    return {
        "if (rand() < 0.3) player.attack(enemy, 2 * dmg);",
        "else player.attack(enemy, dmg);"
    };
}

std::vector<std::string> AttackSplashCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, PHYSICAL);",
        "for (Enemy& adj : adjacentEnemies) adj.takeDamage(0.5 * dmg);"
    };
}

// 攻击函数·破甲
void AttackPierceCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int shieldPierce = enemy.shield / 2;
        enemy.shield -= shieldPierce;
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::PHYSICAL);
    });
}

std::vector<std::string> AttackPierceCard::getCodeLines() const {
    return {
        "int shieldPierce = enemy.shield / 2;",
        "enemy.shield -= shieldPierce;",
        "enemy.takeDamage(dmg, PHYSICAL);"
    };
}

// 攻击函数·毒击
void AttackPoisonCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::PHYSICAL);
        Status poison;
        poison.type = StatusType::POISON;
        poison.value = 3;
        poison.turnsRemaining = 3;
        enemy.addStatus(poison);
    });
}

std::vector<std::string> AttackPoisonCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, PHYSICAL);",
        "Status poison(POISON, 3, 3);  // 3 stacks, 3 turns",
        "enemy.addStatus(poison);"
    };
}

// 攻击函数·灼烧
void AttackBurnCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::FIRE);
        Status burn;
        burn.type = StatusType::BURN;
        burn.value = 5;
        burn.turnsRemaining = 3;
        enemy.addStatus(burn);
    });
}

std::vector<std::string> AttackBurnCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, FIRE);",
        "Status burn(BURN, 5, 3);  // 5 dmg/turn, 3 turns",
        "enemy.addStatus(burn);"
    };
}

// 攻击函数·冰冻
void AttackFreezeCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::ICE);
        if (rand() % 100 < 20) {  // 20\% 概率
            Status frozen;
            frozen.type = StatusType::FREEZE;
            frozen.value = 0;
            frozen.turnsRemaining = 1;
            enemy.addStatus(frozen);
        }
    });
}

std::vector<std::string> AttackFreezeCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, ICE);",
        "if (rand() \% 100 < 20) {",
        "    Status frozen(FREEZE, 0, 1);",
        "    enemy.addStatus(frozen);",
        "}"
    };
}

// 攻击函数·雷霆
void AttackLightningCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::LIGHTNING);
        // TODO: 连锁伤害到其他敌人
    });
}

std::vector<std::string> AttackLightningCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, LIGHTNING);",
        "for (Enemy& e : enemies) {",
        "    e.takeDamage(0.3 * dmg, LIGHTNING);  // Chain 30\%",
        "}"
    };
}

// 攻击函数·阴影
void AttackShadowCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::SHADOW);
    });
}

std::vector<std::string> AttackShadowCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, SHADOW);"
    };
}

// 攻击函数·神圣
void AttackHolyCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int damage = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(damage, DamageType::HOLY);
        player.heal(damage / 2);  // 治疗 50%
    });
}

std::vector<std::string> AttackHolyCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, HOLY);",
        "player.heal(0.5 * dmg);"
    };
}

// 攻击函数·回复
void AttackHealCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int damage = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(damage, DamageType::PHYSICAL);
        player.heal(damage / 3);
    });
}

std::vector<std::string> AttackHealCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, PHYSICAL);",
        "player.heal(dmg / 3);"
    };
}

// 攻击函数·虚弱
void AttackWeakenCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::PHYSICAL);
        Status weak;
        weak.type = StatusType::WEAKEN;
        weak.value = 25;
        weak.turnsRemaining = 2;
        enemy.addStatus(weak);
    });
}

std::vector<std::string> AttackWeakenCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, PHYSICAL);",
        "enemy.addStatus(WEAK(2, 25));  // -25\% atk for 2 turns"
    };
}

// 攻击函数·眩晕
void AttackStunCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::PHYSICAL);
        if (rand() % 100 < 15) {
            Status stunned;
            stunned.type = StatusType::STUN;
            stunned.value = 0;
            stunned.turnsRemaining = 1;
            enemy.addStatus(stunned);
        }
    });
}

std::vector<std::string> AttackStunCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, PHYSICAL);",
        "if (rand() \% 100 < 15) enemy.addStatus(STUNNED(1));"
    };
}

// 攻击函数·回响
void AttackEchoCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int damage = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(damage, DamageType::PHYSICAL);
        enemy.takeDamage(damage / 2, DamageType::PHYSICAL);  // 回响伤害
    });
}

std::vector<std::string> AttackEchoCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, PHYSICAL);",
        "enemy.takeDamage(0.5 * dmg, PHYSICAL);  // Echo"
    };
}

// 攻击函数·蓄力
void AttackChargeCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        static int chargeStacks = 0;
        chargeStacks++;
        int base = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        int damage = base + chargeStacks * 5;
        enemy.takeDamage(damage, DamageType::PHYSICAL);
    });
}

std::vector<std::string> AttackChargeCard::getCodeLines() const {
    return {
        "static int charge = 0; charge++;",
        "enemy.takeDamage(dmg + 5 * charge, PHYSICAL);"
    };
}

// 攻击函数·分裂
void AttackSplitCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int base = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        int splitDmg = base / 2;
        enemy.takeDamage(splitDmg, DamageType::PHYSICAL);
        enemy.takeDamage(splitDmg, DamageType::PHYSICAL);
    });
}

std::vector<std::string> AttackSplitCard::getCodeLines() const {
    return {
        "int splitDmg = dmg / 2;",
        "enemy.takeDamage(splitDmg, PHYSICAL);",
        "enemy.takeDamage(splitDmg, PHYSICAL);"
    };
}

// 攻击函数·风暴
void AttackWindfuryCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        for (int i = 0; i < 2; i++) {
            enemy.takeDamage(dmg, DamageType::PHYSICAL);
        }
    });
}

std::vector<std::string> AttackWindfuryCard::getCodeLines() const {
    return {
        "for (int i = 0; i < 2; i++) {",
        "    enemy.takeDamage(dmg, PHYSICAL);",
        "}"
    };
}

// 攻击函数·吸收
void AttackAbsorbCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int damage = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(damage, DamageType::SHADOW);
        player.shield += damage / 4;
    });
}

std::vector<std::string> AttackAbsorbCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, SHADOW);",
        "player.shield += 0.25 * dmg;"
    };
}

// 攻击函数·精准
void AttackPrecisionCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        // 精准攻击：直接伤害HP，不考虑护盾
        int damage = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.hp -= damage;  // 绕过 takeDamage 的护盾计算
    });
}

std::vector<std::string> AttackPrecisionCard::getCodeLines() const {
    return {
        "enemy.hp -= dmg;  // Bypass shield"
    };
}

// 攻击函数·协同
void AttackSynergyCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int base = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        int bonus = player.minions.size() * 3;
        enemy.takeDamage(base + bonus, DamageType::PHYSICAL);
    });
}

std::vector<std::string> AttackSynergyCard::getCodeLines() const {
    return {
        "int bonus = player.minions.size() * 3;",
        "enemy.takeDamage(dmg + bonus, PHYSICAL);"
    };
}

// 攻击函数·标记
void AttackMarkCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int dmg = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        enemy.takeDamage(dmg, DamageType::PHYSICAL);
        Status mark;
        mark.type = StatusType::MARK;
        mark.value = 50;
        mark.turnsRemaining = 3;
        enemy.addStatus(mark);
    });
}

std::vector<std::string> AttackMarkCard::getCodeLines() const {
    return {
        "enemy.takeDamage(dmg, PHYSICAL);",
        "enemy.addStatus(MARK(3, 50));  // +50\% damage taken"
    };
}

// 攻击函数·处决
void AttackExecuteCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int damage = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        if (enemy.hp < enemy.maxHp * 0.3) {
            damage *= 3;
        }
        enemy.takeDamage(damage, DamageType::PHYSICAL);
    });
}

std::vector<std::string> AttackExecuteCard::getCodeLines() const {
    return {
        "if (enemy.hp < 0.3 * enemy.maxHp) enemy.takeDamage(3 * dmg, PHYSICAL);",
        "else enemy.takeDamage(dmg, PHYSICAL);"
    };
}

// 攻击函数·狂暴
void AttackBerserkerCard::play(Player& player, Enemy* target) {
    auto oldAttack = player.getAttackFunc();
    player.setAttackFunc([&player](Enemy& enemy, std::function<int(int)> mod) {
        int base = mod ? mod(player.getEffectiveAttack()) : player.getEffectiveAttack();
        int bonus = (player.maxHp - player.hp) / 2;
        enemy.takeDamage(base + bonus, DamageType::PHYSICAL);
    });
}

std::vector<std::string> AttackBerserkerCard::getCodeLines() const {
    return {
        "int bonus = (player.maxHp - player.hp) / 2;",
        "enemy.takeDamage(dmg + bonus, PHYSICAL);"
    };
}

// ============================================================
// 防御函数牌
// ============================================================

// 防御函数·铁壁
void DefendIronWallCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int damage, DamageType type) {
        int reduced = std::max(0, damage - 5);
        oldTakeDamage(reduced, type);
    });
}

std::vector<std::string> DefendIronWallCard::getCodeLines() const {
    return {
        "int reduced = max(0, damage - 5);",
        "oldTakeDamage(reduced, type);"
    };
}

// 防御函数·再生
void DefendRegenerationCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int damage, DamageType type) {
        oldTakeDamage(damage, type);
        player.heal(3);  // 受击时恢复 3 点生命
    });
}

std::vector<std::string> DefendRegenerationCard::getCodeLines() const {
    return {
        "oldTakeDamage(damage, type);",
        "player.heal(3);"
    };
}

// 防御函数·护盾
void DefendShieldCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int damage, DamageType type) {
        oldTakeDamage(damage, type);
        player.shield += 6;
    });
}

std::vector<std::string> DefendShieldCard::getCodeLines() const {
    return {
        "oldTakeDamage(damage, type);",
        "player.shield += 6;"
    };
}

// 防御函数·闪避
void DefendDodgeCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int damage, DamageType type) {
        if (rand() % 100 < 25) {  // 25% 闪避
            oldTakeDamage(0, type);
        } else {
            oldTakeDamage(damage, type);
        }
    });
}

std::vector<std::string> DefendDodgeCard::getCodeLines() const {
    return {
        "if (rand() \% 100 < 25) return;  // 25\% dodge",
        "oldTakeDamage(damage, type);"
    };
}

// 防御函数·护甲
void DefendArmorCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int damage, DamageType type) {
        int reduced = damage * 2 / 3;
        oldTakeDamage(reduced, type);
    });
}

std::vector<std::string> DefendArmorCard::getCodeLines() const {
    return {
        "int reduced = damage * 2 / 3;",
        "oldTakeDamage(reduced, type);"
    };
}

// 防御函数·吸收
void DefendAbsorbCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int damage, DamageType type) {
        oldTakeDamage(damage, type);
        player.heal(damage / 5);  // 吸收 20% 伤害转化为生命
    });
}

std::vector<std::string> DefendAbsorbCard::getCodeLines() const {
    return {
        "oldTakeDamage(damage, type);",
        "player.heal(damage / 5);"
    };
}

// 防御函数·分散
void DefendDistributeCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage, &player](int damage, DamageType type) {
        if (!player.minions.empty()) {
            int distributed = damage / (player.minions.size() + 1);
            oldTakeDamage(distributed, type);
            for (auto& minion : player.minions) {
                minion.takeDamage(distributed);
            }
        } else {
            oldTakeDamage(damage, type);
        }
    });
}

std::vector<std::string> DefendDistributeCard::getCodeLines() const {
    return {
        "if (!player.minions.empty()) {",
        "    int distributed = damage / (player.minions.size() + 1);",
        "    oldTakeDamage(distributed, type);",
        "    for (Minion& m : player.minions) m.takeDamage(distributed);",
        "} else oldTakeDamage(damage, type);"
    };
}

// 防御函数·坚固
void DefendFortifyCard::play(Player& player, Enemy* target) {
    auto oldTakeDamage = player.getTakeDamageFunc();
    player.setTakeDamageFunc([oldTakeDamage](int damage, DamageType type) {
        int capped = std::min(damage, 15);  // 单次伤害上限 15
        oldTakeDamage(capped, type);
    });
}

std::vector<std::string> DefendFortifyCard::getCodeLines() const {
    return {
        "int capped = min(damage, 15);",
        "oldTakeDamage(capped, type);"
    };
}

// ============================================================
// 召唤函数牌
// ============================================================

// 构造函数·强化
void SummonEnhanceCard::play(Player& player, Enemy* target) {
    player.setSummonFunc([]() -> Minion {
        return Minion("Enhanced", 20, 8);  // 强化的仆从
    });
}

std::vector<std::string> SummonEnhanceCard::getCodeLines() const {
    return {
        "return Minion(\"Enhanced\", 20, 8);"
    };
}

// 构造函数·量产
void SummonMassCard::play(Player& player, Enemy* target) {
    auto oldSummon = player.getSummonFunc();
    player.setSummonFunc([oldSummon, &player]() -> Minion {
        Minion minion = oldSummon();
        // 尝试召唤多个
        if (player.minions.size() < 2) {
            player.addMinion(Minion("Mass", 10, 5));
        }
        return minion;
    });
}

std::vector<std::string> SummonMassCard::getCodeLines() const {
    return {
        "Minion m = oldSummon();",
        "if (player.minions.size() < 2) {",
        "    player.addMinion(Minion(\"Mass\", 10, 5));",
        "}",
        "return m;"
    };
}

// ============================================================
// 复制函数牌
// ============================================================

// 复制函数·精准
void CopyPrecisionCard::play(Player& player, Enemy* target) {
    player.setCopySummonFunc([](const Minion& original) -> Minion {
        return Minion(original);  // 完全复制
    });
}

std::vector<std::string> CopyPrecisionCard::getCodeLines() const {
    return {
        "return Minion(original);"
    };
}

// 复制函数·增殖
void CopyMultiplyCard::play(Player& player, Enemy* target) {
    auto oldCopy = player.getCopySummonFunc();
    player.setCopySummonFunc([oldCopy, &player](const Minion& original) -> Minion {
        Minion copy1 = oldCopy(original);
        if (player.minions.size() < 2) {
            Minion copy2(original.name, original.hp / 2, original.attack / 2);
            player.addMinion(copy2);
        }
        return copy1;
    });
}

std::vector<std::string> CopyMultiplyCard::getCodeLines() const {
    return {
        "Minion copy1 = oldCopy(original);",
        "if (player.minions.size() < 2) {",
        "    player.addMinion(Minion(original.name, original.hp / 2, original.attack / 2));",
        "}",
        "return copy1;"
    };
}

// 复制函数·改良
void CopyImproveCard::play(Player& player, Enemy* target) {
    player.setCopySummonFunc([](const Minion& original) -> Minion {
        return Minion(original.name, original.hp * 1.2, original.attack * 1.2);
    });
}

std::vector<std::string> CopyImproveCard::getCodeLines() const {
    return {
        "return Minion(original.name, original.hp * 1.2, original.attack * 1.2);"
    };
}

// ============================================================
// 移动函数牌
// ============================================================

// 移动函数·提取
void MoveExtractCard::play(Player& player, Enemy* target) {
    auto oldMove = player.getMoveSummonFunc();
    player.setMoveSummonFunc([oldMove, &player](Minion&& minion) -> Minion {
        player.energy += minion.attack / 2;
        return oldMove(std::move(minion));
    });
}

std::vector<std::string> MoveExtractCard::getCodeLines() const {
    return {
        "player.energy += minion.attack / 2;",
        "return oldMove(std::move(minion));"
    };
}

// 移动函数·遗骸
void MoveRemnantsCard::play(Player& player, Enemy* target) {
    auto oldMove = player.getMoveSummonFunc();
    player.setMoveSummonFunc([oldMove, &player](Minion&& minion) -> Minion {
        player.addMinion(Minion("Corpse", 5, 0));
        return oldMove(std::move(minion));
    });
}

std::vector<std::string> MoveRemnantsCard::getCodeLines() const {
    return {
        "player.addMinion(Minion(\"Corpse\", 5, 0));",
        "return oldMove(std::move(minion));"
    };
}

// 移动函数·共鸣
void MoveResonanceCard::play(Player& player, Enemy* target) {
    auto oldMove = player.getMoveSummonFunc();
    player.setMoveSummonFunc([oldMove, &player](Minion&& minion) -> Minion {
        player.shield += minion.hp / 2;
        return oldMove(std::move(minion));
    });
}

std::vector<std::string> MoveResonanceCard::getCodeLines() const {
    return {
        "player.shield += minion.hp / 2;",
        "return oldMove(std::move(minion));"
    };
}

// ============================================================
// 析构函数牌
// ============================================================

// 析构函数·爆炸
void SacrificeExplodeCard::play(Player& player, Enemy* target) {
    auto oldSacrifice = player.getSacrificeFunc();
    player.setSacrificeFunc([oldSacrifice, target](Minion& minion) {
        oldSacrifice(minion);
        if (target) {
            target->takeDamage(minion.hp, DamageType::FIRE);
        }
    });
}

std::vector<std::string> SacrificeExplodeCard::getCodeLines() const {
    return {
        "oldSacrifice(minion);",
        "if (target) target->takeDamage(minion.hp, FIRE);"
    };
}

// 析构函数·传承
void SacrificeInheritCard::play(Player& player, Enemy* target) {
    auto oldSacrifice = player.getSacrificeFunc();
    player.setSacrificeFunc([oldSacrifice, &player](Minion& minion) {
        oldSacrifice(minion);
        player.baseAttack += minion.attack;
    });
}

std::vector<std::string> SacrificeInheritCard::getCodeLines() const {
    return {
        "oldSacrifice(minion);",
        "player.baseAttack += minion.attack;"
    };
}

// 析构函数·重生
void SacrificeRebornCard::play(Player& player, Enemy* target) {
    auto oldSacrifice = player.getSacrificeFunc();
    player.setSacrificeFunc([oldSacrifice, &player](Minion& minion) {
        oldSacrifice(minion);
        if (rand() % 100 < 30) {  // 30% 概率
            player.addMinion(Minion(minion.name, minion.hp / 2, minion.attack));
        }
    });
}

std::vector<std::string> SacrificeRebornCard::getCodeLines() const {
    return {
        "oldSacrifice(minion);",
        "if (rand() \% 100 < 30) {",
        "    player.addMinion(Minion(minion.name, minion.hp / 2, minion.attack));",
        "}"
    };
}

// ============================================================
// 逃跑函数牌
// ============================================================

// 逃跑函数·诡步
void EscapeNimbleCard::play(Player& player, Enemy* target) {
    player.setEscapeFunc([]() -> bool {
        return rand() % 100 < 70;  // 70% 成功率
    });
}

std::vector<std::string> EscapeNimbleCard::getCodeLines() const {
    return {
        "return (rand() \% 100) < 70;  // 70\% success rate"
    };
}

// 逃跑函数·金蝉脱壳
void EscapeMoltCard::play(Player& player, Enemy* target) {
    auto oldEscape = player.getEscapeFunc();
    player.setEscapeFunc([oldEscape, &player]() -> bool {
        bool success = oldEscape();
        if (!success) {
            // 失败时只承受 50% 伤害（下回合生效）
            Status molt;
            molt.type = StatusType::FORTIFY;
            molt.value = 50;
            molt.turnsRemaining = 1;
            player.addStatus(molt);
        }
        return success;
    });
}

std::vector<std::string> EscapeMoltCard::getCodeLines() const {
    return {
        "bool success = oldEscape();",
        "if (!success) {",
        "    Status molt(FORTIFY, 50, 1);",
        "    player.addStatus(molt);",
        "}",
        "return success;"
    };
}

// 逃跑函数·断后
void EscapeRearguardCard::play(Player& player, Enemy* target) {
    auto oldEscape = player.getEscapeFunc();
    player.setEscapeFunc([oldEscape, target, &player]() -> bool {
        bool success = oldEscape();
        if (success && target) {
            player.attack(*target, [](int atk) { return atk * 2; });
        }
        return success;
    });
}

std::vector<std::string> EscapeRearguardCard::getCodeLines() const {
    return {
        "bool success = oldEscape();",
        "if (success && target) {",
        "    player.attack(*target, [](int atk) { return atk * 2; });",
        "}",
        "return success;"
    };
}
