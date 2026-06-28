#include "game_manager.h"
#include <cstdlib>   // rand, srand
#include <ctime>     // time
#include <algorithm> // shuffle, remove_if
#include <random>    // mt19937 for map generation
#include <map>
#include <QRegularExpression>

// ============================================================
// 静态变量定义
// ============================================================
int GameManager::currentLevel = 1;
GameMap GameManager::currentMap;
int GameManager::currentNodeId = -1;
std::vector<std::unique_ptr<Card>> GameManager::cardCollection;
std::vector<std::unique_ptr<Card>> GameManager::nodeRewards;
int GameManager::persistentHp = DEFAULT_MAX_HP;
int GameManager::persistentMaxHp = DEFAULT_MAX_HP;
bool GameManager::persistentStateValid = false;

static CardView makeCardViewFromCard(const Card* card)
{
    CardView view;
    if (!card) {
        return view;
    }

    view.name = QString::fromStdString(card->name);
    view.description = QString::fromStdString(card->description);
    view.cost = card->cost;
    view.targetMode = card->targetMode;
    for (const std::string& line : card->getCodeLines()) {
        view.codeLines << QString::fromStdString(line);
    }
    return view;
}


// ============================================================
// 多敌人目标辅助
// ============================================================

static QString enemyCodeExpr(Enemy* enemy)
{
    if (!enemy) {
        return QStringLiteral("enemy");
    }

    QString name = QString::fromStdString(enemy->name);
    name.replace("\\", "\\\\");
    name.replace("\"", "\\\"");
    return QString("enemy[\"%1\"]").arg(name);
}

static Enemy* findAliveEnemyByName(BattleContext& battle, const std::string& name)
{
    for (auto& enemy : battle.enemies) {
        if (enemy && enemy->isAlive() && enemy->name == name) {
            return enemy.get();
        }
    }
    return nullptr;
}

static QStringList makeTargetedCodeLines(const Card* card, Enemy* target)
{
    QStringList lines;
    if (!card) {
        return lines;
    }

    const QString targetExpr = enemyCodeExpr(target);
    for (const std::string& raw : card->getCodeLines()) {
        QString line = QString::fromStdString(raw);

        // 只替换独立单词 enemy，不会影响 enemies / adjacentEnemies。
        if (target) {
            line.replace(QRegularExpression(QStringLiteral("\\benemy\\b")), targetExpr);
        }

        lines << line;
    }

    return lines;
}

static void disambiguateEnemyDisplayNames(BattleContext& battle)
{
    std::map<std::string, int> totalCount;
    for (const auto& enemy : battle.enemies) {
        if (enemy) {
            totalCount[enemy->name]++;
        }
    }

    std::map<std::string, int> usedCount;
    for (auto& enemy : battle.enemies) {
        if (!enemy) {
            continue;
        }

        const std::string baseName = enemy->name;
        if (totalCount[baseName] <= 1) {
            continue;
        }

        int index = ++usedCount[baseName];
        enemy->name = baseName + "#" + std::to_string(index);
    }
}

GameManager::GameManager(int nodeId)
    : battle(player)
{
    srand(static_cast<unsigned>(time(nullptr)));

    // 解析节点 ID
    if (nodeId >= 0) {
        currentNodeId = nodeId;
    } else if (currentNodeId < 0 && !currentMap.isEmpty()) {
        currentNodeId = currentMap.startNodeId;
    }

    initLevel();

    // 恢复持久化的血量（非首战时）
    if (persistentStateValid) {
        player.hp = persistentHp;
        player.maxHp = persistentMaxHp;
    }

    initDeck();
}

// ============================================================
// 敌人工厂：根据名称字符串创建敌人实例
// ============================================================

std::unique_ptr<Enemy> GameManager::createEnemyByName(const std::string& name) {
    if (name == "Goblin")        return std::make_unique<Goblin>();
    if (name == "FireGoblin")    return std::make_unique<FireGoblin>();
    if (name == "FrozenGoblin")  return std::make_unique<FrozenGoblin>();
    if (name == "Caster")        return std::make_unique<Caster>();
    if (name == "TemplateKing")  return std::make_unique<TemplateKing>();
    if (name == "ExceptionLord") return std::make_unique<ExceptionLord>();
    return nullptr;
}

