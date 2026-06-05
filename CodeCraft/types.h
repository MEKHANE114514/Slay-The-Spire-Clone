#ifndef TYPES_H
#define TYPES_H

// ============================================================
// types.h — CodeCraft 游戏基础类型与常量
// 全项目最底层头文件，零依赖，仅定义枚举和常量
// ============================================================

// ---- 卡牌体系 ----

// 函数牌修改的目标（对应 Player 的哪个成员函数）
enum class FunctionTarget {
    ATTACK,
    TAKE_DAMAGE,
    SUMMON,
    COPY_SUMMON,
    MOVE_SUMMON,
    SACRIFICE,
    ESCAPE
};

// 卡牌类型
enum class CardType {
    FUNCTION,   // 函数牌 — 修改 Player 成员函数
    COMMAND,    // 指令牌 — 一次性执行动作
    TEMPLATE    // 模板牌 — 包装/增强函数牌
};

// 卡牌稀有度
enum class Rarity {
    COMMON,
    UNCOMMON,
    RARE,
    LEGENDARY
};

// 目标选择模式
enum class TargetMode {
    NONE,            // 无需选择目标（如防御指令）
    SINGLE_ENEMY,    // 单个敌方
    ALL_ENEMIES,     // 全体敌方
    SINGLE_ALLY,     // 单个友方（含自己）
    ALL_ALLIES,      // 全体友方
    SELF             // 仅自身
};

// ---- 战斗相关 ----

// 伤害/元素属性类型
enum class DamageType {
    PHYSICAL,   // 物理
    FIRE,       // 火焰
    ICE,        // 冰冻
    LIGHTNING,  // 雷电
    SHADOW,     // 暗影
    HOLY,       // 神圣
    POISON      // 毒素
};

// 仆从类型
enum class MinionType {
    NORMAL,
    ELITE
};

// 状态效果类型
enum class StatusType {
    BURN,       // 灼烧 — 每回合造成固定伤害
    POISON,     // 中毒 — 每回合造成递增伤害
    FREEZE,     // 冻结 — 跳过一回合
    STUN,       // 眩晕 — 跳过一回合
    WEAKEN,     // 虚弱 — 攻击力降低
    VULNERABLE, // 易伤 — 受到的伤害增加
    STRENGTH,   // 力量 — 攻击力提升
    SHIELD,     // 护盾 — 抵消伤害
    INVINCIBLE, // 无敌 — 不受伤害
    REGEN,      // 再生 — 每回合恢复生命
    MARK,       // 标记 — 被标记的目标受额外伤害
    RAGE,       // 怒气 — 受击后攻击力提升
    FORTIFY,    // 固守 — 受击后防御叠加
    CORRODE,    // 腐蚀 — 对攻击者施加可叠加减益
    DODGE,      // 闪避 — 概率躲避攻击
    CHARGE,     // 蓄力 — 本回合不攻击，下回合伤害提升
    ECHO        // 回响 — 保留伤害记录，下次追加
};

// 阵营
enum class Faction {
    NONE,
    ORDER,
    CHAOS,
    NATURE,
    VOID
};

// 回合阶段
enum class BattlePhase {
    DRAW,       // 抽牌阶段
    MAIN,       // 主要阶段（出牌）
    ATTACK,     // 攻击结算阶段
    END         // 结束阶段（状态扣减、弃牌）
};

// ---- 游戏常量 ----

constexpr int DEFAULT_MAX_HP     = 100;
constexpr int DEFAULT_MAX_ENERGY = 1;   // 起始 1，每回合 +1
constexpr int DEFAULT_HAND_SIZE  = 5;
constexpr int MAX_HAND_SIZE      = 10;
constexpr int MAX_MINIONS        = 5;
constexpr int MAX_ENEMIES        = 5;
constexpr int DEFAULT_DRAW_PER_TURN = 5;

// 仆从基准属性
constexpr int MINION_BASE_HP   = 20;
constexpr int MINION_BASE_ATK  = 5;
constexpr int ELITE_MINION_HP  = 35;
constexpr int ELITE_MINION_ATK = 10;

// 稀有度对应的费用倍率（可在具体卡牌中覆盖）
constexpr int COST_COMMON    = 1;
constexpr int COST_UNCOMMON  = 2;
constexpr int COST_RARE      = 3;
constexpr int COST_LEGENDARY = 5;

// 每回合免费基础动作次数限制
// 攻击/召唤为 0：必须通过指令牌执行，避免函数牌无限滚雪球
// 防御保留 1 次：保证最低生存能力
struct ActionLimits {
    int attacks    = 0;   // 基础攻击：0（需指令牌）
    int defends    = 1;   // 基础防御：1（保底生存）
    int summons    = 0;   // 基础召唤：0（需指令牌）
    int sacrifices = 0;   // 献祭：需函数牌解锁
    int copies     = 0;   // 复制：需函数牌解锁
    int moves      = 0;   // 转移：需函数牌解锁
    bool escapeUsed = false; // 逃跑：整场限一次
};

#endif // TYPES_H
