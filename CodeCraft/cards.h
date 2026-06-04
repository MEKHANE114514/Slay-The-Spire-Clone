#ifndef CARDS_H
#define CARDS_H

#include <string>
#include <functional>
#include <memory>
#include "types.h"

// ============================================================
// cards.h — 卡牌系统
// 三大卡牌体系：函数牌/指令牌/模板牌
// 核心理念：函数即卡牌，对象即棋子，调用即出牌
// ============================================================

class Player;
class Enemy;

// ============================================================
// 基础卡牌抽象类
// ============================================================

class Card {
public:
    std::string name;
    std::string description;
    int cost;
    CardType type;
    Rarity rarity;
    TargetMode targetMode;

    Card(std::string n, std::string desc, int c, CardType t, Rarity r, TargetMode tm)
        : name(std::move(n))
        , description(std::move(desc))
        , cost(c)
        , type(t)
        , rarity(r)
        , targetMode(tm) {}

    virtual ~Card() = default;

    virtual void play(Player& player, Enemy* target = nullptr) = 0;
    virtual bool canPlay(const Player& player) const;
    virtual Card* clone() const = 0;
};

// ============================================================
// 函数牌基类
// ============================================================

class FunctionCard : public Card {
public:
    FunctionTarget target;

    FunctionCard(std::string n, std::string desc, int c, Rarity r,
                 FunctionTarget ft, TargetMode tm = TargetMode::NONE)
        : Card(std::move(n), std::move(desc), c, CardType::FUNCTION, r, tm)
        , target(ft) {}

    virtual void play(Player& player, Enemy* enemy = nullptr) = 0;

protected:
    static std::string getFunctionName(FunctionTarget ft);
};

// ============================================================
// 指令牌基类
// ============================================================

class CommandCard : public Card {
public:
    CommandCard(std::string n, std::string desc, int c, Rarity r, TargetMode tm)
        : Card(std::move(n), std::move(desc), c, CardType::COMMAND, r, tm) {}

    virtual void play(Player& player, Enemy* target = nullptr) = 0;
};

// ============================================================
// 模板牌基类
// ============================================================

class TemplateCard : public Card {
public:
    TemplateCard(std::string n, std::string desc, int c, Rarity r)
        : Card(std::move(n), std::move(desc), c, CardType::TEMPLATE, r, TargetMode::NONE)
        , wrappedCard(nullptr) {}

    virtual ~TemplateCard() { delete wrappedCard; }

    void setWrappedCard(FunctionCard* card) {
        delete wrappedCard;
        wrappedCard = card;
    }

    void play(Player& player, Enemy* target = nullptr) override;
    virtual void applyWrapper(Player& player, Enemy* target) = 0;
    Card* clone() const override;

protected:
    FunctionCard* wrappedCard;
    virtual TemplateCard* cloneTemplate() const = 0;
};

// ============================================================
// 攻击函数牌（覆盖各种机制）
// ============================================================

// 攻击函数·强化
class AttackEnhanceCard : public FunctionCard {
public:
    AttackEnhanceCard() : FunctionCard("攻击函数·强化", "攻击伤害提升 50%",
        2, Rarity::COMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackEnhanceCard(); }
};

// 攻击函数·吸血
class VampireAttackCard : public FunctionCard {
public:
    VampireAttackCard() : FunctionCard("攻击函数·吸血", "攻击时恢复造成伤害 30% 的生命",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new VampireAttackCard(); }
};