// ============================================================
// 卡牌工厂：根据名称字符串创建卡牌实例
// ============================================================

std::unique_ptr<Card> GameManager::createCardByName(const std::string& name) {
    // ---- 函数牌：攻击类 ----
    if (name == "AttackEnhanceCard")    return std::make_unique<AttackEnhanceCard>();
    if (name == "VampireAttackCard")    return std::make_unique<VampireAttackCard>();
    if (name == "ComboAttackCard")      return std::make_unique<ComboAttackCard>();
    if (name == "CritAttackCard")       return std::make_unique<CritAttackCard>();
    if (name == "PoisonAttackCard")     return std::make_unique<PoisonAttackCard>();
    if (name == "BurnAttackCard")       return std::make_unique<BurnAttackCard>();
    if (name == "ExecuteAttackCard")    return std::make_unique<ExecuteAttackCard>();
    if (name == "SynergyAttackCard")    return std::make_unique<SynergyAttackCard>();
    if (name == "BerserkerAttackCard")  return std::make_unique<BerserkerAttackCard>();
    if (name == "MarkAttackCard")       return std::make_unique<MarkAttackCard>();

    // ---- 函数牌：防御类 ----
    if (name == "IronWallCard")         return std::make_unique<IronWallCard>();
    if (name == "CounterDamageCard")    return std::make_unique<CounterDamageCard>();
    if (name == "RegenerationCard")     return std::make_unique<RegenerationCard>();
    if (name == "DodgeCard")            return std::make_unique<DodgeCard>();
    if (name == "ThornsCard")           return std::make_unique<ThornsCard>();
    if (name == "RageCard")             return std::make_unique<RageCard>();
    if (name == "FortifyCard")          return std::make_unique<FortifyCard>();

    // ---- 函数牌：召唤/复制/移动/献祭/逃跑类 ----
    if (name == "EnhancedSummonCard")   return std::make_unique<EnhancedSummonCard>();
    if (name == "EliteSummonCard")      return std::make_unique<EliteSummonCard>();
    if (name == "PreciseCopyCard")      return std::make_unique<PreciseCopyCard>();
    if (name == "ProliferateCopyCard")  return std::make_unique<ProliferateCopyCard>();
    if (name == "RemainsMoveCard")      return std::make_unique<RemainsMoveCard>();
    if (name == "InheritSacrificeCard") return std::make_unique<InheritSacrificeCard>();
    if (name == "RearguardEscapeCard")  return std::make_unique<RearguardEscapeCard>();

    // ---- 指令牌 ----
    if (name == "PowerStrikeCard")      return std::make_unique<PowerStrikeCard>();
    if (name == "SweepCard")            return std::make_unique<SweepCard>();
    if (name == "DefendCard")           return std::make_unique<DefendCard>();
    if (name == "FortressCard")         return std::make_unique<FortressCard>();
    if (name == "EmergencyDodgeCard")   return std::make_unique<EmergencyDodgeCard>();
    if (name == "HealCard")             return std::make_unique<HealCard>();
    if (name == "PurifyCard")           return std::make_unique<PurifyCard>();
    if (name == "StrengthCard")         return std::make_unique<StrengthCard>();
    if (name == "SummonCard")           return std::make_unique<SummonCard>();
    if (name == "QuickCopyCard")        return std::make_unique<QuickCopyCard>();
    if (name == "SacrificeCard")        return std::make_unique<SacrificeCard>();
    if (name == "BloodSacrificeCard")   return std::make_unique<BloodSacrificeCard>();

    return nullptr;
}

// ============================================================
// 关卡初始化（优先使用地图节点，无地图时回退到 currentLevel）
// ============================================================

