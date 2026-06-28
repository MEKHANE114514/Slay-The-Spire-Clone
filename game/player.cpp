#include "player.h"
#include "enemy.h"       // initDefaultFunctions 中调用 target.takeDamage()
#include "game_text.h"   // statusName()
#include <algorithm>     // std::min, std::remove_if, std::any_of
#include <cstdlib>       // rand

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
    // ---- 默认攻击：effectiveAtk 经 modifier 处理后造成伤害 ----
    attackImpl = [this](Enemy& target, std::function<int(int)> modifier) {
        int baseDmg = getEffectiveAttack();
        int dmg = modifier ? modifier(baseDmg) : baseDmg;
        if (dmg > 0) target.takeDamage(dmg, DamageType::PHYSICAL);
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

// ============================================================
// 第 2 组：7 个对外转发函数
// 主动行动：检查 isDisabled()（眩晕/冻结拦截）
// 被动响应：检查 INVINCIBLE（无敌拦截）
// summon / copySummon / moveSummon：自动 addMinion 入场
// 所有转发函数在值变化后触发 UI 回调
// ============================================================

void Player::attack(Enemy& target, std::function<int(int)> damageModifier) {
    if (isDisabled()) return;
    attackImpl(target, std::move(damageModifier));
}

void Player::takeDamage(int dmg, DamageType type) {
    if (hasStatus(StatusType::INVINCIBLE)) return;
    int oldHp = hp;
    int oldShield = shield;
    takeDamageImpl(dmg, type);
    // 通知 Qt：生命/护盾变化 + 受击飘字
    if (onDamageReceived && hp < oldHp) onDamageReceived(oldHp - hp, type);
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp, hp - oldHp);
    if (onShieldChanged && shield != oldShield) onShieldChanged(shield, shield - oldShield);
    // 触发死亡回调（仅播放动画，不真正结束游戏）
    if (!isAlive() && onDeath) onDeath();
}

Minion Player::summon() {
    if (isDisabled()) return {};
    Minion m = summonImpl();
    addMinion(m);
    return m;
}

Minion Player::copySummon(const Minion& original) {
    if (isDisabled()) return {};
    Minion m = copySummonImpl(original);
    addMinion(m);
    return m;
}

Minion Player::moveSummon(Minion& original) {
    if (isDisabled()) return {};

    // 在 minions 中找到原仆从
    auto it = std::find_if(minions.begin(), minions.end(),
        [&original](const Minion& m) { return &m == &original; });
    if (it == minions.end()) return {};

    // ⚠️ 先保存 onDeath，再移动——std::move 会把 std::function 移空
    auto deathCallback = std::move(it->onDeath);

    // 移走资源，创建新仆从
    Minion moved = moveSummonImpl(std::move(*it));

    // 触发原仆从死亡清理（Qt 动画 + 从 vector 移除）
    if (deathCallback) deathCallback();

    // 新仆从入场
    addMinion(std::move(moved));
    return minions.back();
}

void Player::sacrifice(Minion& m) {
    if (isDisabled()) return;
    sacrificeImpl(m);
    // 献祭可能直接设置 hp=0，不走 takeDamage，手动触发清理
    if (!m.isAlive() && m.onDeath) m.onDeath();
}

bool Player::tryEscape() {
    if (isDisabled()) return false;
    return escapeImpl();
}

// ============================================================
// 第 3 组：基础动作（受 ActionLimits 限制）
// ============================================================

void Player::basicAttack(Enemy& target) {
    if (!canBasicAttack()) return;
    actions.attacks--;
    attack(target);
}

void Player::basicDefend() {
    if (!canBasicDefend()) return;
    actions.defends--;
    addShield(5);  // 防御姿态：获得临时护盾
}

void Player::basicSummon() {
    if (!canBasicSummon()) return;
    actions.summons--;
    summon();  // 转发函数已处理 addMinion + 回调
}

// ============================================================
// 第 4 组：状态管理
// ============================================================

void Player::heal(int amount) {
    int oldHp = hp;
    hp = std::min(hp + amount, maxHp);
    int healed = hp - oldHp;
    if (onHealed && healed > 0) onHealed(healed);
    if (onHpChanged && healed > 0) onHpChanged(hp, maxHp, healed);
}

void Player::addShield(int amount) {
    int oldShield = shield;
    shield += amount;
    if (onShieldChanged) onShieldChanged(shield, amount);
}

void Player::addStatus(Status s) {
    statuses.push_back(s);
    if (onStatusAdded) onStatusAdded(s);
}

void Player::removeStatus(StatusType type) {
    auto it = std::remove_if(statuses.begin(), statuses.end(),
        [type](const Status& s) { return s.type == type; });
    if (it != statuses.end()) {
        if (onStatusRemoved) onStatusRemoved(type);
    }
    statuses.erase(it, statuses.end());
}

bool Player::hasStatus(StatusType type) const {
    return std::any_of(statuses.begin(), statuses.end(),
        [type](const Status& s) { return s.type == type; });
}

void Player::tickStatuses() {
    int oldHp = hp;

    for (auto& s : statuses) {
        switch (s.type) {
            case StatusType::BURN:    hp -= s.value;            break;
            case StatusType::POISON:  hp -= s.value; s.value++; break;
            case StatusType::REGEN:   heal(s.value);            break;
            case StatusType::SHIELD:  /* 护盾不自动消失 */      break;
            // 其他状态仅倒计时，不触发每回合效果
            default: break;
        }
        if (s.turnsRemaining > 0) s.turnsRemaining--;
    }

    // 清除到期状态（到期前通知 Qt）
    auto it = std::remove_if(statuses.begin(), statuses.end(),
        [this](const Status& s) {
            if (s.turnsRemaining == 0 && s.type != StatusType::SHIELD) {
                if (onStatusRemoved) onStatusRemoved(s.type);
                return true;
            }
            return false;
        });
    statuses.erase(it, statuses.end());

    // 生命变化通知
    if (onHpChanged && hp != oldHp) onHpChanged(hp, maxHp, hp - oldHp);
}

std::vector<std::string> Player::getStatusesCode() const {
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

void Player::resetActionLimits() {
    actions = ActionLimits{};
}

// ============================================================
// 第 5 组：仆从管理
// ============================================================

bool Player::addMinion(Minion m) {
    if (minions.size() >= MAX_MINIONS) return false;
    minions.push_back(std::move(m));
    Minion* ptr = &minions.back();
    int index = static_cast<int>(minions.size()) - 1;

    // ① 先让 Qt 绑定自己的 onDeath（动画）
    if (onMinionAdded) onMinionAdded(index);

    // ② 包装 onDeath：先 Qt 动画 → 再通知移除 → 从 vector 清除
    auto oldOnDeath = std::move(ptr->onDeath);
    ptr->onDeath = [this, ptr, oldDeath = std::move(oldOnDeath)]() {
        if (oldDeath) oldDeath();           // Qt 死亡动画
        auto it = std::find_if(minions.begin(), minions.end(),
            [ptr](const Minion& m) { return &m == ptr; });
        if (it != minions.end()) {
            minions.erase(it);
        }
    };

    return true;
}

// ============================================================
// 第 6 组：查询
// ============================================================

bool Player::isDisabled() const {
    return hasStatus(StatusType::FREEZE) || hasStatus(StatusType::STUN);
}

int Player::getEffectiveAttack() const {
    int atk = baseAttack;
    for (auto& s : statuses) {
        if (s.type == StatusType::STRENGTH) atk += s.value;
        if (s.type == StatusType::WEAKEN)   atk -= s.value;
    }
    return atk > 0 ? atk : 0;
}