// 攻击函数·连击
class ComboAttackCard : public FunctionCard {
public:
    ComboAttackCard() : FunctionCard("攻击函数·连击", "攻击两次，第二次造成 50% 伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ComboAttackCard(); }
};

// 攻击函数·暴击
class CritAttackCard : public FunctionCard {
public:
    CritAttackCard() : FunctionCard("攻击函数·暴击", "30% 概率造成双倍伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new CritAttackCard(); }
};

// 攻击函数·毒击
class PoisonAttackCard : public FunctionCard {
public:
    PoisonAttackCard() : FunctionCard("攻击函数·毒击", "攻击附加 3 层中毒",
        2, Rarity::COMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new PoisonAttackCard(); }
};

// 攻击函数·灼烧
class BurnAttackCard : public FunctionCard {
public:
    BurnAttackCard() : FunctionCard("攻击函数·灼烧", "攻击附加 5 点灼烧伤害，持续 3 回合",
        2, Rarity::COMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new BurnAttackCard(); }
};

// 攻击函数·斩杀
class ExecuteAttackCard : public FunctionCard {
public:
    ExecuteAttackCard() : FunctionCard("攻击函数·斩杀", "对生命低于 30% 的目标造成三倍伤害",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ExecuteAttackCard(); }
};

// 攻击函数·连携
class SynergyAttackCard : public FunctionCard {
public:
    SynergyAttackCard() : FunctionCard("攻击函数·连携", "每个己方仆从使攻击 +3",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SynergyAttackCard(); }
};

// 攻击函数·狂暴
class BerserkerAttackCard : public FunctionCard {
public:
    BerserkerAttackCard() : FunctionCard("攻击函数·狂暴", "每损失 10 点生命，攻击 +5",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new BerserkerAttackCard(); }
};

// 攻击函数·标记
class MarkAttackCard : public FunctionCard {
public:
    MarkAttackCard() : FunctionCard("攻击函数·标记", "攻击给目标打上标记，友方攻击标记目标增伤 50%",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new MarkAttackCard(); }
};

// ============================================================
// 受击函数牌（覆盖防御机制）
// ============================================================

// 受击函数·铁壁
class IronWallCard : public FunctionCard {
public:
    IronWallCard() : FunctionCard("受击函数·铁壁", "减免 5 点伤害",
        2, Rarity::COMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new IronWallCard(); }
};

// 受击函数·反伤
class CounterDamageCard : public FunctionCard {
public:
    CounterDamageCard() : FunctionCard("受击函数·反伤", "将 50% 伤害反弹给攻击者",
        3, Rarity::RARE, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new CounterDamageCard(); }
};

// 受击函数·回春
class RegenerationCard : public FunctionCard {
public:
    RegenerationCard() : FunctionCard("受击函数·回春", "受击后恢复 3 点生命",
        1, Rarity::COMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new RegenerationCard(); }
};

// 受击函数·闪避
class DodgeCard : public FunctionCard {
public:
    DodgeCard() : FunctionCard("受击函数·闪避", "30% 概率完全躲避攻击",
        2, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DodgeCard(); }
};

// 受击函数·荆棘
class ThornsCard : public FunctionCard {
public:
    ThornsCard() : FunctionCard("受击函数·荆棘", "每次受击对攻击者造成 3 点反伤",
        2, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ThornsCard(); }
};

// 受击函数·怒气
class RageCard : public FunctionCard {
public:
    RageCard() : FunctionCard("受击函数·怒气", "受击后下一次攻击 +5",
        1, Rarity::COMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new RageCard(); }
};

// 受击函数·固守
class FortifyCard : public FunctionCard {
public:
    FortifyCard() : FunctionCard("受击函数·固守", "受击后获得 2 点护盾（可叠加）",
        2, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new FortifyCard(); }
};

// ============================================================
// 召唤函数牌
// ============================================================

// 召唤函数·强化
class EnhancedSummonCard : public FunctionCard {
public:
    EnhancedSummonCard() : FunctionCard("召唤函数·强化", "召唤仆从属性提升 50%",
        2, Rarity::COMMON, FunctionTarget::SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new EnhancedSummonCard(); }
};

// 召唤函数·精英
class EliteSummonCard : public FunctionCard {
public:
    EliteSummonCard() : FunctionCard("召唤函数·精英", "召唤精英仆从（HP 35, ATK 10）",
        3, Rarity::RARE, FunctionTarget::SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new EliteSummonCard(); }
};

// 召唤函数·量产
class MassProductionCard : public FunctionCard {
public:
    MassProductionCard() : FunctionCard("召唤函数·量产", "一次召唤 3 个弱化仆从（HP 10, ATK 3）",
        3, Rarity::UNCOMMON, FunctionTarget::SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new MassProductionCard(); }
};

// ============================================================
// 复制构造函数牌
// ============================================================

// 复制构造·精准
class PreciseCopyCard : public FunctionCard {
public:
    PreciseCopyCard() : FunctionCard("复制构造·精准", "复制时保留 90% 属性",
        2, Rarity::UNCOMMON, FunctionTarget::COPY_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new PreciseCopyCard(); }
};

// 复制构造·增殖
class ProliferateCopyCard : public FunctionCard {
public:
    ProliferateCopyCard() : FunctionCard("复制构造·增殖", "复制时额外产生次级复制品",
        3, Rarity::RARE, FunctionTarget::COPY_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ProliferateCopyCard(); }
};

// ============================================================
// 移动构造函数牌
// ============================================================

// 移动构造·榨取
class ExtractMoveCard : public FunctionCard {
public:
    ExtractMoveCard() : FunctionCard("移动构造·榨取", "移动后获得 2 点能量",
        2, Rarity::UNCOMMON, FunctionTarget::MOVE_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ExtractMoveCard(); }
};

// 移动构造·遗骸
class RemainsMoveCard : public FunctionCard {
public:
    RemainsMoveCard() : FunctionCard("移动构造·遗骸", "移动后原地留下残骸（HP 5）",
        1, Rarity::COMMON, FunctionTarget::MOVE_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new RemainsMoveCard(); }
};

// ============================================================
// 析构函数牌
// ============================================================

// 析构函数·爆裂
class ExplodeSacrificeCard : public FunctionCard {
public:
    ExplodeSacrificeCard() : FunctionCard("析构函数·爆裂", "献祭时对敌方全体造成基于仆从生命的伤害",
        2, Rarity::UNCOMMON, FunctionTarget::SACRIFICE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ExplodeSacrificeCard(); }
};

// 析构函数·传承
class InheritSacrificeCard : public FunctionCard {
public:
    InheritSacrificeCard() : FunctionCard("析构函数·传承", "献祭时将仆从的力量状态转移给玩家",
        2, Rarity::UNCOMMON, FunctionTarget::SACRIFICE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new InheritSacrificeCard(); }
};

// 析构函数·重生
class RebirthSacrificeCard : public FunctionCard {
public:
    RebirthSacrificeCard() : FunctionCard("析构函数·重生", "献祭时 50% 概率不销毁，恢复 50% 生命",
        1, Rarity::COMMON, FunctionTarget::SACRIFICE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new RebirthSacrificeCard(); }
};

// ============================================================
// 逃跑函数牌
// ============================================================

// 逃跑函数·诡步
class CunningEscapeCard : public FunctionCard {
public:
    CunningEscapeCard() : FunctionCard("逃跑函数·诡步", "逃跑成功率提升至 80%",
        1, Rarity::COMMON, FunctionTarget::ESCAPE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new CunningEscapeCard(); }
};

// 逃跑函数·金蝉脱壳
class EmergencyEscapeCard : public FunctionCard {
public:
    EmergencyEscapeCard() : FunctionCard("逃跑函数·金蝉脱壳", "逃跑失败时仅承受 50% 伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ESCAPE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new EmergencyEscapeCard(); }
};

// 逃跑函数·断后
class RearguardEscapeCard : public FunctionCard {
public:
    RearguardEscapeCard() : FunctionCard("逃跑函数·断后", "逃跑成功时对敌方造成 20 点伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ESCAPE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new RearguardEscapeCard(); }
};

// ============================================================
// 指令牌 - 攻击类
// ============================================================

// 全力一击
class PowerStrikeCard : public CommandCard {
public:
    PowerStrikeCard() : CommandCard("全力一击", "造成 2 倍攻击力的伤害",
        2, Rarity::COMMON, TargetMode::SINGLE_ENEMY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new PowerStrikeCard(); }
};

// 横扫
class SweepCard : public CommandCard {
public:
    SweepCard() : CommandCard("横扫", "攻击全体敌方（伤害降低至 70%）",
        3, Rarity::UNCOMMON, TargetMode::ALL_ENEMIES) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SweepCard(); }
};

// 连斩
class DoubleSlashCard : public CommandCard {
public:
    DoubleSlashCard() : CommandCard("连斩", "连续执行两次攻击",
        3, Rarity::UNCOMMON, TargetMode::SINGLE_ENEMY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DoubleSlashCard(); }
};

// 致命打击
class LethalStrikeCard : public CommandCard {
public:
    LethalStrikeCard() : CommandCard("致命打击", "攻击，若击败目标则恢复 2 点能量",
        3, Rarity::RARE, TargetMode::SINGLE_ENEMY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new LethalStrikeCard(); }
};

// ============================================================
// 指令牌 - 防御类
// ============================================================

// 防御姿态
class DefendCard : public CommandCard {
public:
    DefendCard() : CommandCard("防御姿态", "获得 10 点护盾",
        1, Rarity::COMMON, TargetMode::SELF) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendCard(); }
};

// 坚守
class FortressCard : public CommandCard {
public:
    FortressCard() : CommandCard("坚守", "获得 20 点护盾",
        2, Rarity::COMMON, TargetMode::SELF) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new FortressCard(); }
};

// 紧急回避
class EmergencyDodgeCard : public CommandCard {
public:
    EmergencyDodgeCard() : CommandCard("紧急回避", "完全躲避下一次攻击",
        1, Rarity::UNCOMMON, TargetMode::SELF) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new EmergencyDodgeCard(); }
};