void GameManager::initLevel() {
    // 优先使用地图系统
    const MapNode* node = currentMap.getNode(currentNodeId);
    if (node && !node->enemyTypes.empty()) {
        for (const auto& enemyType : node->enemyTypes) {
            auto enemy = createEnemyByName(enemyType);
            if (enemy) {
                battle.addEnemy(std::move(enemy));
            }
        }

        // 同一种怪物如果出现多个，必须在 GameManager 层给出稳定、唯一的显示名。
        // 例如：程序猿#1 / 程序猿#2。
        // UI 和代码行都依赖这个名字来区分目标。
        disambiguateEnemyDisplayNames(battle);
        return;
    }

    // 无地图时回退到旧关卡系统
    switch (currentLevel) {
        case 1:
            battle.addEnemy(createEnemyByName("Goblin"));
            break;
        case 2:
            battle.addEnemy(createEnemyByName("Goblin"));
            battle.addEnemy(createEnemyByName("Goblin"));
            break;
        case 3:
            battle.addEnemy(createEnemyByName("FireGoblin"));
            battle.addEnemy(createEnemyByName("FrozenGoblin"));
            break;
        case 4:
            battle.addEnemy(createEnemyByName("Goblin"));
            battle.addEnemy(createEnemyByName("Goblin"));
            battle.addEnemy(createEnemyByName("Caster"));
            break;
        case 5:
            battle.addEnemy(createEnemyByName("TemplateKing"));
            break;
        case 6:
            battle.addEnemy(createEnemyByName("FireGoblin"));
            battle.addEnemy(createEnemyByName("Caster"));
            battle.addEnemy(createEnemyByName("Goblin"));
            break;
        case 7:
            battle.addEnemy(createEnemyByName("Caster"));
            battle.addEnemy(createEnemyByName("Caster"));
            battle.addEnemy(createEnemyByName("FrozenGoblin"));
            break;
        case 8:
            battle.addEnemy(createEnemyByName("ExceptionLord"));
            break;
        default:
            battle.addEnemy(createEnemyByName("Goblin"));
            break;
    }

    disambiguateEnemyDisplayNames(battle);
}

// ============================================================
// 地图生成
// ============================================================

// 根据深度比例返回该层可选的敌人类型池
static std::vector<std::string> getEnemyPoolForDepthRatio(double ratio) {
    if (ratio < 0.15) {
        // 第一层战斗：简单
        return {"Goblin"};
    }
    if (ratio < 0.35) {
        // 前期：普通敌人
        return {"Goblin", "FireGoblin", "FrozenGoblin"};
    }
    if (ratio < 0.60) {
        // 中期：混合
        return {"Goblin", "FireGoblin", "FrozenGoblin", "Caster"};
    }
    if (ratio < 0.85) {
        // 后期：强敌组合
        return {"FireGoblin", "FrozenGoblin", "Caster"};
    }
    // Boss 前最后一层：强敌
    return {"Caster", "FireGoblin", "FrozenGoblin"};
}

// 根据深度比例决定该节点敌人数量
static int getEnemyCountForDepthRatio(double ratio, std::mt19937& rng) {
    if (ratio < 0.15) return 1;
    if (ratio < 0.40) {
        // 前期 1-2 个
        return (rng() % 2) + 1;
    }
    if (ratio < 0.75) {
        // 中期 2-3 个
        return (rng() % 2) + 2;
    }
    // 后期 3 个
    return 3;
}

// 随机从池中选取 count 个敌人类型
static std::vector<std::string> pickEnemiesFromPool(
    const std::vector<std::string>& pool, int count, std::mt19937& rng)
{
    std::vector<std::string> result;
    if (pool.empty() || count <= 0) return result;

    for (int i = 0; i < count; ++i) {
        int idx = rng() % pool.size();
        result.push_back(pool[idx]);
    }
    return result;
}

// 根据种子选择 Boss 类型
static std::string pickBossType(int seed) {
    return (seed % 2 == 0) ? "TemplateKing" : "ExceptionLord";
}

