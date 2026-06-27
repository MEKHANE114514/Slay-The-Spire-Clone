#include "game_manager.h"
#include <cstdlib>   // rand, srand
#include <ctime>     // time
#include <algorithm> // shuffle, remove_if
#include <random>    // mt19937 for map generation

// ============================================================
// 静态变量定义
// ============================================================
int GameManager::currentLevel = 1;
GameMap GameManager::currentMap;
int GameManager::currentNodeId = -1;

namespace {
CardView makeCardViewFromCard(const Card* card)
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
}

// ============================================================
// 地图生成
// ============================================================

namespace {

// 根据深度比例返回该层可选的敌人类型池
std::vector<std::string> getEnemyPoolForDepthRatio(double ratio) {
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
int getEnemyCountForDepthRatio(double ratio, std::mt19937& rng) {
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
std::vector<std::string> pickEnemiesFromPool(
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
std::string pickBossType(int seed) {
    return (seed % 2 == 0) ? "TemplateKing" : "ExceptionLord";
}

} // anonymous namespace

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
        start.displayName = QStringLiteral("起点");
        currentMap.nodes.push_back(start);
        currentMap.startNodeId = start.id;
        layerNodes[0].push_back(start.id);
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
            node.displayName = QString();
            currentMap.nodes.push_back(node);
            layerNodes[layer].push_back(node.id);
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
        boss.displayName = QString();
        currentMap.nodes.push_back(boss);
        currentMap.bossNodeId = boss.id;
        layerNodes[totalDepth].push_back(boss.id);
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
// 牌组初始化（基础牌组）
// ============================================================

void GameManager::initDeck() {
    // 每种基础卡牌各放几张
    for (int i = 0; i < 3; ++i) drawPile.push_back(std::make_unique<PowerStrikeCard>());
    for (int i = 0; i < 3; ++i) drawPile.push_back(std::make_unique<DefendCard>());
    for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<AttackEnhanceCard>());
    for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<HealCard>());
    for (int i = 0; i < 2; ++i) drawPile.push_back(std::make_unique<StrengthCard>());
    drawPile.push_back(std::make_unique<SummonCard>());

    // 洗牌
    std::random_shuffle(drawPile.begin(), drawPile.end());
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
            if (!raw->isAlive()) return;
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
            if (!raw->isAlive()) return;
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
        cmd.effect = [this]() { player.tickStatuses(); };
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
        cmd.effect = [raw]() { if (raw->isAlive()) raw->tickStatuses(); };
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
        cmd.effect = [raw = e.get()]() { if (raw->isAlive()) raw->tickStatuses(); };
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
    std::random_shuffle(drawPile.begin(), drawPile.end());
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

    // 不立即执行，而是挂入代码队列
    Card* rawCard = card;
    Enemy* rawTarget = target;

    // 从卡牌自身获取代码行
    QStringList lines;
    for (auto& s : card->getCodeLines())
        lines << QString::fromStdString(s);

    PendingCodeCommand cmd;
    cmd.title = view.name;
    cmd.lines = lines;
    cmd.effect = [this, rawCard, rawTarget]() { rawCard->play(player, rawTarget); };

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
