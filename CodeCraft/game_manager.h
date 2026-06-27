#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <functional>
#include <vector>
#include <memory>
#include <QString>
#include <QVector>
#include <QStringList>
#include "player.h"
#include "battle.h"
#include "cards.h"

// ============================================================
// game_manager.h — 游戏总管理器
// 回合流程、能量管理、牌组管理、地图系统
// ============================================================

// ============================================================
// 地图系统：DAG 地图节点
// ============================================================

struct MapNode {
    int id = -1;                              // 节点唯一编号
    int depth = 0;                            // 距起点的层数
    bool isStart = false;                     // 是否为起点（无战斗）
    bool isBoss = false;                      // 是否为 Boss 节点
    std::vector<std::string> enemyTypes;      // 敌人类型名称列表
    std::vector<std::unique_ptr<Card>> rewardCards; // 通关后的奖励牌（3张，地图生成时确定）

    MapNode() = default;
    MapNode(MapNode&&) = default;
    MapNode& operator=(MapNode&&) = default;
    MapNode(const MapNode&) = delete;
    MapNode& operator=(const MapNode&) = delete;
};

// ============================================================
// 地图系统：DAG 地图
// edges 为邻接表，edges[fromNodeId] = {toNodeId1, toNodeId2, ...}
// 唯一起点(startNodeId) → 多层战斗节点 → 唯一终点(bossNodeId)
// ============================================================

struct GameMap {
    int seed = 0;
    std::vector<MapNode> nodes;
    std::vector<std::vector<int>> edges;
    int startNodeId = -1;
    int bossNodeId = -1;

    GameMap() = default;
    GameMap(GameMap&&) = default;
    GameMap& operator=(GameMap&&) = default;
    GameMap(const GameMap&) = delete;
    GameMap& operator=(const GameMap&) = delete;

    const MapNode* getNode(int id) const {
        for (auto& n : nodes)
            if (n.id == id) return &n;
        return nullptr;
    }

    std::vector<int> getNextNodes(int fromId) const {
        if (fromId < 0 || fromId >= static_cast<int>(edges.size()))
            return {};
        return edges[fromId];
    }

    int nodeCount() const { return static_cast<int>(nodes.size()); }
    bool isEmpty() const { return nodes.empty(); }

    int maxDepth() const {
        int d = 0;
        for (auto& n : nodes)
            if (n.depth > d) d = n.depth;
        return d;
    }
};

// ---- Qt 桥梁结构体 ----
struct CardView {
    QString name;
    QString description;
    int cost = 0;
    TargetMode targetMode = TargetMode::NONE;
    QStringList codeLines;   // 这张牌打出后会写入主代码块的代码行
};

struct DrawResult {
    bool success = false;
    bool needRecycle = false;
    int handIndex = -1;
    CardView card;
};

struct PlayResult {
    bool success = false;
    int handIndex = -1;
    CardView card;
    QString failReason;
};

struct TurnResult {
    bool gameOver = false;
    bool playerWin = false;
};

struct CodeCommandView {
    QString title;
    QStringList lines;
    bool executed = false;
};

class GameManager {
public:
    // ============================================================
    // 地图系统（静态，跨实例，一次 run 中共享）
    // ============================================================
    static GameMap currentMap;           // 当前 run 的地图
    static int currentNodeId;            // 当前所在节点 ID

    // 在游戏开始时调用一次，根据种子生成整个 DAG 地图
    // 生成后 currentMap 被填充，currentNodeId 设为 startNodeId
    static void generateMap(int seed);

    // 获取当前节点信息（供 UI 显示可用路线）
    static const MapNode* getCurrentMapNode() { return currentMap.getNode(currentNodeId); }
    static std::vector<int> getAvailableNextNodes() { return currentMap.getNextNodes(currentNodeId); }
    static bool hasMap() { return !currentMap.isEmpty(); }

    // ============================================================
    // 关卡系统（兼容旧接口，无地图时回退到此）
    // ============================================================
    static int currentLevel;
    static void setLevel(int level) { currentLevel = level; }
    static int getLevel() { return currentLevel; }