// depthRatio → 卡牌名称池
static std::vector<std::string> getRewardCardPool(double ratio) {
    // 前期：基础卡
    std::vector<std::string> pool = {
        "PowerStrikeCard", "DefendCard", "AttackEnhanceCard",
        "HealCard", "StrengthCard", "SummonCard",
        "PoisonAttackCard", "BurnAttackCard"
    };
    // 中期：加入进阶卡
    if (ratio >= 0.30) {
        pool.insert(pool.end(), {
            "VampireAttackCard", "ComboAttackCard", "CritAttackCard",
            "SynergyAttackCard", "IronWallCard", "CounterDamageCard",
            "DodgeCard", "ThornsCard", "EnhancedSummonCard",
            "FortressCard", "SweepCard", "PurifyCard"
        });
    }
    // 后期：加入稀有卡
    if (ratio >= 0.60) {
        pool.insert(pool.end(), {
            "ExecuteAttackCard", "BerserkerAttackCard", "MarkAttackCard",
            "RegenerationCard", "RageCard", "FortifyCard",
            "EliteSummonCard", "PreciseCopyCard", "ProliferateCopyCard",
            "RemainsMoveCard", "InheritSacrificeCard",
            "QuickCopyCard", "BloodSacrificeCard", "EmergencyDodgeCard"
        });
    }
    return pool;
}

void GameManager::generateMap(int seed) {
    std::mt19937 rng(static_cast<unsigned>(seed));

    currentMap = GameMap();
    currentMap.seed = seed;

    // ---- 1. 确定地图深度（5~8 层战斗 + 起点 + Boss = 7~10 层总节点深度）----
    const int battleDepth = 5 + (seed % 4);  // 战斗层数：5~8
    const int totalDepth = battleDepth + 1;  // 包含 Boss 层的总深度（不含起点）

    int nextId = 0;
    std::vector<std::vector<int>> layerNodes(totalDepth + 1);  // layer 0 = 起点, layer 1..battleDepth = 战斗, layer totalDepth = Boss

    // ---- 2. 创建起点（layer 0）----
    {
        MapNode start;
        start.id = nextId++;
        start.depth = 0;
        start.isStart = true;
        int startId = start.id;
        currentMap.nodes.push_back(std::move(start));
        currentMap.startNodeId = startId;
        layerNodes[0].push_back(startId);
    }

    // ---- 3. 创建中间战斗节点（layer 1 到 battleDepth）----
    for (int layer = 1; layer <= battleDepth; ++layer) {
        double depthRatio = static_cast<double>(layer) / totalDepth;
        auto pool = getEnemyPoolForDepthRatio(depthRatio);

        // 每层 2~4 个节点
        int nodesInLayer = 2 + (rng() % 3);

        for (int n = 0; n < nodesInLayer; ++n) {
            int count = getEnemyCountForDepthRatio(depthRatio, rng);
            auto enemyTypes = pickEnemiesFromPool(pool, count, rng);

            MapNode node;
            node.id = nextId++;
            node.depth = layer;
            node.enemyTypes = enemyTypes;
            // 为每个战斗节点预生成 3 张奖励牌（用同一 RNG 保证确定性）
            {
                auto rewardPool = getRewardCardPool(depthRatio);
                for (int r = 0; r < 3; ++r) {
                    int ridx = rng() % rewardPool.size();
                    auto card = createCardByName(rewardPool[ridx]);
                    if (card) {
                        node.rewardCards.push_back(std::move(card));
                    }
                }
            }
            int nodeId = node.id;
            currentMap.nodes.push_back(std::move(node));
            layerNodes[layer].push_back(nodeId);
        }
    }

    // ---- 4. 创建 Boss 节点（layer totalDepth）----
    {
        std::string bossType = pickBossType(seed);
        MapNode boss;
        boss.id = nextId++;
        boss.depth = totalDepth;
        boss.isBoss = true;
        boss.enemyTypes = {bossType};
        int bossId = boss.id;
        currentMap.nodes.push_back(std::move(boss));
        currentMap.bossNodeId = bossId;
        layerNodes[totalDepth].push_back(bossId);
    }

    // ---- 5. 构建邻接表（区间划分式连边）----
    // 下一层节点用连续区间瓜分上一层节点，相邻区间共享端点
    // 上层节点 j 连到所有"覆盖了 j"的下层节点
    currentMap.edges.resize(nextId);

    for (int layer = 0; layer < totalDepth; ++layer) {
        const auto& fromNodes = layerNodes[layer];
        const auto& toNodes = layerNodes[layer + 1];
        int M = static_cast<int>(fromNodes.size());
        int N = static_cast<int>(toNodes.size());

        if (M == 0 || N == 0) continue;

        // ---- 5a. 生成 N-1 个分割点（允许重复，即相邻区间共享端点）----
        // split[i] 是下层节点 i 和 i+1 之间的分界点（在上层节点索引空间）
        std::vector<int> splits;
        if (N > 1) {
            for (int i = 0; i < N - 1; ++i) {
                // 分割点在 [0, M-1] 中随机选取，允许重复
                splits.push_back(rng() % M);
            }
            std::sort(splits.begin(), splits.end());
        }

        // ---- 5b. 每个下层节点覆盖一个连续区间 ----
        // 下层节点 i 覆盖上层节点区间 [rangeStart, rangeEnd]（均包含）
        // split[-1] 视为 0，split[N-1] 视为 M-1
        for (int ti = 0; ti < N; ++ti) {
            int rangeStart = (ti == 0) ? 0 : splits[ti - 1];
            int rangeEnd   = (ti == N - 1) ? M - 1 : splits[ti];

            // 将该区间内所有上层节点连到此下层节点
            for (int fi = rangeStart; fi <= rangeEnd; ++fi) {
                int fromId = fromNodes[fi];
                int toId   = toNodes[ti];
                currentMap.edges[fromId].push_back(toId);
            }
        }
    }

    // ---- 6. 设置当前节点为起点 ----
    currentNodeId = currentMap.startNodeId;
}

