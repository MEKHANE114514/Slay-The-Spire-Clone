#include "enemy.h"
#include "player.h"
#include "battle.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include "game_text.h"

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

std::vector<std::string> Enemy::getStatusesCode() const {
    std::vector<std::string> lines;
    for (const auto& s : statuses) {
        std::string effect;
        switch (s.type) {
            case StatusType::BURN:
                effect = "hp-=" + std::to_string(s.value);
                break;
            case StatusType::POISON:
                effect = "hp-=" + std::to_string(s.value) + "(↑)";
                break;
            case StatusType::REGEN:
                effect = "hp+=" + std::to_string(s.value);
                break;
            case StatusType::STRENGTH:
                effect = "atk+=" + std::to_string(s.value);
                break;
            case StatusType::WEAKEN:
                effect = "atk-=" + std::to_string(s.value);
                break;
            case StatusType::VULNERABLE:
                effect = "dmgTaken+" + std::to_string(s.value) + "%";
                break;
            case StatusType::FREEZE:
            case StatusType::STUN:
                effect = "skip";
                break;
            case StatusType::FORTIFY:
                effect = "onHit:shield+=" + std::to_string(s.value);
                break;
            case StatusType::RAGE:
                effect = "onHit:atk+=" + std::to_string(s.value);
                break;
            case StatusType::DODGE:
                effect = "dodge " + std::to_string(s.value) + "%";
                break;
            case StatusType::INVINCIBLE:
                effect = "immune";
                break;
            case StatusType::MARK:
                effect = "dmgTaken+" + std::to_string(s.value) + "%";
                break;
            default:
                effect = "?";
                break;
        }
        std::string turns = (s.turnsRemaining > 0)
            ? std::to_string(s.turnsRemaining) + "回合"
            : "永久";
        lines.push_back(effect + " //" + statusName(s.type) + "，" + turns);
    }
    return lines;
}

std::string Enemy::getEnemyIntent() const {
    switch (nextIntent.type) {
        case EnemyIntent::ATTACK:
            return "攻击" + std::to_string(nextIntent.value);
        case EnemyIntent::DEFEND:
            return "防御" + std::to_string(nextIntent.value);
        case EnemyIntent::HEAL:
            return "回复" + std::to_string(nextIntent.value);
        case EnemyIntent::BUFF:
        case EnemyIntent::SUMMON:
            return "增益";
        case EnemyIntent::NONE:
        default:
            return "等待";
    }
}

// ============================================================
// Goblin 实现
// ============================================================

void Goblin::takeTurn(Player& player) {
    // 设置意图：显示即将攻击
    setIntent(EnemyIntent::ATTACK, getEffectiveAttack());

    // 执行攻击
    if (!isDisabled()) {
        player.takeDamage(getEffectiveAttack(), DamageType::PHYSICAL);
    }
}

std::vector<std::string> Goblin::getDescription() const { return {"普通的程序猿，每回合造成 7 点物理伤害。"}; }

std::string Goblin::getEnemyIntent() const {
    return "攻击" + std::to_string(getEffectiveAttack());
}

void FireGoblin::takeTurn(Player& player) {
    turnCounter++;

    // 先处理累积的反伤
    if (reflectDamageStacks > 0) {
        int totalReflectDamage = reflectDamageStacks * 3;
        player.takeDamage(totalReflectDamage, DamageType::FIRE);
        reflectDamageStacks = 0;
    }

    // 奇数回合：火焰屏障
    if (turnCounter % 2 == 1) {
        setIntent(EnemyIntent::DEFEND, 10);

        if (!isDisabled()) {
            // 获得 10 点护盾
            shield += 10;
            if (onShieldChanged) onShieldChanged(shield, 10);
        }
    }
    // 偶数回合：燃烧
    else {
        setIntent(EnemyIntent::BUFF, 3);

        if (!isDisabled()) {
            // 给玩家施加 3 回合灼烧
            Status burn;
            burn.type = StatusType::BURN;
            burn.value = 3;
            burn.turnsRemaining = 3;
            player.addStatus(burn);
        }
    }

    // 每回合都进行攻击
    if (!isDisabled()) {
        player.takeDamage(getEffectiveAttack(), DamageType::FIRE);
    }
}

