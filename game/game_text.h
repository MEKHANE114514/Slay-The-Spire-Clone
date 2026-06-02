#ifndef GAME_TEXT_H
#define GAME_TEXT_H

#include <string>
#include "types.h"

// ============================================================
// game_text.h — 枚举 → 中文显示名映射
// Qt 层通过调用这些函数获取中文名，无需自己维护字典
// ============================================================

// ---- 状态效果 ----
inline std::string statusName(StatusType s) {
    switch (s) {
        case StatusType::BURN:       return "灼烧";
        case StatusType::POISON:     return "中毒";
        case StatusType::FREEZE:     return "冻结";
        case StatusType::STUN:       return "眩晕";
        case StatusType::WEAKEN:     return "虚弱";
        case StatusType::VULNERABLE: return "易伤";
        case StatusType::STRENGTH:   return "力量";
        case StatusType::SHIELD:     return "护盾";
        case StatusType::REGEN:      return "再生";
        case StatusType::MARK:       return "标记";
        case StatusType::RAGE:       return "怒气";
        case StatusType::FORTIFY:    return "固守";
        case StatusType::CORRODE:    return "腐蚀";
        case StatusType::DODGE:      return "闪避";
        case StatusType::CHARGE:     return "蓄力";
        case StatusType::ECHO:       return "回响";
        default:                     return "未知";
    }
}

// ---- 伤害类型 ----
inline std::string damageTypeName(DamageType d) {
    switch (d) {
        case DamageType::PHYSICAL:  return "物理";
        case DamageType::FIRE:      return "火焰";
        case DamageType::ICE:       return "冰冻";
        case DamageType::LIGHTNING: return "雷电";
        case DamageType::SHADOW:    return "暗影";
        case DamageType::HOLY:      return "神圣";
        case DamageType::POISON:    return "毒素";
        default:                    return "未知";
    }
}

// ---- 稀有度 ----
inline std::string rarityName(Rarity r) {
    switch (r) {
        case Rarity::COMMON:    return "普通";
        case Rarity::UNCOMMON:  return "优秀";
        case Rarity::RARE:      return "稀有";
        case Rarity::LEGENDARY: return "传说";
        default:                return "未知";
    }
}

// ---- 卡牌类型 ----
inline std::string cardTypeName(CardType t) {
    switch (t) {
        case CardType::FUNCTION: return "函数牌";
        case CardType::COMMAND:  return "指令牌";
        case CardType::TEMPLATE: return "模板牌";
        default:                 return "未知";
    }
}

// ---- 函数目标 ----
inline std::string functionTargetName(FunctionTarget f) {
    switch (f) {
        case FunctionTarget::ATTACK:       return "攻击函数";
        case FunctionTarget::TAKE_DAMAGE:  return "受击函数";
        case FunctionTarget::SUMMON:       return "构造函数";
        case FunctionTarget::COPY_SUMMON:  return "复制构造";
        case FunctionTarget::MOVE_SUMMON:  return "移动构造";
        case FunctionTarget::SACRIFICE:    return "析构函数";
        case FunctionTarget::ESCAPE:       return "逃跑函数";
        default:                           return "未知";
    }
}

// ---- 实体状态 ----
inline std::string entityStateName(EntityState s) {
    switch (s) {
        case EntityState::NORMAL:      return "正常";
        case EntityState::STUNNED:     return "眩晕";
        case EntityState::FROZEN:      return "冻结";
        case EntityState::INVINCIBLE:  return "无敌";
        case EntityState::DEFENDING:   return "防御姿态";
        default:                       return "未知";
    }
}

// ---- 阵营 ----
inline std::string factionName(Faction f) {
    switch (f) {
        case Faction::NONE:   return "无";
        case Faction::ORDER:  return "秩序";
        case Faction::CHAOS:  return "混沌";
        case Faction::NATURE: return "自然";
        case Faction::VOID:   return "虚空";
        default:              return "未知";
    }
}

// ---- 仆从类型 ----
inline std::string minionTypeName(MinionType m) {
    switch (m) {
        case MinionType::NORMAL: return "普通仆从";
        case MinionType::ELITE:  return "精锐仆从";
        default:                 return "未知";
    }
}

// ---- 回合阶段 ----
inline std::string battlePhaseName(BattlePhase p) {
    switch (p) {
        case BattlePhase::DRAW:   return "抽牌阶段";
        case BattlePhase::MAIN:   return "主要阶段";
        case BattlePhase::ATTACK: return "攻击阶段";
        case BattlePhase::END:    return "结束阶段";
        default:                  return "未知";
    }
}

#endif // GAME_TEXT_H