// ============================================================
// 卡牌库：初始 15 张起始卡牌
// ============================================================

void GameManager::initCardCollection() {
    cardCollection.clear();

    // 基础卡牌（共 13 张）
    for (int i = 0; i < 3; ++i) cardCollection.push_back(std::make_unique<PowerStrikeCard>());
    for (int i = 0; i < 3; ++i) cardCollection.push_back(std::make_unique<DefendCard>());
    for (int i = 0; i < 2; ++i) cardCollection.push_back(std::make_unique<AttackEnhanceCard>());
    for (int i = 0; i < 2; ++i) cardCollection.push_back(std::make_unique<HealCard>());
    for (int i = 0; i < 2; ++i) cardCollection.push_back(std::make_unique<StrengthCard>());
    cardCollection.push_back(std::make_unique<SummonCard>());
    // 再加 2 张凑满 15
    cardCollection.push_back(std::make_unique<PoisonAttackCard>());
    cardCollection.push_back(std::make_unique<BurnAttackCard>());
}

// ============================================================
// 玩家持久化状态
// ============================================================

void GameManager::savePlayerHp(int hp, int maxHp) {
    persistentHp = hp;
    persistentMaxHp = maxHp;
    persistentStateValid = true;
}

void GameManager::resetPlayerState() {
    persistentHp = DEFAULT_MAX_HP;
    persistentMaxHp = DEFAULT_MAX_HP;
    persistentStateValid = false;
}

// ============================================================
// 节点奖励：实例化当前节点的 rewardCards
// ============================================================

void GameManager::prepareNodeRewards() {
    nodeRewards.clear();

    const MapNode* node = currentMap.getNode(currentNodeId);
    if (!node || node->rewardCards.empty()) return;

    // 从节点中移走奖励牌（节点只访问一次，之后这些牌不再需要）
    MapNode* mutableNode = const_cast<MapNode*>(node);
    nodeRewards = std::move(mutableNode->rewardCards);
}

// ============================================================
// 卡牌交换：奖励牌 ↔ 固定卡牌库
// ============================================================

bool GameManager::exchangeCard(int rewardIdx, int poolIdx) {
    if (rewardIdx < 0 || rewardIdx >= static_cast<int>(nodeRewards.size()))
        return false;
    if (poolIdx < 0 || poolIdx >= static_cast<int>(cardCollection.size()))
        return false;

    std::swap(nodeRewards[rewardIdx], cardCollection[poolIdx]);
    return true;
}

// ============================================================
// 牌组初始化：从 cardCollection 拷贝到 drawPile
// ============================================================