    // ============================================================
    // 卡牌库系统（静态，跨实例，一次 run 中共享）
    // ============================================================
    static constexpr int MAX_COLLECTION_SIZE = 15;
    static std::vector<std::unique_ptr<Card>> cardCollection;  // 固定卡牌库（≤15张）
    static std::vector<std::unique_ptr<Card>> nodeRewards;     // 节点奖励牌（3张）

    // 开局时调用一次，初始化 15 张起始卡牌
    static void initCardCollection();

    // 节点结束后调用，将当前地图节点的 rewardCards 移入 nodeRewards
    static void prepareNodeRewards();

    // 交换：rewardIdx(0~2) ↔ poolIdx(0~cardCollectionSize-1)
    static bool exchangeCard(int rewardIdx, int poolIdx);

    static int cardCollectionSize() { return static_cast<int>(cardCollection.size()); }
    static int nodeRewardsSize()   { return static_cast<int>(nodeRewards.size()); }

    // ============================================================
    // 玩家持久化状态（跨节点保留血量）
    // ============================================================
    static int persistentHp;            // 对局间保留的血量
    static int persistentMaxHp;         // 对局间保留的最大血量
    static bool persistentStateValid;   // 是否有有效的持久化状态

    // 新游戏开始时重置
    static void resetPlayerState();

    // 战斗结束时保存当前血量
    static void savePlayerHp(int hp, int maxHp);

    Player player;
    BattleContext battle;
    int turnNumber = 0;

    // ---- 牌组 ----
    std::vector<std::unique_ptr<Card>> drawPile;
    std::vector<std::unique_ptr<Card>> hand;
    std::vector<std::unique_ptr<Card>> discardPile;

    // 构造函数：nodeId >= 0 时使用地图节点；-1 时自动使用 currentMap 起点或回退到 currentLevel
    explicit GameManager(int nodeId = -1);

    // ---- 回合流程 ----
    void beginTurnWithoutDraw();
    void prepareTurnCodeBlock();
    void prepareAttackCodeBlock();
    void prepareEndCodeBlock();
    std::vector<std::string> getPlayerCodeLines() const;
    std::vector<std::string> getEnemyCodeLines(Enemy* enemy) const;
    std::vector<std::string> getEnemyDescription(Enemy* enemy) const;
    TurnResult finishTurnAfterCodeExecution();
    bool isBattleOver() const { return battle.isBattleOver(); }
    bool isPlayerWin() const { return battle.allEnemiesDead(); }

    // ---- 能量管理 ----
    void growMaxEnergy();
    void restoreEnergy();
    void spendEnergy(int cost);

    // ---- 牌组操作 ----
    DrawResult drawOneCard();
    void recycleDiscardToDrawPile();
    PlayResult playCardAsCode(int handIndex, Enemy* target = nullptr);
    void discardHand();

    // ---- 代码执行模式 ----
    QVector<CodeCommandView> getCodeCommandViews() const;
    int pendingCodeCount() const;
    void executePendingCode(int index);

    // ---- 查询（Qt 只读）----
    QVector<CardView> getHandView() const;
    int getDrawPileCount() const { return static_cast<int>(drawPile.size()); }
    int getDiscardPileCount() const { return static_cast<int>(discardPile.size()); }
    std::string getEnemyIntentText() const;

    // ---- Qt 回调 ----
    std::function<void()> onGameEnd;

private:
    void initLevel();
    void initDeck();

    // ---- 敌人工厂：根据名称字符串创建敌人实例 ----
    static std::unique_ptr<Enemy> createEnemyByName(const std::string& name);

    // ---- 卡牌工厂：根据名称字符串创建卡牌实例 ----
    static std::unique_ptr<Card> createCardByName(const std::string& name);

    // ---- 代码执行模式内部 ----
    enum class CommandSource { PLAYER, MINION, ENEMY, END };

    struct PendingCodeCommand {
        QString title;
        QStringList lines;
        std::function<void()> effect;
        bool executed = false;
        CommandSource source = CommandSource::PLAYER;
    };
    QVector<PendingCodeCommand> pendingCommands;

    QStringList buildEnemyCodeLines(Enemy* enemy) const;
    void insertPlayerCommandBeforeEnemy(PendingCodeCommand cmd);
};

#endif // GAME_MANAGER_H