void FireGoblin::takeDamage(int dmg, DamageType dtype) {
    // 如果有护盾且受到攻击，记录反伤
    if (shield > 0 && dmg > 0) {
        reflectDamageStacks++;
    }

    // 调用基类的受伤逻辑
    Enemy::takeDamage(dmg, dtype);
}

std::vector<std::string> FireGoblin::getDescription() const {
    return {
        "内心火热的程序猿，每回合造成 12 点火属性伤害。",
        "【火焰屏障】奇数回合获得 10 点护盾，攻击有护盾的炽热程序猿会在其下回合受到 3 点火焰反伤",
        "【燃烧】偶数回合使玩家获得 3 回合灼烧状态（每回合 3 点伤害）"
    };
}

std::string FireGoblin::getEnemyIntent() const {
    int nextTurn = turnCounter + 1;
    std::string intent = "攻击" + std::to_string(getEffectiveAttack());

    if (nextTurn % 2 == 1) {
        // 奇数回合：防御
        intent += " + 防御10";
    } else {
        // 偶数回合：增益（燃烧）
        intent += " + 增益";
    }

    return intent;
}

void FrozenGoblin::takeTurn(Player& player) {
    turnCounter++;

    // 先处理累积的能量削弱
    if (energyDebuffStacks > 0) {
        int energyReduction = energyDebuffStacks;
        player.maxEnergy = std::max(1, player.maxEnergy - energyReduction);
        energyDebuffStacks = 0;
    }

    // 奇数回合：寒冰屏障
    if (turnCounter % 2 == 1) {
        setIntent(EnemyIntent::DEFEND, 20);

        if (!isDisabled()) {
            // 获得 20 点护盾
            shield += 20;
            if (onShieldChanged) onShieldChanged(shield, 20);
        }
    }
    // 偶数回合：冰冻
    else {
        setIntent(EnemyIntent::BUFF, 1);

        if (!isDisabled()) {
            // 给玩家施加 1 回合虚弱
            Status weaken;
            weaken.type = StatusType::WEAKEN;
            weaken.value = 3;
            weaken.turnsRemaining = 1;
            player.addStatus(weaken);

            // 50% 概率施加 1 回合冰冻
            if ((rand() % 100) < 50) {
                Status freeze;
                freeze.type = StatusType::FREEZE;
                freeze.value = 1;
                freeze.turnsRemaining = 1;
                player.addStatus(freeze);
            }
        }
    }

    // 每回合都进行攻击
    if (!isDisabled()) {
        player.takeDamage(getEffectiveAttack(), DamageType::ICE);
    }
}

void FrozenGoblin::takeDamage(int dmg, DamageType dtype) {
    // 如果有护盾且受到攻击，记录能量削弱
    if (shield > 0 && dmg > 0) {
        energyDebuffStacks++;
    }

    // 调用基类的受伤逻辑
    Enemy::takeDamage(dmg, dtype);
}

std::vector<std::string> FrozenGoblin::getDescription() const {
    return {
        "内心冰冷的程序猿，每回合造成 12 点冰属性伤害。",
        "【寒冰屏障】奇数回合获得 20 点护盾",
        "【冰冻】偶数回合使玩家获得 1 回合虚弱（攻击力-3），并有 50% 概率获得 1 回合冰冻状态"
    };
}

std::string FrozenGoblin::getEnemyIntent() const {
    int nextTurn = turnCounter + 1;
    std::string intent = "攻击" + std::to_string(getEffectiveAttack());

    if (nextTurn % 2 == 1) {
        // 奇数回合：防御
        intent += " + 防御20";
    } else {
        // 偶数回合：增益（虚弱 + 50%冰冻）
        intent += " + 增益";
    }

    return intent;
}

void Caster::takeTurn(Player& player) {
    int actionChoice = rand() % 2;
    if (actionChoice == 0) {
        int damageTypeChoice = rand() % 3;
        DamageType dmgType;
        switch (damageTypeChoice) {
            case 0: dmgType = DamageType::PHYSICAL; break;
            case 1: dmgType = DamageType::FIRE; break;
            case 2: dmgType = DamageType::ICE; break;
            default: dmgType = DamageType::PHYSICAL; break;
        }
        setIntent(EnemyIntent::ATTACK, getEffectiveAttack());
        if (!isDisabled()) {
            player.takeDamage(getEffectiveAttack(), dmgType);
        }
    } else {
        bool needsHealing = (hp < maxHp / 3);
        if (needsHealing) {
            setIntent(EnemyIntent::HEAL, 5);
            if (!isDisabled()) {
                Status regen;
                regen.type = StatusType::REGEN;
                regen.value = 5;
                regen.turnsRemaining = 2;
                addStatus(regen);
            }
        } else {
            setIntent(EnemyIntent::BUFF, 4);
            if (!isDisabled()) {
                Status strength;
                strength.type = StatusType::STRENGTH;
                strength.value = 4;
                strength.turnsRemaining = -1;  // 永久
                addStatus(strength);
            }
        }
    }
}