void GameManager::initDeck() {
    // 如果 cardCollection 为空（未调用 initCardCollection），回退到旧逻辑
    if (cardCollection.empty()) {
        for (int i = 0; i < 3; ++i) drawPile.push_back(std::make_unique<PowerStrikeCard>());
        for (int i = 0; i < 3; ++i) drawPile.push_back(std::make_unique<DefendCard>());
        for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<AttackEnhanceCard>());
        for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<HealCard>());
        for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<StrengthCard>());
        drawPile.push_back(std::make_unique<SummonCard>());
    } else {
        for (auto& card : cardCollection) {
            if (card) {
                drawPile.push_back(std::unique_ptr<Card>(card->clone()));
            }
        }
    }

    // 洗牌
    {
        std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
        std::shuffle(drawPile.begin(), drawPile.end(), rng);
    }
}

// ============================================================
// 回合流程（代码执行模式）
// ============================================================

void GameManager::beginTurnWithoutDraw() {
    turnNumber++;
    player.resetActionLimits();
    growMaxEnergy();
    restoreEnergy();
    if (battle.onTurnStart) battle.onTurnStart(turnNumber);
}

void GameManager::prepareTurnCodeBlock() {
    pendingCommands.clear();
    prepareAttackCodeBlock();
    prepareEndCodeBlock();
}

std::vector<std::string> GameManager::getPlayerCodeLines() const {
    return battle.player.getStatusesCode();
}

std::vector<std::string> GameManager::getEnemyCodeLines(Enemy* enemy) const {
    if (!enemy) return {};
    return enemy->getStatusesCode();
}

std::vector<std::string> GameManager::getEnemyDescription(Enemy* enemy) const {
    if (!enemy) return {};
    return enemy->getDescription();
}

void GameManager::prepareAttackCodeBlock() {
    // 仆从逐个攻击（倒序）
    for (int i = static_cast<int>(player.minions.size()) - 1; i >= 0; --i) {
        auto& m = player.minions[i];
        if (!m.isAlive() || m.isDisabled()) continue;

        Minion* raw = &m;
        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(raw->name);
        cmd.source = CommandSource::MINION;
        cmd.lines = {QString("target = enemy[random()];"),
                     QString("target.takeDamage(%1, PHYSICAL);").arg(raw->getEffectiveAttack())};
        cmd.effect = [this, raw]() {
            // 确认仆从仍在存活列表中（可能已被其他效果击杀并移除）
            bool found = false;
            for (auto& m : player.minions) {
                if (&m == raw) { found = true; break; }
            }
            if (!found || !raw->isAlive()) return;
            Enemy* t = battle.getRandomEnemy();
            if (t) t->takeDamage(raw->getEffectiveAttack(), DamageType::PHYSICAL);
        };
        pendingCommands.push_back(std::move(cmd));
    }

    // 敌人逐个行动（倒序）
    for (int i = static_cast<int>(battle.enemies.size()) - 1; i >= 0; --i) {
        auto& e = battle.enemies[i];
        if (!e->isAlive() || e->isDisabled()) continue;

        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(e->name);
        cmd.lines = buildEnemyCodeLines(e.get());
        cmd.source = CommandSource::ENEMY;
        cmd.effect = [this, raw = e.get()]() {
            // 确认敌人仍在存活列表中（可能已被其他效果击杀并移除）
            bool found = false;
            for (auto& ep : battle.enemies) {
                if (ep.get() == raw) { found = true; break; }
            }
            if (!found || !raw->isAlive()) return;
            raw->takeTurn(player);
        };
        pendingCommands.push_back(std::move(cmd));
    }
}

