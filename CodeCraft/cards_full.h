#ifndef CARDS_FULL_H
#define CARDS_FULL_H

#include "cards.h"

// ============================================================
// 完整的卡牌库 - 所有攻击函数牌
// ============================================================
// 注意：AttackEnhanceCard 已在 cards.h 中定义，此处不重复

// 攻击函数·暴击：概率双倍伤害
class AttackCritCard : public FunctionCard {
public:
    AttackCritCard() : FunctionCard("攻击函数·暴击", "30% 概率造成双倍伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackCritCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·溅射：对相邻敌人造成伤害
class AttackSplashCard : public FunctionCard {
public:
    AttackSplashCard() : FunctionCard("攻击函数·溅射", "攻击时对相邻敌人造成 50% 伤害",
        3, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackSplashCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·破甲：无视部分防御
class AttackPierceCard : public FunctionCard {
public:
    AttackPierceCard() : FunctionCard("攻击函数·破甲", "无视目标 50% 护盾",
        2, Rarity::COMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackPierceCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·毒击：附加持续伤害
class AttackPoisonCard : public FunctionCard {
public:
    AttackPoisonCard() : FunctionCard("攻击函数·毒击", "攻击附加 3 层中毒",
        2, Rarity::COMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackPoisonCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·灼烧：附加灼烧效果
class AttackBurnCard : public FunctionCard {
public:
    AttackBurnCard() : FunctionCard("攻击函数·灼烧", "攻击附加每回合 5 点灼烧伤害",
        2, Rarity::COMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackBurnCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·冰冻：概率冻结目标
class AttackFreezeCard : public FunctionCard {
public:
    AttackFreezeCard() : FunctionCard("攻击函数·冰冻", "20% 概率冻结目标 1 回合",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackFreezeCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·雷霆：连锁伤害
class AttackLightningCard : public FunctionCard {
public:
    AttackLightningCard() : FunctionCard("攻击函数·雷霆", "对全体敌人造成 30% 连锁伤害",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackLightningCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·暗影：随时间叠加
class AttackShadowCard : public FunctionCard {
public:
    AttackShadowCard() : FunctionCard("攻击函数·暗影", "每次攻击伤害 +2",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackShadowCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·神圣：对特定类型额外伤害
class AttackHolyCard : public FunctionCard {
public:
    AttackHolyCard() : FunctionCard("攻击函数·神圣", "对暗影系敌人造成双倍伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackHolyCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·回复：攻击时恢复生命
class AttackHealCard : public FunctionCard {
public:
    AttackHealCard() : FunctionCard("攻击函数·回复", "攻击时恢复 3 点生命",
        1, Rarity::COMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackHealCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·斩杀：对低生命目标巨额伤害
class AttackExecuteCard : public FunctionCard {
public:
    AttackExecuteCard() : FunctionCard("攻击函数·斩杀", "对生命低于 30% 的目标造成三倍伤害",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackExecuteCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·连携：每个仆从增伤
class AttackSynergyCard : public FunctionCard {
public:
    AttackSynergyCard() : FunctionCard("攻击函数·连携", "每个己方仆从使攻击 +3",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackSynergyCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·狂暴：生命越低伤害越高
class AttackBerserkerCard : public FunctionCard {
public:
    AttackBerserkerCard() : FunctionCard("攻击函数·狂暴", "每损失 10 点生命，攻击 +5",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackBerserkerCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·精准：无视闪避和减免
class AttackPrecisionCard : public FunctionCard {
public:
    AttackPrecisionCard() : FunctionCard("攻击函数·精准", "攻击无视闪避和减免效果",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackPrecisionCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·吸取：降低目标攻击力
class AttackWeakenCard : public FunctionCard {
public:
    AttackWeakenCard() : FunctionCard("攻击函数·吸取", "攻击时降低目标 3 点攻击力",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackWeakenCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·震荡：概率眩晕
class AttackStunCard : public FunctionCard {
public:
    AttackStunCard() : FunctionCard("攻击函数·震荡", "30% 概率眩晕目标 1 回合",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackStunCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·回响：保留伤害记录
class AttackEchoCard : public FunctionCard {
public:
    AttackEchoCard() : FunctionCard("攻击函数·回响", "下次攻击追加本次伤害的 50%",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackEchoCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·蓄力：延时执行
class AttackChargeCard : public FunctionCard {
public:
    AttackChargeCard() : FunctionCard("攻击函数·蓄力", "本回合不攻击，下回合伤害 +200%",
        1, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackChargeCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·标记：给目标打标记
class AttackMarkCard : public FunctionCard {
public:
    AttackMarkCard() : FunctionCard("攻击函数·标记", "攻击给目标打标记，友方攻击标记目标 +5 伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackMarkCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·分裂：额外选中目标
class AttackSplitCard : public FunctionCard {
public:
    AttackSplitCard() : FunctionCard("攻击函数·分裂", "可额外选中一个目标，伤害各 70%",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackSplitCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·风怒：每回合可多次攻击
class AttackWindfuryCard : public FunctionCard {
public:
    AttackWindfuryCard() : FunctionCard("攻击函数·风怒", "每回合可攻击 2 次，每次伤害 70%",
        3, Rarity::RARE, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackWindfuryCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 攻击函数·吸收：伤害转化护盾
class AttackAbsorbCard : public FunctionCard {
public:
    AttackAbsorbCard() : FunctionCard("攻击函数·吸收", "造成伤害的 30% 转化为护盾",
        2, Rarity::UNCOMMON, FunctionTarget::ATTACK) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new AttackAbsorbCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// ============================================================
// 受击函数牌
// ============================================================

// 受击函数·铁壁：减免伤害
class DefendIronWallCard : public FunctionCard {
public:
    DefendIronWallCard() : FunctionCard("受击函数·铁壁", "减免 5 点伤害",
        2, Rarity::COMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendIronWallCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·反伤：反弹伤害
class DefendReflectCard : public FunctionCard {
public:
    DefendReflectCard() : FunctionCard("受击函数·反伤", "反弹 30% 伤害给攻击者",
        3, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendReflectCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·回春：受击恢复生命
class DefendRegenerationCard : public FunctionCard {
public:
    DefendRegenerationCard() : FunctionCard("受击函数·回春", "受击后恢复 3 点生命",
        1, Rarity::COMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendRegenerationCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·护盾：优先消耗护盾
class DefendShieldCard : public FunctionCard {
public:
    DefendShieldCard() : FunctionCard("受击函数·护盾", "获得 6 点护盾",
        2, Rarity::COMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendShieldCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·闪避：概率完全躲避
class DefendDodgeCard : public FunctionCard {
public:
    DefendDodgeCard() : FunctionCard("受击函数·闪避", "25% 概率完全躲避攻击",
        2, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendDodgeCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·硬皮：固定减免
class DefendArmorCard : public FunctionCard {
public:
    DefendArmorCard() : FunctionCard("受击函数·硬皮", "每次受击减免 3 点伤害",
        1, Rarity::COMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendArmorCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·吸收：伤害转能量
class DefendAbsorbCard : public FunctionCard {
public:
    DefendAbsorbCard() : FunctionCard("受击函数·吸收", "将 20% 伤害转化为能量",
        2, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendAbsorbCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·分摊：转移给仆从
class DefendDistributeCard : public FunctionCard {
public:
    DefendDistributeCard() : FunctionCard("受击函数·分摊", "将 50% 伤害转移给仆从",
        3, Rarity::RARE, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendDistributeCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·固守：受击提升防御
class DefendFortifyCard : public FunctionCard {
public:
    DefendFortifyCard() : FunctionCard("受击函数·固守", "受击后获得 2 点护盾",
        2, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendFortifyCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 受击函数·荆棘：固定反伤
class DefendThornsCard : public FunctionCard {
public:
    DefendThornsCard() : FunctionCard("受击函数·荆棘", "受击对攻击者造成 5 点反伤",
        2, Rarity::UNCOMMON, FunctionTarget::TAKE_DAMAGE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new DefendThornsCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 更多受击函数牌...（为节省篇幅，其余牌照此格式）

// ============================================================
// 构造/复制/移动/析构/逃跑函数牌
// ============================================================

// 构造函数·强化
class SummonEnhanceCard : public FunctionCard {
public:
    SummonEnhanceCard() : FunctionCard("构造函数·强化", "召唤的仆从属性 +50%",
        2, Rarity::COMMON, FunctionTarget::SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SummonEnhanceCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 构造函数·量产
class SummonMassCard : public FunctionCard {
public:
    SummonMassCard() : FunctionCard("构造函数·量产", "一次召唤 3 个弱化仆从",
        3, Rarity::UNCOMMON, FunctionTarget::SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SummonMassCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 复制构造·精准
class CopyPrecisionCard : public FunctionCard {
public:
    CopyPrecisionCard() : FunctionCard("复制构造·精准", "复制保留 90% 属性",
        2, Rarity::UNCOMMON, FunctionTarget::COPY_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new CopyPrecisionCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 复制构造·增殖
class CopyMultiplyCard : public FunctionCard {
public:
    CopyMultiplyCard() : FunctionCard("复制构造·增殖", "复制时额外产生一个次级复制",
        3, Rarity::RARE, FunctionTarget::COPY_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new CopyMultiplyCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 复制构造·改良
class CopyImproveCard : public FunctionCard {
public:
    CopyImproveCard() : FunctionCard("复制构造·改良", "复制体随机一项属性 +20%",
        2, Rarity::UNCOMMON, FunctionTarget::COPY_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new CopyImproveCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 移动构造·榨取
class MoveExtractCard : public FunctionCard {
public:
    MoveExtractCard() : FunctionCard("移动构造·榨取", "移动后获得 2 点能量",
        2, Rarity::UNCOMMON, FunctionTarget::MOVE_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new MoveExtractCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 移动构造·遗骸
class MoveRemnantsCard : public FunctionCard {
public:
    MoveRemnantsCard() : FunctionCard("移动构造·遗骸", "移动后原地留下残骸",
        1, Rarity::COMMON, FunctionTarget::MOVE_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new MoveRemnantsCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 移动构造·共鸣
class MoveResonanceCard : public FunctionCard {
public:
    MoveResonanceCard() : FunctionCard("移动构造·共鸣", "移动时同类型仆从攻击 +3",
        2, Rarity::UNCOMMON, FunctionTarget::MOVE_SUMMON) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new MoveResonanceCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 析构函数·爆裂
class SacrificeExplodeCard : public FunctionCard {
public:
    SacrificeExplodeCard() : FunctionCard("析构函数·爆裂", "献祭时对全体敌人造成仆从生命值伤害",
        2, Rarity::UNCOMMON, FunctionTarget::SACRIFICE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SacrificeExplodeCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 析构函数·传承
class SacrificeInheritCard : public FunctionCard {
public:
    SacrificeInheritCard() : FunctionCard("析构函数·传承", "献祭时将仆从增益转移给玩家",
        2, Rarity::UNCOMMON, FunctionTarget::SACRIFICE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SacrificeInheritCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 析构函数·重生
class SacrificeRebornCard : public FunctionCard {
public:
    SacrificeRebornCard() : FunctionCard("析构函数·重生", "30% 概率仆从不销毁并恢复生命",
        1, Rarity::UNCOMMON, FunctionTarget::SACRIFICE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new SacrificeRebornCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 逃跑函数·诡步
class EscapeNimbleCard : public FunctionCard {
public:
    EscapeNimbleCard() : FunctionCard("逃跑函数·诡步", "逃跑成功率 +30%",
        1, Rarity::COMMON, FunctionTarget::ESCAPE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new EscapeNimbleCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 逃跑函数·金蝉脱壳
class EscapeMoltCard : public FunctionCard {
public:
    EscapeMoltCard() : FunctionCard("逃跑函数·金蝉脱壳", "逃跑失败时承受 50% 伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ESCAPE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new EscapeMoltCard(); }
    std::vector<std::string> getCodeLines() const override;
};

// 逃跑函数·断后
class EscapeRearguardCard : public FunctionCard {
public:
    EscapeRearguardCard() : FunctionCard("逃跑函数·断后", "逃跑成功时对敌人造成伤害",
        2, Rarity::UNCOMMON, FunctionTarget::ESCAPE) {}
    void play(Player& player, Enemy* target) override;
    Card* clone() const override { return new EscapeRearguardCard(); }
    std::vector<std::string> getCodeLines() const override;
};

#endif // CARDS_FULL_H