std::vector<std::string> Caster::getDescription() const {
    return {
        "经过 30 年的修炼变成魔法师的程序猿。",
        "每回合有 0.5 的概率进行一次随机属性的攻击，造成 3 点基础伤害",
        "若不进行攻击，则会释放一次 ⌈魔法⌋：",
        "   - 如果己方有单位血量低于最大血量的 1/3，则释放回复魔法，为全体提供两回合的再生",
        "   - 否则释放强化魔法，为己方永久增加 4 点力量"
    };
}

std::string Caster::getEnemyIntent() const {
    bool needsHealing = (hp < maxHp / 3);

    if (needsHealing) {
        // 血量低时必定回复
        return "50% 攻击" + std::to_string(getEffectiveAttack()) + " + 50% 回复5";
    } else {
        // 血量正常时增益或攻击
        return "50% 攻击" + std::to_string(getEffectiveAttack()) + " + 50% 增益";
    }
}

// ============================================================
// TemplateKing（模板魔王）实现
// ============================================================

void TemplateKing::checkPhaseTransition() {
    float hpPercent = static_cast<float>(hp) / maxHp;
    Phase newPhase = currentPhase;

    if (hpPercent <= 0.33f && currentPhase != Phase::THIRD) {
        newPhase = Phase::THIRD;
    } else if (hpPercent <= 0.66f && currentPhase == Phase::FIRST) {
        newPhase = Phase::SECOND;
    }

    if (newPhase != currentPhase) {
        currentPhase = newPhase;
        turnsInPhase = 0;
        // 阶段转换时完全恢复护盾
        shield = 30 + static_cast<int>(currentPhase) * 20;
        if (onShieldChanged) onShieldChanged(shield, shield);
    }
}

std::vector<std::string> TemplateKing::getDescription() const {
    return {
        "上古时期就开始修炼的神秘程序猿，传说参与过传奇程序 ⌈猿神⌋ 的开发。",
        "【多阶段机制】生命值降至 66% 和 33% 时进入新阶段，进入新阶段后恢复护盾",
        "【模式切换】每 3 回合在 ⌈攻击模式⌋ 和 ⌈防御模式⌋ 之间切换",
        "    -【攻击模式】复制玩家攻击力的 50% 作为两回合力量增益，然后进行一次基础伤害为 10 的攻击",
        "    -【防御模式】每回合生成护盾，护盾量随阶段递增（一阶段 100 / 二阶段 150 / 三阶段 200），并回复少量生命值",
        "【终结技】第三阶段时每 2 回合释放一次两倍攻击力的 AOE 攻击"
    };
}

void TemplateKing::switchMode() {
    currentMode = (currentMode == Mode::ATTACK) ? Mode::DEFENSE : Mode::ATTACK;
    turnsSinceLastSwitch = 0;
}

int TemplateKing::getShieldAmount() const {
    switch (currentPhase) {
        case Phase::FIRST:  return 50;
        case Phase::SECOND: return 100;
        case Phase::THIRD:  return 150;
        default:            return 50;
    }
}

void TemplateKing::attackMode(Player& player) {
    // 复制玩家的攻击力作为 STRENGTH
    int playerAtk = player.getEffectiveAttack();

    setIntent(EnemyIntent::ATTACK, getEffectiveAttack());

    if (!isDisabled()) {
        // 先获得玩家攻击力的 STRENGTH
        Status copyStrength;
        copyStrength.type = StatusType::STRENGTH;
        copyStrength.value = playerAtk / 2;  // 复制 50% 的玩家攻击力
        copyStrength.turnsRemaining = -1;    // 永久
        addStatus(copyStrength);

        // 然后攻击
        player.takeDamage(getEffectiveAttack(), DamageType::PHYSICAL);
    }
}