void GameManager::prepareEndCodeBlock() {
    // 玩家状态结算
    {
        PendingCodeCommand cmd;
        cmd.title = QStringLiteral("玩家状态结算");
        cmd.lines = {"player.tickStatuses();  // 灼烧/中毒/再生"};
        cmd.source = CommandSource::END;
        cmd.effect = [this]() { if (!player.isAlive()) return; player.tickStatuses(); };
        pendingCommands.push_back(std::move(cmd));
    }
    // 仆从状态结算（倒序）
    for (int i = static_cast<int>(player.minions.size()) - 1; i >= 0; --i) {
        auto& m = player.minions[i];
        if (!m.isAlive()) continue;
        Minion* raw = &m;
        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(raw->name);
        cmd.lines = {QString("%1.tickStatuses();").arg(cmd.title)};
        cmd.source = CommandSource::MINION;
        cmd.effect = [this, raw]() {
            // 确认仆从仍在存活列表中（可能已被其他效果击杀并移除）
            bool found = false;
            for (auto& m : player.minions) {
                if (&m == raw) { found = true; break; }
            }
            if (!found || !raw->isAlive()) return;
            raw->tickStatuses();
        };
        pendingCommands.push_back(std::move(cmd));
    }
    // 敌人状态结算（倒序）
    for (int i = static_cast<int>(battle.enemies.size()) - 1; i >= 0; --i) {
        auto& e = battle.enemies[i];
        if (!e->isAlive()) continue;
        PendingCodeCommand cmd;
        cmd.title = QString::fromStdString(e->name);
        cmd.lines = {QString("%1.tickStatuses();").arg(cmd.title)};
        cmd.source = CommandSource::ENEMY;
        cmd.effect = [this, raw = e.get()]() {
            // 确认敌人仍在存活列表中（可能已被其他效果击杀并移除）
            bool found = false;
            for (auto& ep : battle.enemies) {
                if (ep.get() == raw) { found = true; break; }
            }
            if (!found || !raw->isAlive()) return;
            raw->tickStatuses();
        };
        pendingCommands.push_back(std::move(cmd));
    }
}

TurnResult GameManager::finishTurnAfterCodeExecution() {
    if (battle.onTurnEnd) battle.onTurnEnd(turnNumber);

    return {false, false};
}

// ============================================================
// 能量管理
// ============================================================

void GameManager::growMaxEnergy() {
    player.maxEnergy = DEFAULT_MAX_ENERGY + turnNumber - 1;
}

void GameManager::restoreEnergy() {
    int oldEnergy = player.energy;
    player.energy = player.maxEnergy;
    if (player.onEnergyChanged)
        player.onEnergyChanged(player.energy, player.maxEnergy,
                               player.energy - oldEnergy);
}

void GameManager::spendEnergy(int cost) {
    int oldEnergy = player.energy;
    player.energy -= cost;
    if (player.onEnergyChanged)
        player.onEnergyChanged(player.energy, player.maxEnergy,
                               player.energy - oldEnergy);
}

// ============================================================
// 牌组操作
// ============================================================

DrawResult GameManager::drawOneCard() {
    if (hand.size() >= MAX_HAND_SIZE)
        return {false, false, -1, {}};

    // 牌堆空但弃牌堆有牌 → 通知 Qt 先播洗牌动画
    if (drawPile.empty() && !discardPile.empty())
        return {false, true, -1, {}};

    // 牌堆空且弃牌堆也空 → 没牌可抽
    if (drawPile.empty())
        return {false, false, -1, {}};

    // 随机抽一张
    int idx = rand() % drawPile.size();
    std::unique_ptr<Card> card = std::move(drawPile[idx]);
    drawPile.erase(drawPile.begin() + idx);

    CardView view = makeCardViewFromCard(card.get());
    int handIndex = static_cast<int>(hand.size());
    hand.push_back(std::move(card));

    return {true, false, handIndex, view};
}

void GameManager::recycleDiscardToDrawPile() {
    for (auto& card : discardPile)
        drawPile.push_back(std::move(card));
    discardPile.clear();

    // 洗牌
    {
        std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
        std::shuffle(drawPile.begin(), drawPile.end(), rng);
    }
}