// 治疗波
class HealCard : public CommandCard {
public:
    HealCard() : CommandCard("治疗波", "恢复 15 点生命",
        2, Rarity::COMMON, TargetMode::SELF) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new HealCard(); }
};

// 净化
class PurifyCard : public CommandCard {
public:
    PurifyCard() : CommandCard("净化", "移除所有减益效果",
        1, Rarity::UNCOMMON, TargetMode::SELF) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new PurifyCard(); }
};

// ============================================================
// 指令牌 - 增益类
// ============================================================

// 强化
class StrengthCard : public CommandCard {
public:
    StrengthCard() : CommandCard("强化", "获得 2 点力量，持续 3 回合",
        1, Rarity::COMMON, TargetMode::SELF) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new StrengthCard(); }
};

// 狂暴
class BerserkerCard : public CommandCard {
public:
    BerserkerCard() : CommandCard("狂暴", "获得 5 点力量，持续 2 回合",
        2, Rarity::UNCOMMON, TargetMode::SELF) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new BerserkerCard(); }
};

// ============================================================
// 指令牌 - 召唤类
// ============================================================

// 召唤
class SummonCard : public CommandCard {
public:
    SummonCard() : CommandCard("召唤", "召唤一个基础仆从",
        2, Rarity::COMMON, TargetMode::NONE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SummonCard(); }
};