void TemplateKing::defenseMode(Player& player) {
    setIntent(EnemyIntent::DEFEND, getShieldAmount());

    if (!isDisabled()) {
        // 生成护盾
        shield += getShieldAmount();
        if (onShieldChanged) onShieldChanged(shield, getShieldAmount());

        // 同时恢复少量生命
        int healAmount = 5 + static_cast<int>(currentPhase) * 3;
        heal(healAmount);
    }
}

void TemplateKing::ultimateAttack(Player& player) {
    setIntent(EnemyIntent::ATTACK, getEffectiveAttack() * 2);

    if (!isDisabled()) {
        int ultimateDamage = getEffectiveAttack() * 2;

        // 对玩家造成伤害（使用雷电伤害代表强力攻击）
        player.takeDamage(ultimateDamage, DamageType::LIGHTNING);

        // 对所有仆从造成伤害
        for (auto& minion : player.minions) {
            minion.takeDamage(ultimateDamage / 2, DamageType::LIGHTNING);
        }
    }
}

void TemplateKing::takeTurn(Player& player) {
    // 检查阶段转换
    checkPhaseTransition();

    turnsInPhase++;
    turnsSinceLastSwitch++;

    // 第三阶段每 4 回合释放终极技
    if (currentPhase == Phase::THIRD && turnsInPhase % 2 == 0) {
        ultimateAttack(player);
        return;
    }

    // 每 3 回合切换模式
    if (turnsSinceLastSwitch >= 3) {
        switchMode();
    }

    // 根据当前模式执行行动
    if (currentMode == Mode::ATTACK) {
        attackMode(player);
    } else {
        defenseMode(player);
    }
}

std::string TemplateKing::getEnemyIntent() const {
    // 检查下回合是否触发终极技
    int nextTurnsInPhase = turnsInPhase + 1;
    if (currentPhase == Phase::THIRD && nextTurnsInPhase % 2 == 0) {
        return "攻击" + std::to_string(getEffectiveAttack() * 2);
    }

    // 检查是否即将切换模式
    int nextTurnsSinceSwitch = turnsSinceLastSwitch + 1;
    Mode nextMode = currentMode;
    if (nextTurnsSinceSwitch >= 3) {
        nextMode = (currentMode == Mode::ATTACK) ? Mode::DEFENSE : Mode::ATTACK;
    }

    if (nextMode == Mode::ATTACK) {
        return "攻击" + std::to_string(getEffectiveAttack()) + " + 增益";
    } else {
        return "防御" + std::to_string(getShieldAmount());
    }
}

// ============================================================
// ExceptionLord（异常魔王）实现
// ============================================================

void ExceptionLord::gainException(int count) {
    exceptionCount += count;
    // 异常计数 >= 5 时激活状态免疫
}

void ExceptionLord::takeDamage(int dmg, DamageType dtype) {
    // Try-Catch 机制：如果激活且即将致命，捕获伤害
    if (tryCatchActive && hp - dmg <= 0) {
        // 捕获致命伤害，恢复到 30% 生命
        hp = maxHp * 30 / 100;
        tryCatchActive = false;

        // 清除所有负面状态
        auto it = statuses.begin();
        while (it != statuses.end()) {
            if (it->type == StatusType::POISON ||
                it->type == StatusType::BURN ||
                it->type == StatusType::WEAKEN) {
                if (onStatusRemoved) onStatusRemoved(it->type);
                it = statuses.erase(it);
            } else {
                ++it;
            }
        }

        if (onHpChanged) onHpChanged(hp, maxHp, hp);
        return;
    }

    // 正常受伤流程
    int oldHp = hp;
    int oldShield = shield;

    if (shield > 0) {
        int blocked = shield < dmg ? shield : dmg;
        shield -= blocked;
        dmg -= blocked;
    }

    hp -= dmg;

    // 每次受伤累积异常计数
    if (dmg > 0) {
        gainException(1);
    }

    if (onShieldChanged && shield != oldShield) onShieldChanged(shield, shield - oldShield);
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp, hp - oldHp);

    // 检查是否触发 Try-Catch
    if (hp > 0 && hp < maxHp * 25 / 100 && !tryCatchActive) {
        activateTryCatch();
    }

    if (!isAlive() && onDeath) onDeath();
}