PlayResult GameManager::playCardAsCode(int handIndex, Enemy* target) {
    if (handIndex < 0 || handIndex >= static_cast<int>(hand.size()))
        return {false, handIndex, {}, QStringLiteral("手牌不存在")};

    Card* card = hand[handIndex].get();
    if (!card) return {false, handIndex, {}, QStringLiteral("手牌不存在")};

    if (!card->canPlay(player))
        return {false, handIndex, {}, QStringLiteral("当前无法打出此牌")};

    if (player.energy < card->cost)
        return {false, handIndex, {}, QStringLiteral("能量不足")};

    spendEnergy(card->cost);

    CardView view = makeCardViewFromCard(card);

    // 不立即执行，而是挂入代码队列。
    // 这里不再捕获 Enemy*，因为敌人死亡后 BattleContext 可能移除 unique_ptr，
    // 原指针会悬空。我们记录选中敌人的唯一显示名，真正执行时再按名字解析。
    Card* rawCard = card;
    const std::string targetName = target ? target->name : std::string();

    QStringList lines = makeTargetedCodeLines(card, target);
    view.codeLines = lines;

    PendingCodeCommand cmd;
    cmd.title = view.name;
    cmd.lines = lines;
    cmd.effect = [this, rawCard, targetName]() {
        if (!player.isAlive()) return;
        Enemy* resolvedTarget = targetName.empty()
            ? nullptr
            : findAliveEnemyByName(battle, targetName);

        rawCard->play(player, resolvedTarget);
    };

    insertPlayerCommandBeforeEnemy(std::move(cmd));

    discardPile.push_back(std::move(hand[handIndex]));
    hand.erase(hand.begin() + handIndex);

    return {true, handIndex, view, ""};
}

void GameManager::discardHand() {
    for (auto& card : hand)
        discardPile.push_back(std::move(card));
    hand.clear();
}

// ============================================================
// 查询（Qt 只读）
// ============================================================

QVector<CardView> GameManager::getHandView() const {
    QVector<CardView> result;
    for (auto& card : hand) {
        if (card) {
            result.push_back(makeCardViewFromCard(card.get()));
        } else {
            result.push_back({});
        }
    }
    return result;
}

std::string GameManager::getEnemyIntentText() const {
    std::string text;
    for (auto& e : battle.enemies) {
        if (!text.empty()) text += "\n";
        text += e->name + "：" + e->nextIntent.name();
        if (e->nextIntent.value > 0)
            text += " " + std::to_string(e->nextIntent.value);
    }
    return text.empty() ? "无敌人" : text;
}

// ============================================================
// 代码执行模式
// ============================================================

QVector<CodeCommandView> GameManager::getCodeCommandViews() const {
    QVector<CodeCommandView> result;
    for (auto& cmd : pendingCommands)
        result.push_back({cmd.title, cmd.lines, cmd.executed});
    return result;
}

int GameManager::pendingCodeCount() const {
    return static_cast<int>(pendingCommands.size());
}

void GameManager::executePendingCode(int index) {
    if (index < 0 || index >= pendingCommands.size()) return;
    auto& cmd = pendingCommands[index];
    if (cmd.executed) return;
    if (cmd.effect) cmd.effect();
    cmd.executed = true;
}

void GameManager::insertPlayerCommandBeforeEnemy(PendingCodeCommand cmd) {
    // 插入到最后一个玩家操作之后、第一个仆从/敌人操作之前
    int insertPos = 0;
    for (int i = static_cast<int>(pendingCommands.size()) - 1; i >= 0; --i) {
        if (pendingCommands[i].source == CommandSource::PLAYER) {
            insertPos = i + 1;
            break;
        }
    }
    pendingCommands.insert(insertPos, std::move(cmd));
}

QStringList GameManager::buildEnemyCodeLines(Enemy* enemy) const {
    if (!enemy)
        return {"// enemy_action", "enemy.wait();"};
    return {QString("// enemy_action"),
            QString("%1.takeTurn(player);").arg(QString::fromStdString(enemy->name))};
}


// ============================================================
// Qt 只读视图：奖励交换界面使用
// MainWindow 不直接访问 unique_ptr<Card>，只拿 CardView 显示。
// ============================================================

QVector<CardView> GameManager::getCardCollectionView()
{
    QVector<CardView> result;
    for (const auto& card : cardCollection) {
        if (card) {
            result.push_back(makeCardViewFromCard(card.get()));
        } else {
            result.push_back({});
        }
    }
    return result;
}

QVector<CardView> GameManager::getNodeRewardView()
{
    QVector<CardView> result;
    for (const auto& card : nodeRewards) {
        if (card) {
            result.push_back(makeCardViewFromCard(card.get()));
        } else {
            result.push_back({});
        }
    }
    return result;
}