// 快速复制
class QuickCopyCard : public CommandCard {
public:
    QuickCopyCard() : CommandCard("快速复制", "复制一个己方仆从",
        2, Rarity::UNCOMMON, TargetMode::SINGLE_ALLY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new QuickCopyCard(); }
};

// 批量召唤
class MassSummonCard : public CommandCard {
public:
    MassSummonCard() : CommandCard("批量召唤", "连续召唤 3 个仆从",
        4, Rarity::RARE, TargetMode::NONE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new MassSummonCard(); }
};

// ============================================================
// 指令牌 - 献祭类
// ============================================================

// 献祭
class SacrificeCard : public CommandCard {
public:
    SacrificeCard() : CommandCard("献祭", "献祭一个己方仆从",
        0, Rarity::COMMON, TargetMode::SINGLE_ALLY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SacrificeCard(); }
};

// 血祭
class BloodSacrificeCard : public CommandCard {
public:
    BloodSacrificeCard() : CommandCard("血祭", "献祭一个仆从，回复等同于其生命值的生命",
        1, Rarity::UNCOMMON, TargetMode::SINGLE_ALLY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new BloodSacrificeCard(); }
};

// 连锁引爆
class ChainExplosionCard : public CommandCard {
public:
    ChainExplosionCard() : CommandCard("连锁引爆", "献祭一个仆从，对敌方全体造成其攻击力的伤害",
        2, Rarity::UNCOMMON, TargetMode::SINGLE_ALLY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ChainExplosionCard(); }
};

// ============================================================
// 指令牌 - 特殊类
// ============================================================

// const_cast 解除限制
class ConstCastCard : public CommandCard {
public:
    ConstCastCard() : CommandCard("const_cast", "解除一个单位的限制状态（眩晕/冻结）",
        1, Rarity::UNCOMMON, TargetMode::SINGLE_ALLY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new ConstCastCard(); }
};

// Lambda 注入
class LambdaCard : public CommandCard {
public:
    LambdaCard() : CommandCard("Lambda 注入", "造成 10 点伤害并获得 5 点护盾",
        2, Rarity::UNCOMMON, TargetMode::SINGLE_ENEMY) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new LambdaCard(); }
};

// ============================================================
// 模板牌
// ============================================================

// 模板·ForceInline
class ForceInlineCard : public TemplateCard {
public:
    ForceInlineCard() : TemplateCard("模板·ForceInline", "将包装的函数牌费用降低 1",
        1, Rarity::UNCOMMON) {}
    void applyWrapper(Player& player, Enemy* target) override;

protected:
    TemplateCard* cloneTemplate() const override {
        return new ForceInlineCard();
    }
};

// 模板·Double
class DoubleEffectCard : public TemplateCard {
public:
    DoubleEffectCard() : TemplateCard("模板·Double", "将包装的函数牌效果触发两次",
        2, Rarity::RARE) {}
    void applyWrapper(Player& player, Enemy* target) override;

protected:
    TemplateCard* cloneTemplate() const override {
        return new DoubleEffectCard();
    }
};

// 模板·Triple
class TripleEffectCard : public TemplateCard {
public:
    TripleEffectCard() : TemplateCard("模板·Triple", "将包装的函数牌效果触发三次",
        3, Rarity::RARE) {}
    void applyWrapper(Player& player, Enemy* target) override;

protected:
    TemplateCard* cloneTemplate() const override {
        return new TripleEffectCard();
    }
};

// 模板·Const
class ConstTemplateCard : public TemplateCard {
public:
    ConstTemplateCard() : TemplateCard("模板·Const", "将函数牌变为防御型（增加 5 点护盾）",
        1, Rarity::UNCOMMON) {}
    void applyWrapper(Player& player, Enemy* target) override;

protected:
    TemplateCard* cloneTemplate() const override {
        return new ConstTemplateCard();
    }
};

#endif // CARDS_H