std::vector<std::string> ExceptionLord::getDescription() const {
    return {
        "传说曾是程序猿神开发的程序，因为位置的原因陷入了崩坏。",
        "每回合释放一次基础伤害为 8 的普通攻击，每次攻击/受击都会累计一次 ⌈异常计数⌋",
        "  -【Throw 攻击】异常计数 >= 3 时，有 60% 概率消耗所有异常计数，造成 (1 + 异常计数) 倍伤害",
        "每 5 回合释放一次 ⌈异常链⌋，⌈异常链⌋ 的攻击次数等于当前异常计数（最多 5 次）",
        "【Try-Catch】生命值低于 25% 时激活保护，下一次致命伤害会被捕获，恢复至 30% 生命并清除负面状态",
        "【状态免疫】异常计数 >= 5 时免疫所有负面状态",
        "【Finally】死亡时对玩家造成 20 点暗影真实伤害"
    };
}

void ExceptionLord::addStatus(Status s) {
    // 异常计数 >= 5 时免疫负面状态
    if (exceptionCount >= 5) {
        if (s.type == StatusType::POISON ||
            s.type == StatusType::BURN ||
            s.type == StatusType::WEAKEN ||
            s.type == StatusType::STUN) {
            return;  // 免疫负面状态
        }
    }

    // 否则正常添加状态
    statuses.push_back(s);
    if (onStatusAdded) onStatusAdded(s);
}

bool ExceptionLord::isAlive() const {
    return hp > 0;
}

void ExceptionLord::activateTryCatch() {
    tryCatchActive = true;

    // 获得临时护盾
    shield += 20;
    if (onShieldChanged) onShieldChanged(shield, 20);
}

void ExceptionLord::throwAttack(Player& player) {
    if (exceptionCount <= 0) return;

    // 消耗异常计数，造成成倍伤害
    int multiplier = exceptionCount;
    int damage = getEffectiveAttack() * (1 + multiplier);

    setIntent(EnemyIntent::ATTACK, damage);

    if (!isDisabled()) {
        player.takeDamage(damage, DamageType::FIRE);
        exceptionCount = 0;  // 消耗所有异常计数
    }
}

void ExceptionLord::chainAttack(Player& player) {
    int attackCount = exceptionCount > 0 ? exceptionCount : 1;
    attackCount = attackCount > 5 ? 5 : attackCount;  // 最多5次

    setIntent(EnemyIntent::ATTACK, getEffectiveAttack() * attackCount);

    if (!isDisabled()) {
        for (int i = 0; i < attackCount; ++i) {
            player.takeDamage(getEffectiveAttack(), DamageType::ICE);
        }
    }
}

void ExceptionLord::finallyEffect(Player& player) {
    // Finally 效果：死亡时造成暗影伤害（代表无法减免的诅咒）
    int finalDamage = 20;
    player.takeDamage(finalDamage, DamageType::SHADOW);
}

void ExceptionLord::takeTurn(Player& player) {
    turnCounter++;

    // 每 5 回合释放异常链
    if (turnCounter % 5 == 0) {
        chainAttack(player);
        return;
    }

    // 决策：如果异常计数 >= 3，有 60% 概率使用 Throw 攻击
    if (exceptionCount >= 3 && (rand() % 100) < 60) {
        throwAttack(player);
        return;
    }

    // 普通攻击并累积异常计数
    setIntent(EnemyIntent::ATTACK, getEffectiveAttack());

    if (!isDisabled()) {
        player.takeDamage(getEffectiveAttack(), DamageType::PHYSICAL);
        gainException(1);  // 主动攻击也会积累异常
    }
}

std::string ExceptionLord::getEnemyIntent() const {
    int nextTurn = turnCounter + 1;

    // 每 5 回合释放异常链
    if (nextTurn % 5 == 0) {
        int attackCount = exceptionCount > 0 ? exceptionCount : 1;
        attackCount = attackCount > 5 ? 5 : attackCount;
        return "攻击" + std::to_string(getEffectiveAttack() * attackCount);
    }

    // 异常计数 >= 3 时，60% 概率使用 Throw 攻击
    if (exceptionCount >= 3) {
        int throwDamage = getEffectiveAttack() * (1 + exceptionCount);
        int normalDamage = getEffectiveAttack();
        return "60% 攻击" + std::to_string(throwDamage) + " + 40% 攻击" + std::to_string(normalDamage);
    }

    // 普通攻击
    return "攻击" + std::to_string(getEffectiveAttack());
}
