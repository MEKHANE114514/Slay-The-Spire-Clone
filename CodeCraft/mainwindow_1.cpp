#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QSizePolicy>
#include <QLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QInputDialog>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QMessageBox>
#include <QResizeEvent>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

namespace {
constexpr int MAIN_EXEC_MS = 800;
constexpr int MAIN_GAP_MS = 450;
constexpr int SIDE_CHANGE_MS = 1400;
constexpr int SIDE_POLL_MS = 80;

const QColor kMainRunColor(210, 70, 0);
const QColor kSideChangeColor(170, 0, 255);
const QColor kCommentColor(80, 150, 80);
const QColor kDamageTextColor(255, 90, 80);
const QColor kHealTextColor(90, 255, 150);
const QColor kShieldTextColor(80, 210, 255);
const QColor kPoisonTextColor(100, 255, 100);
const QColor kPlayerGlowColor(80, 190, 255, 210);
const QColor kEnemyGlowColor(185, 80, 255, 230);
const QColor kMinionGlowColor(90, 220, 255, 190);
constexpr int kBackgroundDimAlpha = 92;

const QString kBattleBgPath   = QStringLiteral(":/images/battle_bg.png");
const QString kPlayerPath     = QStringLiteral(":/images/player.png");
const QString kEnemyPath      = QStringLiteral(":/images/enemy_boss.png");
const QString kMinionPath     = QStringLiteral(":/images/minion.png");
const QString kDrawPilePath   = QStringLiteral(":/images/draw_pile.png");
const QString kDiscardPilePath= QStringLiteral(":/images/discard_pile.png");
const QString kEnergyPath     = QStringLiteral(":/images/energy.png");

QString qstr(const QString& s) { return s; }
QString qstr(const std::string& s) { return QString::fromStdString(s); }

bool isTickBlock(const QStringList& lines)
{
    return !lines.isEmpty() && lines.first().trimmed().contains("tickStatuses");
}

QStringList tickBody(const QStringList& displayedLines)
{
    if (!isTickBlock(displayedLines) || displayedLines.size() < 2) {
        return displayedLines;
    }

    QStringList body;
    for (int i = 1; i + 1 < displayedLines.size(); ++i) {
        QString line = displayedLines[i];
        if (line.startsWith("    ")) {
            line.remove(0, 4);
        }
        body << line;
    }

    if (body.size() == 1 && body.first().trimmed().isEmpty()) {
        body.clear();
    }
    return body;
}

QSet<int> bodyChangesToDisplayedLines(const QSet<int>& bodyChanges,
                                      const QStringList& oldBody,
                                      const QStringList& newBody)
{
    QSet<int> result;
    for (int line : bodyChanges) {
        if (line >= 0 && line < newBody.size()) {
            result.insert(line + 1); // 0 行是 tickStatuses() {
        }
    }

    // 函数体被清空时没有可见文字，改为高亮函数声明行。
    if (result.isEmpty() && !oldBody.isEmpty() && newBody.isEmpty()) {
        result.insert(0);
    }
    return result;
}

struct TickFlags {
    bool player = false;
    bool enemy = false;
};

TickFlags detectTickCall(const CodeCommandView& command)
{
    QString text = command.title + "\n" + command.lines.join("\n");
    text = text.toLower();

    const bool hasTick = text.contains("tickstate")
                         || text.contains("tickstatus")
                         || text.contains("tick_status");
    if (!hasTick) {
        return {};
    }

    TickFlags flags;
    flags.player = text.contains("player");
    flags.enemy = text.contains("enemy") || text.contains("boss");
    return flags;
}

QString enemySummaryText(const MapNode& node)
{
    if (node.isStart) {
        return QStringLiteral("选择下一条路径开始战斗");
    }

    if (node.enemyTypes.empty()) {
        return QStringLiteral("无敌人");
    }

    QStringList enemies;
    for (const std::string& enemy : node.enemyTypes) {
        enemies << QString::fromStdString(enemy);
    }
    return enemies.join(QStringLiteral(" / "));
}

QString mapNodeButtonText(const MapNode& node)
{
    if (node.isStart) {
        return QStringLiteral("起点");
    }
    if (node.isBoss) {
        return QStringLiteral("BOSS\n终点");
    }
    return QStringLiteral("战斗\n第 %1 层").arg(node.depth);
}

QString mapNodeToolTipText(const MapNode& node)
{
    return QStringLiteral("<html><body style='background:#07111C;color:#F7FDFF;'>"
                          "<b>%1</b><br>层数：%2<br>敌人：%3</body></html>")
        .arg(mapNodeButtonText(node).replace("\n", " "))
        .arg(node.depth)
        .arg(enemySummaryText(node));
}

class MapGraphWidget : public QWidget
{
public:
    explicit MapGraphWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet("QWidget { background-color: rgba(3, 7, 13, 80); border: none; }");
    }

    void setMap(const GameMap& newMap,
                int currentId,
                std::function<void(int)> callback)
    {
        // GameMap / MapNode 里现在包含 std::unique_ptr，已经变成 move-only。
        // 地图由 GameManager::currentMap 持有，MapGraphWidget 只读显示它，
        // 所以这里不能再拷贝：map = newMap; 只保存指针即可。
        map = &newMap;
        currentNodeId = currentId;
        onNodeClicked = std::move(callback);
        rebuildButtons();
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        layoutButtons();
        update();
    }

    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);

        if (!map) {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const std::vector<int> nextNodes = map->getNextNodes(currentNodeId);

        for (int from = 0; from < static_cast<int>(map->edges.size()); ++from) {
            if (!buttonById.count(from)) {
                continue;
            }
            const QPoint fromCenter = buttonById[from]->geometry().center();
            for (int to : map->edges[from]) {
                if (!buttonById.count(to)) {
                    continue;
                }
                const QPoint toCenter = buttonById[to]->geometry().center();
                const bool available = (from == currentNodeId)
                                       && (std::find(nextNodes.begin(), nextNodes.end(), to) != nextNodes.end());

                QPen pen;
                if (available) {
                    pen = QPen(QColor(105, 235, 255, 235), 3);
                } else {
                    pen = QPen(QColor(80, 130, 160, 80), 2);
                }
                painter.setPen(pen);
                painter.drawLine(fromCenter, toCenter);

                if (available) {
                    painter.setBrush(QColor(105, 235, 255, 235));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(toCenter, 4, 4);
                }
            }
        }
    }

private:
    const GameMap* map = nullptr;
    int currentNodeId = -1;
    std::function<void(int)> onNodeClicked;
    QVector<QPushButton*> buttons;
    std::map<int, QPushButton*> buttonById;

    void rebuildButtons()
    {
        for (QPushButton* button : buttons) {
            delete button;
        }
        buttons.clear();
        buttonById.clear();

        if (!map) {
            return;
        }

        const std::vector<int> nextNodes = map->getNextNodes(currentNodeId);

        for (const MapNode& node : map->nodes) {
            auto* button = new QPushButton(mapNodeButtonText(node), this);
            button->setToolTip(mapNodeToolTipText(node));
            button->setCursor(Qt::PointingHandCursor);

            const bool isCurrent = node.id == currentNodeId;
            const bool isAvailable = std::find(nextNodes.begin(), nextNodes.end(), node.id) != nextNodes.end();
            button->setEnabled(isAvailable);

            QString border = QStringLiteral("rgba(80, 150, 180, 100)");
            QString textColor = QStringLiteral("#7F9BAA");
            QString bg = QStringLiteral("rgba(8, 13, 24, 155)");

            if (node.isBoss) {
                border = QStringLiteral("rgba(194, 100, 255, 180)");
                textColor = QStringLiteral("#E9D7FF");
            }
            if (isCurrent) {
                border = QStringLiteral("rgba(255, 190, 80, 230)");
                textColor = QStringLiteral("#FFE8B0");
                bg = QStringLiteral("rgba(40, 25, 8, 210)");
                button->setText(button->text() + QStringLiteral("\n当前位置"));
            }
            if (isAvailable) {
                border = QStringLiteral("rgba(105, 235, 255, 235)");
                textColor = QStringLiteral("#F4FEFF");
                bg = QStringLiteral("rgba(8, 30, 42, 220)");
            }

            button->setStyleSheet(QString(
                "QPushButton {"
                "background-color: %1;"
                "color: %2;"
                "border: 2px solid %3;"
                "border-radius: 13px;"
                "font-family: 'Microsoft YaHei UI', 'Segoe UI';"
                "font-size: 13px;"
                "font-weight: 800;"
                "padding: 5px;"
                "}"
                "QPushButton:hover {"
                "background-color: rgba(22, 58, 82, 235);"
                "border: 2px solid #FFFFFF;"
                "}"
                "QPushButton:disabled {"
                "background-color: %1;"
                "color: %2;"
                "border: 2px solid %3;"
                "}"
            ).arg(bg, textColor, border));

            if (isAvailable) {
                connect(button, &QPushButton::clicked, this, [this, id = node.id]() {
                    if (onNodeClicked) {
                        onNodeClicked(id);
                    }
                });
            }

            buttons.push_back(button);
            buttonById[node.id] = button;
        }

        layoutButtons();
        update();
    }

    void layoutButtons()
    {
        if (!map || map->nodes.empty() || buttons.isEmpty()) {
            return;
        }

        std::map<int, QVector<int>> byDepth;
        for (const MapNode& node : map->nodes) {
            byDepth[node.depth].push_back(node.id);
        }

        const int maxDepth = std::max(1, map->maxDepth());
        const int buttonW = 118;
        const int buttonH = 62;
        const int left = 45;
        const int right = 45;
        const int top = 45;
        const int bottom = 45;
        const int usableW = std::max(1, width() - left - right - buttonW);
        const int usableH = std::max(1, height() - top - bottom - buttonH);

        for (const auto& pair : byDepth) {
            const int depth = pair.first;
            const QVector<int>& ids = pair.second;
            const int x = left + static_cast<int>(usableW * (static_cast<double>(depth) / maxDepth));

            const int count = ids.size();
            for (int i = 0; i < count; ++i) {
                const double ratio = (count == 1) ? 0.5 : static_cast<double>(i) / (count - 1);
                const int y = top + static_cast<int>(usableH * ratio);
                auto it = buttonById.find(ids[i]);
                if (it != buttonById.end()) {
                    it->second->setGeometry(x, y, buttonW, buttonH);
                }
            }
        }
    }
};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initCardButtons();
    initCodeEditors();
    initTheme();
    initImageAssets();
    initCharacterContrast();
    initResourceContrast();
    initOverlays();

    // 等 MainWindow 完成 show()、事件循环启动后再弹 seed 输入框。
    // 这样“输入 seed → 生成地图 → 显示地图”的流程更自然。
    QTimer::singleShot(0, this, &MainWindow::startNewGame);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if (ui && ui->battleBackgroundLabel) {
        setScaledPixmap(ui->battleBackgroundLabel, kBattleBgPath, Qt::IgnoreAspectRatio);
    }

    positionOverlays();
}

void MainWindow::initOverlays()
{
    gameOverOverlay = nullptr;
    helpOverlay = nullptr;
    mapOverlay = nullptr;
}

void MainWindow::positionOverlays()
{
    if (!ui || !ui->centralwidget) {
        return;
    }

    const QRect rect = ui->centralwidget->rect();

    if (gameOverOverlay) {
        gameOverOverlay->setGeometry(rect);
    }

    if (helpOverlay) {
        helpOverlay->setGeometry(rect);
    }

    if (mapOverlay) {
        mapOverlay->setGeometry(rect);
    }

    if (rewardOverlay) {
        rewardOverlay->setGeometry(rect);
    }
}

// ============================================================
// 初始化
// ============================================================

void MainWindow::initCardButtons()
{
    cardButtons = {
        ui->cardButton1,
        ui->cardButton2,
        ui->cardButton3,
        ui->cardButton4,
        ui->cardButton5
    };
}

void MainWindow::initCodeEditors()
{
    playerCode.editor = ui->playerTickCodePlainTextEdit;
    enemyCode.editor = ui->enemyTickCodePlainTextEdit;

    setupCodeEditor(ui->codePlainTextEdit);
    setupCodeEditor(playerCode.editor);
    setupCodeEditor(enemyCode.editor);
}

void MainWindow::setupCodeEditor(QPlainTextEdit* editor)
{
    if (!editor) {
        return;
    }

    editor->setReadOnly(true);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont font("Consolas");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(11);
    editor->setFont(font);

    editor->setStyleSheet(
        "QPlainTextEdit {"
        "background-color: rgba(5, 10, 18, 210);"
        "color: #D7F7FF;"
        "border: 1px solid #1E6F86;"
        "border-radius: 8px;"
        "padding: 8px;"
        "selection-background-color: rgba(49, 189, 255, 90);"
        "}"
        );
}

void MainWindow::initTheme()
{
    setStyleSheet(
        "QMainWindow {"
        "background-color: #03070D;"
        "}"
        "QLabel {"
        "color: #D7F7FF;"
        "font-family: 'Microsoft YaHei UI', 'Segoe UI';"
        "font-size: 13px;"
        "}"
        "QLabel#playerHpLabel, QLabel#playerEnergyLabel, QLabel#playerShieldLabel, "
        "QLabel#playerStrengthLabel, QLabel#enemyHpLabel, QLabel#enemyIntentLabel, "
        "QLabel#bossSkillLabel, QLabel#minionHpLabel1, QLabel#minionHpLabel2 {"
        "background-color: rgba(4, 9, 18, 185);"
        "border: 1px solid rgba(45, 198, 255, 150);"
        "border-radius: 8px;"
        "padding: 5px;"
        "}"
        "QLabel#drawPileLabel, QLabel#discardPileLabel {"
        "background-color: rgba(6, 12, 24, 205);"
        "border: 1px solid rgba(92, 231, 255, 175);"
        "border-radius: 10px;"
        "padding: 5px 8px;"
        "font-size: 13px;"
        "font-weight: 700;"
        "}"
        "QLabel#drawPileCountLabel {"
        "background-color: rgba(8, 18, 34, 230);"
        "border: 2px solid rgba(87, 230, 255, 220);"
        "border-radius: 10px;"
        "padding: 3px 8px;"
        "font-size: 16px;"
        "font-weight: 900;"
        "color: #ECFFFF;"
        "}"
        "QLabel#discardPileCountLabel {"
        "background-color: rgba(26, 16, 6, 230);"
        "border: 2px solid rgba(255, 170, 55, 220);"
        "border-radius: 10px;"
        "padding: 3px 8px;"
        "font-size: 16px;"
        "font-weight: 900;"
        "color: #FFF3D8;"
        "}"
        "QLabel#energyValueLabel {"
        "background-color: rgba(8, 18, 34, 235);"
        "border: 2px solid rgba(111, 234, 255, 230);"
        "border-radius: 12px;"
        "padding: 6px 10px;"
        "font-size: 18px;"
        "font-weight: 900;"
        "color: #FFFFFF;"
        "}"
        "QPushButton {"
        "background-color: rgba(10, 18, 30, 220);"
        "color: #E8FBFF;"
        "border: 1px solid #2DC6FF;"
        "border-radius: 8px;"
        "padding: 6px;"
        "font-family: 'Microsoft YaHei UI', 'Segoe UI';"
        "font-size: 13px;"
        "font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(18, 48, 70, 230);"
        "border: 1px solid #8BE9FF;"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(41, 113, 140, 230);"
        "}"
        "QPushButton:disabled {"
        "background-color: rgba(20, 20, 25, 140);"
        "color: #667680;"
        "border: 1px solid #34424C;"
        "}"
        "QLabel#bossSkillLabel {"
        "background-color: rgba(8, 14, 28, 220);"
        "border: 1px solid #B86CFF;"
        "border-radius: 10px;"
        "color: #E9D7FF;"
        "font-weight: 800;"
        "padding: 6px;"
        "}"
        "QLabel#bossSkillLabel:hover {"
        "background-color: rgba(42, 20, 70, 235);"
        "border: 1px solid #FFFFFF;"
        "color: #FFFFFF;"
        "}"
        "QToolTip {"
        "background-color: #07111C;"
        "color: #F7FDFF;"
        "border: 1px solid #6FEAFF;"
        "border-radius: 8px;"
        "padding: 8px;"
        "font-family: 'Microsoft YaHei UI', 'Segoe UI';"
        "font-size: 13px;"
        "}"
        "QTextEdit, QTextBrowser {"
        "background-color: rgba(5, 10, 18, 205);"
        "color: #C9F4FF;"
        "border: 1px solid #1E6F86;"
        "border-radius: 8px;"
        "padding: 6px;"
        "font-family: 'Consolas', 'Microsoft YaHei UI';"
        "font-size: 12px;"
        "}"
    );

    for (QPushButton* btn : cardButtons) {
        if (!btn) {
            continue;
        }
        btn->setMinimumHeight(104);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(cardButtonStyle(QStringLiteral("#6FEAFF"), true));
    }
}

void MainWindow::initImageAssets()
{
    QList<QLabel*> imageLabels = {
        ui->battleBackgroundLabel,
        ui->playerImageLabel,
        ui->enemyImageLabel,
        ui->minionImageLabel1,
        ui->minionImageLabel2,
        ui->drawPileIconLabel,
        ui->discardPileIconLabel,
        ui->energyIconLabel
    };

    for (QLabel* label : imageLabels) {
        if (!label) {
            continue;
        }

        label->setText("");
        label->setFrameShape(QFrame::NoFrame);
        label->setFrameShadow(QFrame::Plain);
        label->setLineWidth(0);
        label->setMidLineWidth(0);
        label->setAutoFillBackground(false);
        label->setAttribute(Qt::WA_TranslucentBackground, true);
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->setStyleSheet("QLabel { background: transparent; border: none; }");
        label->setAlignment(Qt::AlignCenter);
        label->setScaledContents(false);
    }

    setScaledPixmap(ui->battleBackgroundLabel, kBattleBgPath, Qt::IgnoreAspectRatio);
    ui->battleBackgroundLabel->lower();

    setScaledPixmap(ui->playerImageLabel, kPlayerPath);
    setScaledPixmap(ui->enemyImageLabel, kEnemyPath);
    setScaledPixmap(ui->minionImageLabel1, kMinionPath);
    setScaledPixmap(ui->minionImageLabel2, kMinionPath);

    setScaledPixmap(ui->drawPileIconLabel, kDrawPilePath);
    setScaledPixmap(ui->discardPileIconLabel, kDiscardPilePath);
    setScaledPixmap(ui->energyIconLabel, kEnergyPath);
}


void MainWindow::initCharacterContrast()
{
    applyActorImageStyle(ui->playerImageLabel, kPlayerGlowColor, 34);
    applyActorImageStyle(ui->enemyImageLabel,  kEnemyGlowColor,  44);
    applyActorImageStyle(ui->minionImageLabel1, kMinionGlowColor, 24);
    applyActorImageStyle(ui->minionImageLabel2, kMinionGlowColor, 24);

    // 图标类控件保持透明，不加大面积底板。
    for (QLabel* label : { ui->drawPileIconLabel, ui->discardPileIconLabel, ui->energyIconLabel }) {
        if (!label) {
            continue;
        }
        label->setStyleSheet("QLabel { background: transparent; border: none; }");
    }
}

void MainWindow::initResourceContrast()
{
    applyResourceIconStyle(ui->drawPileIconLabel, QColor(72, 222, 255, 220), 22, false);
    applyResourceIconStyle(ui->discardPileIconLabel, QColor(255, 165, 60, 210), 20, false);
    applyResourceIconStyle(ui->energyIconLabel, QColor(120, 235, 255, 235), 28, true);

    applyResourceCountStyle(ui->drawPileCountLabel, QColor(72, 222, 255, 220));
    applyResourceCountStyle(ui->discardPileCountLabel, QColor(255, 165, 60, 220));
    applyResourceCountStyle(ui->energyValueLabel, QColor(111, 234, 255, 235));

    if (ui->drawPileLabel) {
        auto* effect = new QGraphicsDropShadowEffect(ui->drawPileLabel);
        effect->setBlurRadius(16);
        effect->setOffset(0, 0);
        effect->setColor(QColor(72, 222, 255, 120));
        ui->drawPileLabel->setGraphicsEffect(effect);
    }
    if (ui->discardPileLabel) {
        auto* effect = new QGraphicsDropShadowEffect(ui->discardPileLabel);
        effect->setBlurRadius(16);
        effect->setOffset(0, 0);
        effect->setColor(QColor(255, 165, 60, 120));
        ui->discardPileLabel->setGraphicsEffect(effect);
    }
}

void MainWindow::applyActorImageStyle(QLabel* label, const QColor& glowColor, int blurRadius)
{
    if (!label) {
        return;
    }

    label->setStyleSheet(
        "QLabel {"
        "background-color: qradialgradient(cx:0.50, cy:0.66, radius:0.58, "
        "fx:0.50, fy:0.66, "
        "stop:0 rgba(95, 210, 255, 70), "
        "stop:0.42 rgba(40, 95, 160, 42), "
        "stop:0.72 rgba(8, 14, 28, 96), "
        "stop:1 rgba(0, 0, 0, 0));"
        "border: none;"
        "}"
    );

    auto* effect = new QGraphicsDropShadowEffect(label);
    effect->setBlurRadius(blurRadius);
    effect->setOffset(0, 0);
    effect->setColor(glowColor);
    label->setGraphicsEffect(effect);
}

void MainWindow::applyResourceIconStyle(QLabel* label, const QColor& glowColor, int blurRadius, bool circular)
{
    if (!label) {
        return;
    }

    const QString radiusPart = circular ? "18px" : "14px";
    label->setStyleSheet(QString(
        "QLabel {"
        "background-color: qradialgradient(cx:0.50, cy:0.50, radius:0.62, "
        "fx:0.50, fy:0.50, "
        "stop:0 rgba(255, 255, 255, 20), "
        "stop:0.34 rgba(110, 215, 255, 36), "
        "stop:0.70 rgba(8, 18, 32, 128), "
        "stop:1 rgba(0, 0, 0, 0));"
        "border: 1px solid rgba(105, 225, 255, 90);"
        "border-radius: %1;"
        "padding: 4px;"
        "}"
    ).arg(radiusPart));

    auto* effect = new QGraphicsDropShadowEffect(label);
    effect->setBlurRadius(blurRadius);
    effect->setOffset(0, 0);
    effect->setColor(glowColor);
    label->setGraphicsEffect(effect);
}

void MainWindow::applyResourceCountStyle(QLabel* label, const QColor& glowColor)
{
    if (!label) {
        return;
    }

    auto* effect = new QGraphicsDropShadowEffect(label);
    effect->setBlurRadius(18);
    effect->setOffset(0, 0);
    effect->setColor(glowColor);
    label->setGraphicsEffect(effect);
    label->setAlignment(Qt::AlignCenter);
}

void MainWindow::resetRuntimeState()
{
    hideGameResultOverlay();
    hideHelpOverlay();
    hideMapOverlay();
    hideRewardOverlay();

    ++executionToken;
    ++sideHighlightToken;

    logs.clear();
    codeRanges.clear();
    sideHighlightQueue.clear();

    activeCodeIndex = -1;
    controlsEnabled = false;
    sideChangeHighlightActive = false;

    playerCode.displayedLines.clear();
    playerCode.bodyLines.clear();
    playerCode.changedLines.clear();
    playerCode.executingLines.clear();

    enemyCode.displayedLines.clear();
    enemyCode.bodyLines.clear();
    enemyCode.changedLines.clear();
    enemyCode.executingLines.clear();

    // 重新开始时要把旧游戏留下的文本格式也清掉。
    // 否则 refreshSideCodeEditors() 会把旧文本误判为“发生变化”，
    // 导致点击重新开始后两侧代码块立刻出现函数修改高亮。
    auto resetEditor = [](QPlainTextEdit* editor) {
        if (!editor) {
            return;
        }
        editor->clear();
        editor->setExtraSelections({});
    };

    resetEditor(ui ? ui->codePlainTextEdit : nullptr);
    resetEditor(playerCode.editor);
    resetEditor(enemyCode.editor);
}

void MainWindow::startNewGame()
{
    resetRuntimeState();

    // 一局游戏开始前，玩家必须先输入 seed。
    // seed 交给 GameManager::generateMap(seed)，由 GameManager 生成 DAG 地图。
    gameManager.reset();
    GameManager::resetPlayerState();
    GameManager::initCardCollection();

    const int defaultSeed = static_cast<int>(QRandomGenerator::global()->bounded(1, 1000000));
    bool ok = false;

    const int seed = QInputDialog::getInt(
        this,
        QStringLiteral("输入地图 Seed"),
        QStringLiteral("请输入整数 seed，用于生成本局 DAG 地图："),
        defaultSeed,
        1,
        999999999,
        1,
        &ok
    );

    if (!ok) {
        clearLogs();
        clearCodeHighlight();
        clearSideChangeHighlight();
        appendLog(QStringLiteral("已取消输入 seed。本局尚未开始，点击“重新开始”可重新输入 seed。"));

        setControlsEnabled(false);
        ui->endTurnButton->setEnabled(false);
        ui->restartButton->setEnabled(true);
        ui->helpButton->setEnabled(true);
        return;
    }

    GameManager::generateMap(seed);

    clearLogs();
    clearCodeHighlight();
    clearSideChangeHighlight();
    appendLog(QString("新的 DAG 地图已生成，seed = %1。请先在地图中选择一个战斗节点。").arg(seed));

    // 地图阶段不允许出牌/结束回合，但允许重新开始和查看规则。
    setControlsEnabled(false);
    ui->endTurnButton->setEnabled(false);
    ui->restartButton->setEnabled(true);
    ui->helpButton->setEnabled(true);

    // 输入 seed 并生成地图后，再显示地图覆盖层。
    QTimer::singleShot(0, this, [this]() {
        if (!gameManager && GameManager::hasMap()) {
            showMapOverlay();
        }
    });
}


// ============================================================
// 日志
// ============================================================

void MainWindow::appendLog(const QString& text)
{
    logs << text;
    refreshLogUi();
}

void MainWindow::clearLogs()
{
    logs.clear();
    refreshLogUi();
}

void MainWindow::refreshLogUi()
{
    ui->logTextEdit->setReadOnly(true);
    ui->logTextEdit->setPlainText(logs.join("\n"));

    QTextCursor cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logTextEdit->setTextCursor(cursor);
}

// ============================================================
// 回合流程
// ============================================================

void MainWindow::beginTurnWithoutAutoDraw()
{
    if (!gameManager) {
        return;
    }

    gameManager->beginTurnWithoutDraw();
    gameManager->prepareTurnCodeBlock();

    appendLog(QString("第 %1 回合开始。").arg(gameManager->turnNumber));
    appendLog("本回合函数调用已写入代码块，开始抽牌。");

    refreshUi();
    refreshMainCodeEditor();
}

void MainWindow::startTurnDrawFive()
{
    setControlsEnabled(false);
    drawNextCard(DEFAULT_DRAW_PER_TURN);
}

void MainWindow::drawNextCard(int remainingCount)
{
    if (!gameManager) {
        return;
    }

    // 抽牌/洗牌动画是异步的。若动画过程中重新开始、返回地图或进入新节点，
    // resetRuntimeState() 会改变 executionToken，旧动画回调必须停止，
    // 否则可能在旧 GameManager 已销毁后继续刷新手牌导致崩溃。
    const int token = executionToken;

    if (remainingCount <= 0) {
        appendLog("抽牌阶段结束。请出牌，出牌只会写入代码块，暂不立即结算。");
        refreshUi();
        refreshMainCodeEditor();
        setControlsEnabled(true);
        return;
    }

    if (static_cast<int>(gameManager->getHandView().size()) >= cardButtons.size()) {
        appendLog("手牌已满，停止抽牌。");
        refreshUi();
        refreshMainCodeEditor();
        setControlsEnabled(true);
        return;
    }

    DrawResult result = gameManager->drawOneCard();

    if (result.needRecycle) {
        appendLog("抽牌堆为空，弃牌堆放回抽牌堆。");
        recycleDiscardToDrawPileAnimation([this, remainingCount, token]() {
            if (token != executionToken || !gameManager) {
                return;
            }
            gameManager->recycleDiscardToDrawPile();
            refreshUi();
            refreshMainCodeEditor();
            setControlsEnabled(false);
            drawNextCard(remainingCount);
        });
        return;
    }

    if (!result.success) {
        appendLog("没有牌可抽。");
        refreshUi();
        refreshMainCodeEditor();
        setControlsEnabled(true);
        return;
    }

    appendLog(QString("抽到【%1】。").arg(qstr(result.card.name)));
    refreshPileUi();

    drawOneCardAnimation(result.handIndex, result.card, [this, remainingCount, token]() {
        if (token != executionToken || !gameManager) {
            return;
        }
        refreshUi();
        refreshMainCodeEditor();
        setControlsEnabled(false);
        drawNextCard(remainingCount - 1);
    });
}

// ============================================================
// 出牌 / 代码执行
// ============================================================

void MainWindow::playCardByIndex(int index)
{
    if (!gameManager || !controlsEnabled) {
        return;
    }

    auto handView = gameManager->getHandView();
    if (index < 0 || index >= static_cast<int>(handView.size())) {
        return;
    }

    CardView card = handView[index];
    PlayResult result = gameManager->playCardAsCode(index, firstAliveEnemy());

    if (!result.success) {
        appendLog(QString("无法写入代码：%1").arg(qstr(result.failReason)));
        refreshUi();
        refreshMainCodeEditor();
        return;
    }

    appendLog(QString("玩家打出【%1】，对应代码已写入代码块。").arg(qstr(result.card.name)));

    const int token = executionToken;
    playCardToDiscardAnimation(index, card, [this, token]() {
        if (token != executionToken || !gameManager) {
            return;
        }
        refreshUi();
        refreshMainCodeEditor();
        if (!gameManager->isBattleOver()) {
            setControlsEnabled(true);
        }
    });
}

void MainWindow::on_endTurnButton_clicked()
{
    if (!gameManager || !controlsEnabled) {
        return;
    }

    setControlsEnabled(false);
    appendLog("玩家结束回合。剩余手牌进入弃牌堆。");

    for (const CardView& card : gameManager->getHandView()) {
        appendLog(QString("【%1】进入弃牌堆。").arg(qstr(card.name)));
    }

    gameManager->discardHand();
    refreshUi();
    refreshMainCodeEditor();

    appendLog("开始按顺序执行代码块。");
    executeCodeQueue();
}

void MainWindow::executeCodeQueue()
{
    executeNextCode(0, ++executionToken);
}

void MainWindow::executeNextCode(int index, int token)
{
    if (token != executionToken || !gameManager) {
        return;
    }

    if (sideChangeHighlightActive || !sideHighlightQueue.isEmpty()) {
        if (!sideChangeHighlightActive) {
            startNextSideChangeHighlight();
        }
        QTimer::singleShot(SIDE_POLL_MS, this, [this, index, token]() {
            executeNextCode(index, token);
        });
        return;
    }

    if (index >= gameManager->pendingCodeCount()) {
        clearCodeHighlight();
        TurnResult result = gameManager->finishTurnAfterCodeExecution();
        refreshUi();
        refreshMainCodeEditor();

        const bool noAliveEnemy = (firstAliveEnemy() == nullptr);
        const bool playerAlive = gameManager->player.isAlive();
        if (result.gameOver || gameManager->isBattleOver() || (playerAlive && noAliveEnemy)) {
            showGameOverMessage();
            return;
        }

        beginTurnWithoutAutoDraw();
        startTurnDrawFive();
        return;
    }

    highlightCodeBlock(index);

    QTimer::singleShot(MAIN_EXEC_MS, this, [this, index, token]() {
        if (token != executionToken || !gameManager) {
            return;
        }

        const int oldPlayerHp = gameManager->player.hp;
        const int oldPlayerShield = gameManager->player.shield;

        Enemy* oldEnemy = firstAliveEnemy();
        const int oldEnemyHp = oldEnemy ? oldEnemy->hp : 0;
        const int oldEnemyShield = oldEnemy ? oldEnemy->shield : 0;

        CodeCommandView command;
        const QVector<CodeCommandView> commands = gameManager->getCodeCommandViews();
        if (index >= 0 && index < commands.size()) {
            command = commands[index];
        }

        gameManager->executePendingCode(index);
        refreshUi();
        refreshMainCodeEditor();
        playCommandFeedback(command, oldPlayerHp, oldPlayerShield, oldEnemyHp, oldEnemyShield);

        const bool noAliveEnemy = (firstAliveEnemy() == nullptr);
        const bool playerAlive = gameManager->player.isAlive();
        if (gameManager->isBattleOver() || (playerAlive && noAliveEnemy)) {
            clearCodeHighlight();
            showGameOverMessage();
            return;
        }

        QTimer::singleShot(MAIN_GAP_MS, this, [this, index, token]() {
            executeNextCode(index + 1, token);
        });
    });
}

void MainWindow::showGameOverMessage()
{
    clearCodeHighlight();

    if (gameManager) {
        refreshUi();
        refreshMainCodeEditor();
    }

    // 不再依赖 TurnResult 或 isPlayerWin() 单一路径。
    // 对地图模式来说，“玩家还活着 + 当前没有存活敌人”就是本节点胜利。
    const bool playerAlive = gameManager && gameManager->player.isAlive();
    const bool noAliveEnemy = gameManager && (firstAliveEnemy() == nullptr);
    const bool battleWon = playerAlive && noAliveEnemy;
    const bool battleLost = gameManager && !gameManager->player.isAlive();

    if (battleWon && GameManager::hasMap()) {
        const MapNode* node = GameManager::getCurrentMapNode();

        // 普通战斗节点胜利：先进入抽卡/换牌奖励环节，完成后再回地图。
        if (node && !node->isBoss) {
            ++executionToken;       // 让旧的代码执行/动画回调全部失效
            ++sideHighlightToken;
            clearCodeHighlight();
            clearSideChangeHighlight();

            GameManager::savePlayerHp(gameManager->player.hp, gameManager->player.maxHp);
            GameManager::prepareNodeRewards();

            appendLog(QStringLiteral("战斗胜利，节点 %1 已完成。进入抽卡交换环节。").arg(node->id));

            setControlsEnabled(false);
            ui->restartButton->setEnabled(true);
            ui->helpButton->setEnabled(true);

            QTimer::singleShot(250, this, [this]() {
                if (GameManager::getNodeRewardView().isEmpty()) {
                    finishRewardAndShowMap();
                } else {
                    showRewardOverlay();
                }
            });
            return;
        }

        // Boss 节点胜利：整局胜利。
        showGameResultOverlay(true);
        setControlsEnabled(false);
        ui->restartButton->setEnabled(true);
        ui->helpButton->setEnabled(true);
        return;
    }

    // 玩家死亡：整局失败。
    if (battleLost) {
        showGameResultOverlay(false);
        setControlsEnabled(false);
        ui->restartButton->setEnabled(true);
        ui->helpButton->setEnabled(true);
        return;
    }

    // 兜底：保持旧逻辑，避免其他异常状态卡住。
    const bool playerWin = gameManager
        && gameManager->player.isAlive()
        && (gameManager->isPlayerWin() || gameManager->isBattleOver());

    showGameResultOverlay(playerWin);
    setControlsEnabled(false);
    ui->restartButton->setEnabled(true);
    ui->helpButton->setEnabled(true);
}


void MainWindow::showRewardOverlay()
{
    hideRewardOverlay();

    if (!ui || !ui->centralwidget) {
        return;
    }

    rewardOverlay = new QWidget(ui->centralwidget);
    rewardOverlay->setObjectName("rewardOverlay");
    rewardOverlay->setAttribute(Qt::WA_StyledBackground, true);
    rewardOverlay->setGeometry(ui->centralwidget->rect());
    rewardOverlay->setStyleSheet(
        "QWidget#rewardOverlay {"
        "background-color: rgba(0, 0, 0, 178);"
        "}"
        "QWidget#rewardCard {"
        "background-color: rgba(4, 9, 18, 242);"
        "border: 2px solid rgba(255, 214, 107, 220);"
        "border-radius: 18px;"
        "}"
        "QLabel#rewardTitleLabel {"
        "color: #FFE28A;"
        "font-size: 30px;"
        "font-weight: 900;"
        "letter-spacing: 2px;"
        "}"
        "QLabel#rewardSubLabel {"
        "color: #D7F7FF;"
        "font-size: 14px;"
        "font-weight: 700;"
        "}"
        "QLabel#rewardColumnTitle {"
        "color: #F4FEFF;"
        "font-size: 17px;"
        "font-weight: 900;"
        "}"
        "QLabel#rewardStatusLabel {"
        "color: #A9EFFF;"
        "font-size: 13px;"
        "font-weight: 700;"
        "}"
        "QPushButton#rewardBottomButton {"
        "background-color: rgba(10, 18, 30, 230);"
        "color: #E8FBFF;"
        "border: 1px solid #2DC6FF;"
        "border-radius: 8px;"
        "padding: 8px 18px;"
        "font-weight: 800;"
        "}"
        "QPushButton#rewardBottomButton:hover {"
        "background-color: rgba(25, 60, 82, 235);"
        "border: 1px solid #FFFFFF;"
        "}"
    );

    struct RewardUiState {
        int selectedPool = -1;
        int selectedReward = -1;
        int exchangeCount = 0;
        bool animating = false;
        QSet<int> usedRewardIndices;
        QGridLayout* poolGrid = nullptr;
        QGridLayout* rewardGrid = nullptr;
        QLabel* statusLabel = nullptr;
        QWidget* cardPanel = nullptr;
        QVector<QPushButton*> poolButtons;
        QVector<QPushButton*> rewardButtons;
    };

    auto state = std::make_shared<RewardUiState>();

    auto* outer = new QVBoxLayout(rewardOverlay);
    outer->setContentsMargins(70, 50, 70, 50);

    auto* card = new QWidget(rewardOverlay);
    card->setObjectName("rewardCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    state->cardPanel = card;

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(26, 20, 26, 20);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("CARD REWARD / 抽卡交换"), card);
    title->setObjectName("rewardTitleLabel");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto* sub = new QLabel(QStringLiteral("左侧是当前牌库，右侧是本层备选奖励。先选左侧要替换的牌，再选右侧奖励牌进行交换，最多交换 3 次。"), card);
    sub->setObjectName("rewardSubLabel");
    sub->setAlignment(Qt::AlignCenter);
    sub->setWordWrap(true);
    layout->addWidget(sub);

    auto* columns = new QHBoxLayout();
    columns->setSpacing(18);

    auto makeColumn = [&](const QString& titleText, QGridLayout** gridOut) -> QWidget* {
        auto* col = new QWidget(card);
        auto* colLayout = new QVBoxLayout(col);
        colLayout->setContentsMargins(0, 0, 0, 0);
        colLayout->setSpacing(8);

        auto* label = new QLabel(titleText, col);
        label->setObjectName("rewardColumnTitle");
        label->setAlignment(Qt::AlignCenter);
        colLayout->addWidget(label);

        auto* scroll = new QScrollArea(col);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet(
            "QScrollArea { background-color: rgba(2, 6, 12, 120); border: 1px solid rgba(105, 235, 255, 85); border-radius: 12px; }"
            "QScrollBar:vertical { background: rgba(0,0,0,70); width: 10px; margin: 0px; }"
            "QScrollBar::handle:vertical { background: rgba(105,235,255,160); border-radius: 5px; }"
        );

        auto* body = new QWidget(scroll);
        auto* grid = new QGridLayout(body);
        grid->setContentsMargins(10, 10, 10, 10);
        grid->setSpacing(10);
        *gridOut = grid;
        scroll->setWidget(body);
        colLayout->addWidget(scroll, 1);
        return col;
    };

    columns->addWidget(makeColumn(QStringLiteral("当前已有牌 / Card Collection"), &state->poolGrid), 3);
    columns->addWidget(makeColumn(QStringLiteral("本层备选牌 / Rewards"), &state->rewardGrid), 2);
    layout->addLayout(columns, 1);

    state->statusLabel = new QLabel(card);
    state->statusLabel->setObjectName("rewardStatusLabel");
    state->statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(state->statusLabel);

    auto* bottom = new QHBoxLayout();
    bottom->addStretch();

    auto* finish = new QPushButton(QStringLiteral("完成选择，返回地图"), card);
    finish->setObjectName("rewardBottomButton");
    auto* skip = new QPushButton(QStringLiteral("跳过奖励"), card);
    skip->setObjectName("rewardBottomButton");
    auto* help = new QPushButton(QStringLiteral("查看规则"), card);
    help->setObjectName("rewardBottomButton");

    bottom->addWidget(skip);
    bottom->addWidget(finish);
    bottom->addWidget(help);
    bottom->addStretch();
    layout->addLayout(bottom);

    auto clearLayout = [](QLayout* target) {
        while (QLayoutItem* item = target->takeAt(0)) {
            if (QWidget* w = item->widget()) {
                w->deleteLater();
            }
            delete item;
        }
    };

    auto makeRewardButtonStyle = [this](const CardView& view, bool selected, bool disabled) {
        const QString accent = cardAccentColor(view);
        const QString borderColor = selected ? QStringLiteral("#FFD66B") : accent;
        const int borderWidth = selected ? 3 : 1;
        const QString opacityBg = disabled ? QStringLiteral("rgba(40, 40, 45, 130)")
                                          : QStringLiteral("rgba(7, 14, 26, 230)");
        const QString textColor = disabled ? QStringLiteral("#7E8790") : QStringLiteral("#F7FDFF");
        return QString(
            "QPushButton {"
            "text-align: left;"
            "color: %1;"
            "background-color: %2;"
            "border: %3px solid %4;"
            "border-radius: 10px;"
            "padding: 8px;"
            "font-family: 'Microsoft YaHei UI', 'Segoe UI';"
            "font-weight: 800;"
            "font-size: 13px;"
            "}"
            "QPushButton:hover {"
            "background-color: rgba(20, 38, 58, 240);"
            "border: 2px solid #FFFFFF;"
            "}"
        ).arg(textColor).arg(opacityBg).arg(borderWidth).arg(borderColor);
    };

    auto rebuild = std::make_shared<std::function<void()>>();
    auto doExchange = std::make_shared<std::function<void(int, int)>>();

    *rebuild = [this, state, clearLayout, makeRewardButtonStyle, rebuild, doExchange]() {
        clearLayout(state->poolGrid);
        clearLayout(state->rewardGrid);
        state->poolButtons.clear();
        state->rewardButtons.clear();

        const QVector<CardView> poolCards = GameManager::getCardCollectionView();
        const QVector<CardView> rewardCards = GameManager::getNodeRewardView();

        state->statusLabel->setText(QStringLiteral("已交换 %1 / 3 次。%2")
                                        .arg(state->exchangeCount)
                                        .arg(state->exchangeCount >= 3
                                                 ? QStringLiteral("已达到本层交换上限，可返回地图。")
                                                 : QStringLiteral("请选择左侧一张牌和右侧一张奖励牌。")));

        for (int i = 0; i < poolCards.size(); ++i) {
            const CardView view = poolCards[i];
            auto* btn = new QPushButton(formatCardText(view), rewardOverlay);
            btn->setToolTip(cardToolTipText(view));
            btn->setMinimumHeight(118);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            btn->setStyleSheet(makeRewardButtonStyle(view, state->selectedPool == i, false));
            state->poolButtons.push_back(btn);
            state->poolGrid->addWidget(btn, i / 3, i % 3);

            connect(btn, &QPushButton::clicked, this, [this, state, i, rebuild, doExchange]() {
                if (state->animating || state->exchangeCount >= 3) {
                    return;
                }
                state->selectedPool = i;
                if (state->selectedReward >= 0) {
                    (*doExchange)(state->selectedReward, state->selectedPool);
                    return;
                }
                if (state->statusLabel) {
                    state->statusLabel->setText(QStringLiteral("已选择左侧第 %1 张牌。现在请选择右侧奖励牌。")
                                                    .arg(i + 1));
                }
                (*rebuild)();
            });
        }

        for (int i = 0; i < rewardCards.size(); ++i) {
            const CardView view = rewardCards[i];
            const bool used = state->usedRewardIndices.contains(i);
            auto* btn = new QPushButton(formatCardText(view), rewardOverlay);
            btn->setToolTip(cardToolTipText(view));
            btn->setMinimumHeight(132);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            btn->setEnabled(!used && state->exchangeCount < 3);
            btn->setStyleSheet(makeRewardButtonStyle(view, state->selectedReward == i, used));
            if (used) {
                btn->setText(QStringLiteral("[已交换]\n") + btn->text());
            }
            state->rewardButtons.push_back(btn);
            state->rewardGrid->addWidget(btn, i, 0);

            connect(btn, &QPushButton::clicked, this, [this, state, i, rebuild, doExchange]() {
                if (state->animating || state->exchangeCount >= 3 || state->usedRewardIndices.contains(i)) {
                    return;
                }
                state->selectedReward = i;
                if (state->selectedPool >= 0) {
                    (*doExchange)(state->selectedReward, state->selectedPool);
                    return;
                }
                if (state->statusLabel) {
                    state->statusLabel->setText(QStringLiteral("已选择右侧第 %1 张奖励牌。现在请选择左侧一张要换出的牌。")
                                                    .arg(i + 1));
                }
                (*rebuild)();
            });
        }
    };

    *doExchange = [this, state, rebuild](int rewardIdx, int poolIdx) {
        if (state->animating || state->exchangeCount >= 3) {
            return;
        }
        if (rewardIdx < 0 || rewardIdx >= state->rewardButtons.size()
            || poolIdx < 0 || poolIdx >= state->poolButtons.size()) {
            return;
        }

        state->animating = true;
        state->cardPanel->setEnabled(false);

        QPushButton* rewardBtn = state->rewardButtons[rewardIdx];
        QPushButton* poolBtn = state->poolButtons[poolIdx];
        const QVector<CardView> rewards = GameManager::getNodeRewardView();
        const QVector<CardView> pools = GameManager::getCardCollectionView();
        const CardView rewardView = (rewardIdx >= 0 && rewardIdx < rewards.size()) ? rewards[rewardIdx] : CardView{};
        const CardView poolView = (poolIdx >= 0 && poolIdx < pools.size()) ? pools[poolIdx] : CardView{};

        animateGhost(geometryInCentral(rewardBtn), geometryInCentral(poolBtn),
                     formatCardText(rewardView), cardToolTipText(rewardView),
                     520, QEasingCurve::OutCubic, nullptr);
        animateGhost(geometryInCentral(poolBtn), geometryInCentral(rewardBtn),
                     formatCardText(poolView), cardToolTipText(poolView),
                     520, QEasingCurve::OutCubic, nullptr);

        QTimer::singleShot(560, this, [this, state, rewardIdx, poolIdx, rebuild]() mutable {
            if (!rewardOverlay) {
                return;
            }

            const bool ok = GameManager::exchangeCard(rewardIdx, poolIdx);
            if (ok) {
                state->usedRewardIndices.insert(rewardIdx);
                state->exchangeCount++;
                appendLog(QStringLiteral("抽卡交换：右侧第 %1 张奖励牌 ↔ 左侧第 %2 张已有牌。")
                              .arg(rewardIdx + 1)
                              .arg(poolIdx + 1));
            }

            state->selectedPool = -1;
            state->selectedReward = -1;
            state->animating = false;
            state->cardPanel->setEnabled(true);
            (*rebuild)();
        });
    };

    connect(finish, &QPushButton::clicked, this, [this]() {
        finishRewardAndShowMap();
    });
    connect(skip, &QPushButton::clicked, this, [this]() {
        appendLog(QStringLiteral("跳过本层抽卡交换奖励。"));
        finishRewardAndShowMap();
    });
    connect(help, &QPushButton::clicked, this, [this]() {
        showHelpOverlay();
    });

    (*rebuild)();

    outer->addWidget(card);
    rewardOverlay->show();
    rewardOverlay->raise();
    positionOverlays();
}

void MainWindow::hideRewardOverlay()
{
    if (!rewardOverlay) {
        return;
    }

    QWidget* overlay = rewardOverlay;
    rewardOverlay = nullptr;
    overlay->hide();
    overlay->deleteLater();
}

void MainWindow::finishRewardAndShowMap()
{
    hideRewardOverlay();
    appendLog(QStringLiteral("奖励环节结束，返回地图。"));
    setControlsEnabled(false);
    ui->restartButton->setEnabled(true);
    ui->helpButton->setEnabled(true);

    QTimer::singleShot(180, this, [this]() {
        showMapOverlay();
    });
}

void MainWindow::showMapOverlay()
{
    hideMapOverlay();

    if (!ui || !ui->centralwidget) {
        return;
    }

    if (!GameManager::hasMap()) {
        GameManager::generateMap(static_cast<int>(QRandomGenerator::global()->bounded(1, 1000000)));
    }

    mapOverlay = new QWidget(ui->centralwidget);
    mapOverlay->setObjectName("mapOverlay");
    mapOverlay->setAttribute(Qt::WA_StyledBackground, true);
    mapOverlay->setGeometry(ui->centralwidget->rect());
    mapOverlay->setStyleSheet(
        "QWidget#mapOverlay {"
        "background-color: rgba(0, 0, 0, 172);"
        "}"
        "QWidget#mapCard {"
        "background-color: rgba(4, 9, 18, 238);"
        "border: 2px solid rgba(105, 225, 255, 210);"
        "border-radius: 18px;"
        "}"
        "QLabel#mapTitleLabel {"
        "color: #F4FEFF;"
        "font-size: 30px;"
        "font-weight: 900;"
        "letter-spacing: 2px;"
        "}"
        "QLabel#mapSubTitleLabel {"
        "color: #A9EFFF;"
        "font-size: 14px;"
        "font-weight: 700;"
        "}"
        "QPushButton#mapCloseButton {"
        "background-color: rgba(10, 18, 30, 220);"
        "color: #E8FBFF;"
        "border: 1px solid #2DC6FF;"
        "border-radius: 8px;"
        "padding: 8px 18px;"
        "font-weight: 800;"
        "}"
        "QPushButton#mapCloseButton:hover {"
        "background-color: rgba(25, 60, 82, 235);"
        "border: 1px solid #FFFFFF;"
        "}"
    );

    auto* outer = new QVBoxLayout(mapOverlay);
    outer->setContentsMargins(90, 60, 90, 60);

    auto* card = new QWidget(mapOverlay);
    card->setObjectName("mapCard");
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(28, 22, 28, 22);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("CODE MAP / DAG 路线图"), card);
    title->setObjectName("mapTitleLabel");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    const MapNode* currentNode = GameManager::getCurrentMapNode();
    const QString currentName = currentNode ? mapNodeButtonText(*currentNode).replace("\n", " ") : QStringLiteral("未知");
    auto* subtitle = new QLabel(
        QStringLiteral("当前节点：%1    可选择高亮节点进入下一场战斗；击败 Boss 后整局胜利。")
            .arg(currentName),
        card);
    subtitle->setObjectName("mapSubTitleLabel");
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    auto* graph = new MapGraphWidget(card);
    graph->setMinimumHeight(520);
    graph->setMap(GameManager::currentMap, GameManager::currentNodeId,
                  [this](int nodeId) { enterMapNode(nodeId); });
    layout->addWidget(graph, 1);

    auto* bottom = new QHBoxLayout();
    bottom->addStretch();

    auto* restart = new QPushButton(QStringLiteral("重新开始整局"), card);
    restart->setObjectName("mapCloseButton");
    connect(restart, &QPushButton::clicked, this, [this]() {
        startNewGame();
    });
    bottom->addWidget(restart);

    auto* help = new QPushButton(QStringLiteral("查看规则"), card);
    help->setObjectName("mapCloseButton");
    connect(help, &QPushButton::clicked, this, [this]() {
        showHelpOverlay();
    });
    bottom->addWidget(help);

    bottom->addStretch();
    layout->addLayout(bottom);

    outer->addWidget(card);
    mapOverlay->show();
    mapOverlay->raise();
    positionOverlays();
}

void MainWindow::hideMapOverlay()
{
    if (!mapOverlay) {
        return;
    }
    mapOverlay->deleteLater();
    mapOverlay = nullptr;
}

void MainWindow::enterMapNode(int nodeId)
{
    resetRuntimeState();
    gameManager = std::make_unique<GameManager>(nodeId);

    const MapNode* node = GameManager::getCurrentMapNode();
    const QString nodeName = node ? mapNodeButtonText(*node).replace("\n", " ") : QStringLiteral("未知节点");

    clearLogs();
    clearCodeHighlight();
    clearSideChangeHighlight();
    appendLog(QStringLiteral("进入地图节点：%1。敌人：%2")
                  .arg(nodeName)
                  .arg(node ? enemySummaryText(*node) : QStringLiteral("未知")));

    refreshUi();
    refreshMainCodeEditor();
    beginTurnWithoutAutoDraw();
    startTurnDrawFive();
}

void MainWindow::showGameResultOverlay(bool playerWin)
{
    hideGameResultOverlay();

    gameOverOverlay = new QWidget(ui->centralwidget);
    gameOverOverlay->setObjectName("gameOverOverlay");
    gameOverOverlay->setAttribute(Qt::WA_StyledBackground, true);
    gameOverOverlay->setGeometry(ui->centralwidget->rect());
    gameOverOverlay->setStyleSheet(
        "QWidget#gameOverOverlay {"
        "background-color: rgba(0, 0, 0, 165);"
        "}"
        "QWidget#resultCard {"
        "background-color: rgba(5, 10, 18, 232);"
        "border: 2px solid rgba(105, 225, 255, 210);"
        "border-radius: 18px;"
        "}"
        "QLabel#resultTitleLabel {"
        "font-family: 'Microsoft YaHei UI', 'Segoe UI';"
        "font-size: 40px;"
        "font-weight: 900;"
        "letter-spacing: 3px;"
        "}"
        "QLabel#resultSubLabel {"
        "font-size: 17px;"
        "color: #D7F7FF;"
        "}"
        "QLabel#resultCodeLabel {"
        "font-family: Consolas, 'Microsoft YaHei UI';"
        "font-size: 15px;"
        "color: #C9F4FF;"
        "background-color: rgba(2, 6, 12, 190);"
        "border: 1px solid rgba(45, 198, 255, 130);"
        "border-radius: 10px;"
        "padding: 12px;"
        "}"
        "QPushButton {"
        "background-color: rgba(10, 22, 34, 235);"
        "color: #E8FBFF;"
        "border: 1px solid #2DC6FF;"
        "border-radius: 10px;"
        "padding: 8px 18px;"
        "font-size: 15px;"
        "font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(25, 62, 90, 240);"
        "border: 1px solid #9BEFFF;"
        "}"
    );

    QVBoxLayout* overlayLayout = new QVBoxLayout(gameOverOverlay);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->addStretch();

    QWidget* card = new QWidget(gameOverOverlay);
    card->setObjectName("resultCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setFixedSize(560, 360);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(34, 28, 34, 28);
    cardLayout->setSpacing(14);

    QLabel* title = new QLabel(card);
    title->setObjectName("resultTitleLabel");
    title->setAlignment(Qt::AlignCenter);
    title->setText(playerWin ? QStringLiteral("VICTORY") : QStringLiteral("DEFEAT"));
    title->setStyleSheet(playerWin
                             ? "QLabel#resultTitleLabel { color: #FFD66B; }"
                             : "QLabel#resultTitleLabel { color: #FF5C7A; }");

    QLabel* sub = new QLabel(card);
    sub->setObjectName("resultSubLabel");
    sub->setAlignment(Qt::AlignCenter);
    sub->setWordWrap(true);
    sub->setText(playerWin
                     ? QStringLiteral("所有函数调用执行完毕，Boss 对象已被析构。")
                     : QStringLiteral("玩家对象生命值归零，本次运行异常终止。"));

    QLabel* code = new QLabel(card);
    code->setObjectName("resultCodeLabel");
    code->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    code->setText(playerWin
                      ? QStringLiteral("if (boss.hp <= 0) {\n    player.gainExp();\n    return VICTORY;\n}")
                      : QStringLiteral("if (player.hp <= 0) {\n    throw GameOverException();\n}") );

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* restart = new QPushButton(QStringLiteral("重新开始"), card);
    QPushButton* help = new QPushButton(QStringLiteral("查看规则"), card);
    QPushButton* close = new QPushButton(QStringLiteral("停留查看"), card);

    buttonLayout->addStretch();
    buttonLayout->addWidget(restart);
    buttonLayout->addWidget(help);
    buttonLayout->addWidget(close);
    buttonLayout->addStretch();

    cardLayout->addWidget(title);
    cardLayout->addWidget(sub);
    cardLayout->addWidget(code);
    cardLayout->addStretch();
    cardLayout->addLayout(buttonLayout);

    overlayLayout->addWidget(card, 0, Qt::AlignCenter);
    overlayLayout->addStretch();

    connect(restart, &QPushButton::clicked, this, [this]() {
        startNewGame();
    });
    connect(help, &QPushButton::clicked, this, [this]() {
        showHelpOverlay();
    });
    connect(close, &QPushButton::clicked, this, [this]() {
        hideGameResultOverlay();
        ui->restartButton->setEnabled(true);
        ui->helpButton->setEnabled(true);
    });

    gameOverOverlay->show();
    gameOverOverlay->raise();
}

void MainWindow::hideGameResultOverlay()
{
    if (!gameOverOverlay) {
        return;
    }

    QWidget* overlay = gameOverOverlay;
    gameOverOverlay = nullptr;
    overlay->hide();
    overlay->deleteLater();
}

void MainWindow::showHelpOverlay()
{
    hideHelpOverlay();

    helpOverlay = new QWidget(ui->centralwidget);
    helpOverlay->setObjectName("helpOverlay");
    helpOverlay->setAttribute(Qt::WA_StyledBackground, true);
    helpOverlay->setGeometry(ui->centralwidget->rect());
    helpOverlay->setStyleSheet(
        "QWidget#helpOverlay {"
        "background-color: rgba(0, 0, 0, 150);"
        "}"
        "QWidget#helpCard {"
        "background-color: rgba(4, 9, 18, 238);"
        "border: 2px solid rgba(45, 198, 255, 210);"
        "border-radius: 18px;"
        "}"
        "QLabel#helpTitleLabel {"
        "color: #8BE9FF;"
        "font-size: 30px;"
        "font-weight: 900;"
        "}"
        "QTextBrowser {"
        "background-color: rgba(2, 6, 12, 190);"
        "border: 1px solid rgba(45, 198, 255, 120);"
        "border-radius: 10px;"
        "color: #D7F7FF;"
        "font-family: 'Microsoft YaHei UI';"
        "font-size: 14px;"
        "padding: 12px;"
        "}"
        "QPushButton {"
        "background-color: rgba(10, 22, 34, 235);"
        "color: #E8FBFF;"
        "border: 1px solid #2DC6FF;"
        "border-radius: 10px;"
        "padding: 8px 18px;"
        "font-size: 15px;"
        "font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(25, 62, 90, 240);"
        "border: 1px solid #9BEFFF;"
        "}"
    );

    QVBoxLayout* overlayLayout = new QVBoxLayout(helpOverlay);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->addStretch();

    QWidget* card = new QWidget(helpOverlay);
    card->setObjectName("helpCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setFixedSize(760, 520);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 24, 28, 24);
    cardLayout->setSpacing(14);

    QLabel* title = new QLabel(QStringLiteral("HELP / RULES"), card);
    title->setObjectName("helpTitleLabel");
    title->setAlignment(Qt::AlignCenter);

    QTextBrowser* browser = new QTextBrowser(card);
    browser->setOpenExternalLinks(false);
    browser->setHtml(QStringLiteral(
        "<h2 style='color:#8BE9FF;'>CodeCraft 规则说明</h2>"
        "<p><b>目标：</b>在玩家生命归零前击败 Boss。</p>"
        "<p><b>函数调用模式：</b>打出卡牌不会立即结算，而是把对应语句写入中间代码块。点击结束回合后，代码会从上到下依次执行。</p>"
        "<p><b>代码高亮：</b>橙色表示当前正在执行的语句；左右两侧的 <code>tickStatuses()</code> 表示玩家与 Boss 的状态函数实现。</p>"
        "<p><b>状态变化：</b>中毒、灼烧、再生、力量等效果会改变状态函数。函数实现变化时，对应代码行会用紫色高亮。</p>"
        "<p><b>卡牌：</b>攻击、防御、治疗、强化、召唤等卡牌会生成不同代码语句。费用不足时无法写入代码。</p>"
        "<p><b>抽弃牌：</b>抽牌堆为空时，弃牌堆会回收到抽牌堆。</p>"
        "<p><b>C++ 映射：</b>玩家、随从、Boss 都是对象；出牌相当于写入函数调用；状态结算相当于执行成员函数。</p>"
    ));

    QPushButton* close = new QPushButton(QStringLiteral("关闭说明"), card);
    connect(close, &QPushButton::clicked, this, [this]() {
        hideHelpOverlay();
    });

    cardLayout->addWidget(title);
    cardLayout->addWidget(browser, 1);
    cardLayout->addWidget(close, 0, Qt::AlignCenter);

    overlayLayout->addWidget(card, 0, Qt::AlignCenter);
    overlayLayout->addStretch();

    helpOverlay->show();
    helpOverlay->raise();
}

void MainWindow::hideHelpOverlay()
{
    if (!helpOverlay) {
        return;
    }

    QWidget* overlay = helpOverlay;
    helpOverlay = nullptr;
    overlay->hide();
    overlay->deleteLater();
}


// ============================================================
// 执行反馈动画
// ============================================================

void MainWindow::playCommandFeedback(const CodeCommandView& command,
                                     int oldPlayerHp,
                                     int oldPlayerShield,
                                     int oldEnemyHp,
                                     int oldEnemyShield)
{
    if (!gameManager) {
        return;
    }

    const QString commandText = (command.title + "\n" + command.lines.join("\n")).toLower();
    const bool looksLikeTick = commandText.contains("tickstatus") || commandText.contains("tickstate");
    const bool looksLikePoison = commandText.contains("poison") || commandText.contains("中毒");
    const bool looksLikeHeal = commandText.contains("heal") || commandText.contains("治疗") || commandText.contains("regen") || commandText.contains("再生");
    const bool looksLikeShield = commandText.contains("shield") || commandText.contains("defend") || commandText.contains("防御") || commandText.contains("护盾");

    Enemy* enemy = firstAliveEnemy();
    const int newPlayerHp = gameManager->player.hp;
    const int newPlayerShield = gameManager->player.shield;
    const int newEnemyHp = enemy ? enemy->hp : oldEnemyHp;
    const int newEnemyShield = enemy ? enemy->shield : oldEnemyShield;

    const int playerHpDelta = newPlayerHp - oldPlayerHp;
    const int playerShieldDelta = newPlayerShield - oldPlayerShield;
    const int enemyHpDelta = newEnemyHp - oldEnemyHp;
    const int enemyShieldDelta = newEnemyShield - oldEnemyShield;

    const bool playerDidMove = commandText.contains("player") &&
                               (commandText.contains("attack") || commandText.contains("takeDamage") || commandText.contains("summon"));
    const bool enemyDidMove = (commandText.contains("enemy") || commandText.contains("boss")) &&
                              (commandText.contains("attack") || commandText.contains("taketurn") || commandText.contains("takeDamage"));

    if (playerDidMove && enemyHpDelta < 0) {
        animateActorNudge(ui->playerImageLabel, 28);
    }
    if (enemyDidMove && playerHpDelta < 0) {
        animateActorNudge(ui->enemyImageLabel, -28);
    }

    if (enemyHpDelta < 0) {
        animateActorShake(ui->enemyImageLabel);
        showFloatingText(ui->enemyImageLabel,
                         QString("-%1").arg(-enemyHpDelta),
                         looksLikePoison || looksLikeTick ? kPoisonTextColor : kDamageTextColor);
    } else if (enemyHpDelta > 0) {
        showFloatingText(ui->enemyImageLabel,
                         QString("+%1").arg(enemyHpDelta),
                         kHealTextColor);
    }

    if (playerHpDelta < 0) {
        animateActorShake(ui->playerImageLabel);
        showFloatingText(ui->playerImageLabel,
                         QString("-%1").arg(-playerHpDelta),
                         looksLikePoison || looksLikeTick ? kPoisonTextColor : kDamageTextColor);
    } else if (playerHpDelta > 0 || looksLikeHeal) {
        if (playerHpDelta > 0) {
            showFloatingText(ui->playerImageLabel,
                             QString("+%1").arg(playerHpDelta),
                             kHealTextColor);
        }
    }

    if (playerShieldDelta > 0 || looksLikeShield) {
        if (playerShieldDelta > 0) {
            showFloatingText(ui->playerImageLabel,
                             QString("护盾 +%1").arg(playerShieldDelta),
                             kShieldTextColor);
        }
    }

    if (enemyShieldDelta > 0) {
        showFloatingText(ui->enemyImageLabel,
                         QString("护盾 +%1").arg(enemyShieldDelta),
                         kShieldTextColor);
    }
}

void MainWindow::animateActorNudge(QWidget* widget, int dx)
{
    if (!widget || !widget->isVisible()) {
        return;
    }

    const QPoint origin = widget->pos();

    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    QPropertyAnimation* forward = new QPropertyAnimation(widget, "pos");
    forward->setDuration(120);
    forward->setStartValue(origin);
    forward->setEndValue(origin + QPoint(dx, 0));
    forward->setEasingCurve(QEasingCurve::OutCubic);

    QPropertyAnimation* back = new QPropertyAnimation(widget, "pos");
    back->setDuration(160);
    back->setStartValue(origin + QPoint(dx, 0));
    back->setEndValue(origin);
    back->setEasingCurve(QEasingCurve::OutBack);

    group->addAnimation(forward);
    group->addAnimation(back);

    connect(group, &QSequentialAnimationGroup::finished, this, [widget, origin, group]() {
        if (widget) {
            widget->move(origin);
        }
        group->deleteLater();
    });

    group->start();
}

void MainWindow::animateActorShake(QWidget* widget)
{
    if (!widget || !widget->isVisible()) {
        return;
    }

    const QPoint origin = widget->pos();
    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    const QList<int> offsets = { -8, 8, -6, 6, -3, 3, 0 };
    QPoint start = origin;

    for (int off : offsets) {
        QPropertyAnimation* anim = new QPropertyAnimation(widget, "pos");
        anim->setDuration(45);
        anim->setStartValue(start);
        anim->setEndValue(origin + QPoint(off, 0));
        anim->setEasingCurve(QEasingCurve::Linear);
        group->addAnimation(anim);
        start = origin + QPoint(off, 0);
    }

    connect(group, &QSequentialAnimationGroup::finished, this, [widget, origin, group]() {
        if (widget) {
            widget->move(origin);
        }
        group->deleteLater();
    });

    group->start();
}

void MainWindow::showFloatingText(QWidget* anchor,
                                  const QString& text,
                                  const QColor& color)
{
    if (!anchor || text.isEmpty()) {
        return;
    }

    QRect anchorRect = geometryInCentral(anchor);
    if (anchorRect.isNull()) {
        return;
    }

    QLabel* label = new QLabel(ui->centralwidget);
    label->setText(text);
    label->setAlignment(Qt::AlignCenter);
    label->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    QFont font("Microsoft YaHei UI");
    font.setPointSize(16);
    font.setBold(true);
    label->setFont(font);

    label->setStyleSheet(QString(
        "QLabel {"
        "color: rgb(%1,%2,%3);"
        "background: rgba(0, 0, 0, 90);"
        "border: 1px solid rgba(%1,%2,%3,180);"
        "border-radius: 10px;"
        "padding: 4px 10px;"
        "}"
    ).arg(color.red()).arg(color.green()).arg(color.blue()));

    label->adjustSize();
    const int x = anchorRect.center().x() - label->width() / 2;
    const int y = anchorRect.top() + anchorRect.height() / 4;
    label->move(x, y);
    label->show();
    label->raise();

    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(label);
    label->setGraphicsEffect(opacity);

    QParallelAnimationGroup* group = new QParallelAnimationGroup(this);

    QPropertyAnimation* move = new QPropertyAnimation(label, "pos");
    move->setDuration(800);
    move->setStartValue(label->pos());
    move->setEndValue(label->pos() + QPoint(0, -55));
    move->setEasingCurve(QEasingCurve::OutCubic);

    QPropertyAnimation* fade = new QPropertyAnimation(opacity, "opacity");
    fade->setDuration(800);
    fade->setStartValue(1.0);
    fade->setEndValue(0.0);
    fade->setEasingCurve(QEasingCurve::InCubic);

    group->addAnimation(move);
    group->addAnimation(fade);

    connect(group, &QParallelAnimationGroup::finished, this, [label, group]() {
        label->deleteLater();
        group->deleteLater();
    });

    group->start();
}

// ============================================================
// 动画
// ============================================================

void MainWindow::animateGhost(const QRect& startRect,
                              const QRect& endRect,
                              const QString& text,
                              const QString& toolTip,
                              int duration,
                              QEasingCurve::Type easing,
                              std::function<void()> onFinished)
{
    QPushButton* ghost = new QPushButton(ui->centralwidget);
    ghost->setText(text);
    ghost->setToolTip(toolTip);
    ghost->setGeometry(startRect);
    ghost->setStyleSheet(cardButtonStyle(QStringLiteral("#8BE9FF"), true));
    ghost->show();
    ghost->raise();

    QPropertyAnimation* animation = new QPropertyAnimation(ghost, "geometry");
    animation->setDuration(duration);
    animation->setStartValue(startRect);
    animation->setEndValue(endRect);
    animation->setEasingCurve(easing);

    connect(animation, &QPropertyAnimation::finished, this, [ghost, animation, onFinished]() {
        ghost->deleteLater();
        animation->deleteLater();
        if (onFinished) {
            onFinished();
        }
    });

    animation->start();
}

void MainWindow::drawOneCardAnimation(int handIndex,
                                      const CardView& card,
                                      std::function<void()> onFinished)
{
    if (handIndex < 0 || handIndex >= cardButtons.size()) {
        if (onFinished) {
            onFinished();
        }
        return;
    }

    animateGhost(geometryInCentral(ui->drawPileLabel),
                 geometryInCentral(cardButtons[handIndex]),
                 formatCardText(card),
                 qstr(card.description),
                 220,
                 QEasingCurve::OutCubic,
                 onFinished);
}

void MainWindow::playCardToDiscardAnimation(int index,
                                            const CardView& card,
                                            std::function<void()> onFinished)
{
    if (index < 0 || index >= cardButtons.size()) {
        if (onFinished) {
            onFinished();
        }
        return;
    }

    setControlsEnabled(false);
    cardButtons[index]->hide();

    animateGhost(geometryInCentral(cardButtons[index]),
                 geometryInCentral(ui->discardPileLabel),
                 formatCardText(card),
                 qstr(card.description),
                 250,
                 QEasingCurve::InCubic,
                 onFinished);
}

void MainWindow::recycleDiscardToDrawPileAnimation(std::function<void()> onFinished)
{
    animateGhost(geometryInCentral(ui->discardPileLabel),
                 geometryInCentral(ui->drawPileLabel),
                 "洗牌",
                 "",
                 300,
                 QEasingCurve::OutCubic,
                 onFinished);
}

// ============================================================
// 刷新界面
// ============================================================

void MainWindow::refreshUi()
{
    if (!gameManager) {
        return;
    }

    refreshPlayerUi();
    refreshEnemyUi();
    refreshPileUi();
    refreshHandUi();
    refreshMinionUi();
    refreshImageUi();
    refreshSideCodeEditors();
}

void MainWindow::refreshPlayerUi()
{
    const Player& player = gameManager->player;

    ui->playerHpLabel->setText(QString("玩家生命：%1/%2").arg(player.hp).arg(player.maxHp));
    ui->playerEnergyLabel->setText(QString("玩家能量：%1/%2").arg(player.energy).arg(player.maxEnergy));
    ui->energyValueLabel->setText(QString("%1/%2").arg(player.energy).arg(player.maxEnergy));
    ui->playerShieldLabel->setText(QString("玩家护盾：%1").arg(player.shield));

    int strength = 0;
    for (const Status& status : player.statuses) {
        if (status.type == StatusType::STRENGTH) {
            strength += status.value;
        }
    }

    ui->playerStrengthLabel->setText(
        QString("玩家力量：%1    实际攻击：%2")
            .arg(strength)
            .arg(player.getEffectiveAttack())
        );
}

void MainWindow::refreshEnemyUi()
{
    Enemy* enemy = firstAliveEnemy();

    if (!enemy) {
        ui->enemyHpLabel->setText("敌人生命：无");
        ui->bossSkillLabel->setText("Boss 技能：无");
        ui->bossSkillLabel->setToolTip("");
        return;
    }

    ui->enemyHpLabel->setText(
        QString("%1  生命：%2/%3")
            .arg(QString::fromStdString(enemy->name))
            .arg(enemy->hp)
            .arg(enemy->maxHp)
        );

    ui->enemyIntentLabel->setText(
        toQString(gameManager->getEnemyIntentText())
        );

    // bossSkillLabel 只作为“技能卡片入口”显示，完整技能说明通过美化后的 ToolTip 展示。
    ui->bossSkillLabel->setText("Boss 技能");
    ui->bossSkillLabel->setCursor(Qt::PointingHandCursor);
    ui->bossSkillLabel->setToolTip(buildBossSpecialSkillText(enemy));
}

void MainWindow::refreshPileUi()
{
    const int drawCount = gameManager->getDrawPileCount();
    const int discardCount = gameManager->getDiscardPileCount();

    ui->drawPileLabel->setText("抽牌堆");
    ui->discardPileLabel->setText("弃牌堆");

    ui->drawPileCountLabel->setText(QString::number(drawCount));
    ui->discardPileCountLabel->setText(QString::number(discardCount));
}

void MainWindow::refreshHandUi()
{
    if (!gameManager) {
        for (QPushButton* button : cardButtons) {
            if (!button) {
                continue;
            }
            button->setText("");
            button->setToolTip(QString());
            button->hide();
            button->setEnabled(false);
        }
        return;
    }

    auto handView = gameManager->getHandView();

    for (int i = 0; i < cardButtons.size(); ++i) {
        const bool hasCard = i < static_cast<int>(handView.size())
        && !qstr(handView[i].name).isEmpty();

        if (!hasCard) {
            cardButtons[i]->setText("");
            cardButtons[i]->setToolTip(QString());
            cardButtons[i]->hide();
            cardButtons[i]->setEnabled(false);
            continue;
        }

        const CardView& card = handView[i];
        const bool playable = controlsEnabled && gameManager->player.energy >= card.cost;

        cardButtons[i]->setText(formatCardText(card));
        cardButtons[i]->setToolTip(cardToolTipText(card));
        cardButtons[i]->show();
        cardButtons[i]->setEnabled(playable);
        updateCardButtonStyle(cardButtons[i], card, playable);
    }
}

void MainWindow::refreshMinionUi()
{
    QLabel* labels[2] = { ui->minionHpLabel1, ui->minionHpLabel2 };
    QLabel* images[2] = { ui->minionImageLabel1, ui->minionImageLabel2 };

    for (int i = 0; i < 2; ++i) {
        labels[i]->hide();
        labels[i]->clear();
        images[i]->hide();
    }

    if (!gameManager) {
        return;
    }

    int slot = 0;
    for (const Minion& minion : gameManager->player.minions) {
        if (slot >= 2) {
            break;
        }
        if (!minion.isAlive()) {
            continue;
        }

        QStringList lines;
        lines << QString("仆从%1：%2").arg(slot + 1).arg(QString::fromStdString(minion.name));
        lines << QString("生命：%1/%2    攻击：%3")
                     .arg(minion.hp)
                     .arg(minion.maxHp)
                     .arg(minion.getEffectiveAttack());

        const QString statusSummary = buildStatusSummary(minion.statuses);
        if (!statusSummary.isEmpty()) {
            lines << "状态：" + statusSummary;
        }

        labels[slot]->setText(lines.join("\n"));
        labels[slot]->show();
        images[slot]->show();
        ++slot;
    }
}

void MainWindow::refreshImageUi()
{
    if (!gameManager) {
        return;
    }

    ui->playerImageLabel->show();

    Enemy* enemy = firstAliveEnemy();
    ui->enemyImageLabel->setVisible(enemy != nullptr);

    // 图片内容初始化时已经设置；这里主要控制显隐。
}

void MainWindow::refreshMainCodeEditor()
{
    if (!gameManager) {
        return;
    }

    QStringList lines = { "{", "    // 请在此输入代码" };
    QVector<CodeRange> ranges;
    int currentLine = 2;

    for (const CodeCommandView& command : gameManager->getCodeCommandViews()) {
        if (command.lines.isEmpty()) {
            continue;
        }

        const TickFlags flags = detectTickCall(command);
        ranges.push_back({ currentLine,
                          static_cast<int>(command.lines.size()),
                          flags.player,
                          flags.enemy });

        for (const QString& line : command.lines) {
            lines << "    " + line;
        }
        currentLine += command.lines.size();
    }

    lines << "}";
    codeRanges = ranges;

    if (activeCodeIndex >= codeRanges.size()) {
        activeCodeIndex = -1;
    }

    ui->codePlainTextEdit->setPlainText(lines.join("\n"));
    applyMainCodeStyle();
    syncSideExecutionHighlightWithActiveCode();
}

void MainWindow::refreshSideCodeEditors()
{
    if (!gameManager) {
        return;
    }

    updateSideCode(Side::Player, makeTickStatusesBlock(gameManager->getPlayerCodeLines()));
    updateSideCode(Side::Enemy, makeTickStatusesBlock(gameManager->getEnemyCodeLines(firstAliveEnemy())));

    if (!sideChangeHighlightActive) {
        startNextSideChangeHighlight();
    }

    // 两侧执行高亮只由当前主代码 activeCodeIndex 决定，避免旧的 executingLines 残留，
    // 这可以修复左侧 tickStatuses() 偶尔一直保持橙色的问题。
    syncSideExecutionHighlightWithActiveCode();
}

void MainWindow::updateSideCode(Side side, const QStringList& newDisplayedLines)
{
    SideCodeState& state = (side == Side::Player) ? playerCode : enemyCode;
    if (!state.editor) {
        return;
    }

    const QStringList newBody = tickBody(newDisplayedLines);
    const bool firstRender = state.displayedLines.isEmpty()
                             && state.bodyLines.isEmpty()
                             && state.editor->toPlainText().isEmpty();
    const bool textChanged = state.displayedLines != newDisplayedLines;

    QSet<int> changedLines;
    if (textChanged && !firstRender) {
        changedLines = bodyChangesToDisplayedLines(
            changedBodyLines(state.bodyLines, newBody),
            state.bodyLines,
            newBody
            );

        if (changedLines.isEmpty()) {
            changedLines = allLineNumbers(state.editor);
        }
    }

    const QString newText = newDisplayedLines.join("\n");
    if (textChanged || state.editor->toPlainText() != newText) {
        state.displayedLines = newDisplayedLines;
        state.bodyLines = newBody;
        state.editor->setPlainText(newText);
    }

    if (textChanged && !changedLines.isEmpty()) {
        enqueueSideChangeHighlight(side, changedLines);
    }
}

// ============================================================
// 代码高亮
// ============================================================

void MainWindow::highlightCodeBlock(int commandIndex)
{
    if (commandIndex < 0 || commandIndex >= codeRanges.size()) {
        clearCodeHighlight();
        return;
    }

    activeCodeIndex = commandIndex;
    applyMainCodeStyle();
    syncSideExecutionHighlightWithActiveCode();
}

void MainWindow::clearCodeHighlight()
{
    activeCodeIndex = -1;
    applyMainCodeStyle();
    syncSideExecutionHighlightWithActiveCode();
}

void MainWindow::applyMainCodeStyle()
{
    if (!ui || !ui->codePlainTextEdit) {
        return;
    }

    QList<QTextEdit::ExtraSelection> selections;
    QTextDocument* document = ui->codePlainTextEdit->document();

    if (activeCodeIndex >= 0 && activeCodeIndex < codeRanges.size()) {
        const CodeRange& range = codeRanges[activeCodeIndex];
        for (int i = 0; i < range.lineCount; ++i) {
            QTextBlock block = document->findBlockByNumber(range.startLine + i);
            if (!block.isValid()) {
                continue;
            }

            QTextEdit::ExtraSelection sel;
            sel.cursor = QTextCursor(block);
            sel.cursor.select(QTextCursor::LineUnderCursor);
            sel.format.setForeground(kMainRunColor);
            sel.format.setFontWeight(QFont::Bold);
            selections << sel;
        }
    }

    addCommentStyle(ui->codePlainTextEdit, selections);
    ui->codePlainTextEdit->setExtraSelections(selections);
}

void MainWindow::applySideCodeStyle(SideCodeState& state)
{
    QPlainTextEdit* editor = state.editor;
    if (!editor) {
        return;
    }

    const QString text = editor->toPlainText();
    const int scrollValue = editor->verticalScrollBar() ? editor->verticalScrollBar()->value() : 0;
    const int cursorPosition = editor->textCursor().position();

    // 重置旧字符格式，再重新叠加当前文字高亮。
    editor->setPlainText(text);

    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(qMin(cursorPosition, qMax(0, editor->document()->characterCount() - 1)));
    editor->setTextCursor(cursor);
    if (editor->verticalScrollBar()) {
        editor->verticalScrollBar()->setValue(scrollValue);
    }

    applyLineTextStyle(editor, state.executingLines, kMainRunColor, true);
    applyLineTextStyle(editor, state.changedLines, kSideChangeColor, true);

    QList<QTextEdit::ExtraSelection> selections;
    addCommentStyle(editor, selections);
    editor->setExtraSelections(selections);
}

void MainWindow::applyLineTextStyle(QPlainTextEdit* editor,
                                    const QSet<int>& lines,
                                    const QColor& color,
                                    bool bold)
{
    if (!editor || lines.isEmpty()) {
        return;
    }

    QTextDocument* document = editor->document();
    for (int lineNumber : lines) {
        QTextBlock block = document->findBlockByNumber(lineNumber);
        if (!block.isValid()) {
            continue;
        }

        QTextCursor cursor(block);
        cursor.select(QTextCursor::LineUnderCursor);

        QTextCharFormat format;
        format.setForeground(color);
        if (bold) {
            format.setFontWeight(QFont::Bold);
        }
        cursor.mergeCharFormat(format);
    }
}

void MainWindow::addCommentStyle(QPlainTextEdit* editor,
                                 QList<QTextEdit::ExtraSelection>& selections) const
{
    if (!editor) {
        return;
    }

    QTextDocument* document = editor->document();
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const QString text = block.text();
        const int pos = text.indexOf("//");
        if (pos < 0) {
            continue;
        }

        QTextEdit::ExtraSelection sel;
        QTextCursor cursor(document);
        cursor.setPosition(block.position() + pos);
        cursor.setPosition(block.position() + text.length(), QTextCursor::KeepAnchor);
        sel.cursor = cursor;
        sel.format.setForeground(kCommentColor);
        sel.format.setFontItalic(true);
        selections << sel;
    }
}

void MainWindow::clearSideExecutionHighlight()
{
    playerCode.executingLines.clear();
    enemyCode.executingLines.clear();
    applySideCodeStyle(playerCode);
    applySideCodeStyle(enemyCode);
}

void MainWindow::setSideExecutionHighlight(const CodeRange& range)
{
    playerCode.executingLines.clear();
    enemyCode.executingLines.clear();

    if (!sideChangeHighlightActive) {
        if (range.callsPlayerTick) {
            playerCode.executingLines = allLineNumbers(playerCode.editor);
        }
        if (range.callsEnemyTick) {
            enemyCode.executingLines = allLineNumbers(enemyCode.editor);
        }
    }

    applySideCodeStyle(playerCode);
    applySideCodeStyle(enemyCode);
}

void MainWindow::syncSideExecutionHighlightWithActiveCode()
{
    playerCode.executingLines.clear();
    enemyCode.executingLines.clear();

    if (!sideChangeHighlightActive
        && activeCodeIndex >= 0
        && activeCodeIndex < codeRanges.size()) {
        const CodeRange& range = codeRanges[activeCodeIndex];

        if (range.callsPlayerTick) {
            playerCode.executingLines = allLineNumbers(playerCode.editor);
        }
        if (range.callsEnemyTick) {
            enemyCode.executingLines = allLineNumbers(enemyCode.editor);
        }
    }

    applySideCodeStyle(playerCode);
    applySideCodeStyle(enemyCode);
}

void MainWindow::enqueueSideChangeHighlight(Side side, const QSet<int>& lines)
{
    if (!lines.isEmpty()) {
        sideHighlightQueue.push_back({ side, lines });
    }
}

void MainWindow::startNextSideChangeHighlight()
{
    if (sideChangeHighlightActive || sideHighlightQueue.isEmpty()) {
        return;
    }

    // 函数修改高亮期间，停止其它高亮。
    activeCodeIndex = -1;
    playerCode.executingLines.clear();
    enemyCode.executingLines.clear();
    playerCode.changedLines.clear();
    enemyCode.changedLines.clear();
    applyMainCodeStyle();

    const SideHighlightRequest request = sideHighlightQueue.takeFirst();
    SideCodeState& target = (request.side == Side::Player) ? playerCode : enemyCode;
    target.changedLines = request.lines;

    sideChangeHighlightActive = true;
    const int token = ++sideHighlightToken;

    applySideCodeStyle(playerCode);
    applySideCodeStyle(enemyCode);

    QTimer::singleShot(SIDE_CHANGE_MS, this, [this, token]() {
        if (token != sideHighlightToken) {
            return;
        }

        playerCode.changedLines.clear();
        enemyCode.changedLines.clear();
        sideChangeHighlightActive = false;

        syncSideExecutionHighlightWithActiveCode();
        startNextSideChangeHighlight();
    });
}

void MainWindow::clearSideChangeHighlight()
{
    ++sideHighlightToken;
    sideHighlightQueue.clear();
    sideChangeHighlightActive = false;
    playerCode.changedLines.clear();
    enemyCode.changedLines.clear();
    syncSideExecutionHighlightWithActiveCode();
}

// ============================================================
// 按钮槽函数
// ============================================================

void MainWindow::on_cardButton1_clicked() { playCardByIndex(0); }
void MainWindow::on_cardButton2_clicked() { playCardByIndex(1); }
void MainWindow::on_cardButton3_clicked() { playCardByIndex(2); }
void MainWindow::on_cardButton4_clicked() { playCardByIndex(3); }
void MainWindow::on_cardButton5_clicked() { playCardByIndex(4); }

void MainWindow::on_restartButton_clicked()
{
    startNewGame();
}

void MainWindow::on_helpButton_clicked()
{
    showHelpOverlay();
}

// ============================================================
// 工具函数
// ============================================================

QRect MainWindow::geometryInCentral(QWidget* widget) const
{
    QPoint topLeft = widget->mapTo(ui->centralwidget, QPoint(0, 0));
    return QRect(topLeft, widget->size());
}

void MainWindow::setScaledPixmap(QLabel* label,
                                 const QString& resourcePath,
                                 Qt::AspectRatioMode mode)
{
    if (!label) {
        return;
    }

    QPixmap pixmap(resourcePath);
    label->setAlignment(Qt::AlignCenter);
    label->setFrameShape(QFrame::NoFrame);
    label->setAutoFillBackground(false);
    label->setAttribute(Qt::WA_TranslucentBackground, true);

    if (pixmap.isNull()) {
        label->setPixmap(QPixmap());
        label->setText(QString("缺少图片\n%1").arg(resourcePath));
        label->setStyleSheet(
            "QLabel {"
            "color: #66E6FF;"
            "background: transparent;"
            "border: none;"
            "}"
        );
        return;
    }

    label->setText("");
    label->setStyleSheet("QLabel { background: transparent; border: none; }");

    QPixmap output;
    if (label->width() > 0 && label->height() > 0) {
        output = pixmap.scaled(label->size(), mode, Qt::SmoothTransformation);
    } else {
        output = pixmap;
        label->setScaledContents(true);
    }

    // 背景图做轻微压暗，避免和人物立绘争夺视觉焦点。
    if (label == ui->battleBackgroundLabel && !output.isNull()) {
        QPixmap dimmed(output.size());
        dimmed.fill(Qt::transparent);
        QPainter painter(&dimmed);
        painter.drawPixmap(0, 0, output);
        painter.fillRect(dimmed.rect(), QColor(0, 0, 0, kBackgroundDimAlpha));
        painter.end();
        output = dimmed;
    }

    label->setPixmap(output);
}

void MainWindow::setControlsEnabled(bool enabled)
{
    controlsEnabled = enabled;

    // 地图界面显示时，gameManager 可能暂时为空。
    // 这时不能调用 refreshHandUi()，否则会访问空指针导致程序启动或重开时崩溃。
    if (gameManager) {
        refreshHandUi();
    } else {
        for (QPushButton* button : cardButtons) {
            if (!button) {
                continue;
            }
            button->setText("");
            button->setToolTip(QString());
            button->hide();
            button->setEnabled(false);
        }
    }

    ui->endTurnButton->setEnabled(enabled && gameManager != nullptr);
    ui->restartButton->setEnabled(true);
    ui->helpButton->setEnabled(true);
}

Enemy* MainWindow::firstAliveEnemy() const
{
    if (!gameManager) {
        return nullptr;
    }

    for (const auto& enemyPtr : gameManager->battle.enemies) {
        if (enemyPtr && enemyPtr->isAlive()) {
            return enemyPtr.get();
        }
    }
    return nullptr;
}

QString MainWindow::formatCardText(const CardView& card) const
{
    // 牌面空间有限：只显示名称、类别和费用。
    // 完整描述与“将写入的代码”保留在 ToolTip 中查看。
    return QString("【%1】\n%2\n⚡ 费用：%3")
        .arg(qstr(card.name))
        .arg(cardKindText(card))
        .arg(card.cost);
}

QString MainWindow::cardKindText(const CardView& card) const
{
    const QString text = (qstr(card.name) + " " + qstr(card.description)).toLower();

    if (text.contains("攻击") || text.contains("strike") || text.contains("damage") || text.contains("全力")) {
        return QStringLiteral("攻击指令");
    }
    if (text.contains("防御") || text.contains("护盾") || text.contains("defend") || text.contains("shield")) {
        return QStringLiteral("防御指令");
    }
    if (text.contains("治疗") || text.contains("恢复") || text.contains("heal") || text.contains("regen")) {
        return QStringLiteral("恢复指令");
    }
    if (text.contains("召唤") || text.contains("summon") || text.contains("仆从")) {
        return QStringLiteral("对象创建");
    }
    if (text.contains("强化") || text.contains("函数") || text.contains("function") || text.contains("template")) {
        return QStringLiteral("函数改写");
    }

    return QStringLiteral("代码指令");
}

QString MainWindow::cardCodePreview(const CardView& card) const
{
    QString preview;
    for (const QString& line : card.codeLines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            preview = trimmed;
            break;
        }
    }

    if (preview.isEmpty()) {
        return QStringLiteral("// 无代码");
    }

    const int maxLen = 22;
    if (preview.size() > maxLen) {
        preview = preview.left(maxLen - 1) + QStringLiteral("…");
    }
    return preview;
}

QString MainWindow::cardCodeHtml(const CardView& card) const
{
    if (card.codeLines.isEmpty()) {
        return QStringLiteral("<span style='color:#8AA0AA;'>// 无代码</span>");
    }

    QStringList htmlLines;
    for (const QString& line : card.codeLines) {
        QString escaped = line.toHtmlEscaped();
        if (escaped.trimmed().startsWith("//")) {
            escaped = QString("<span style='color:#76D982;font-style:italic;'>%1</span>").arg(escaped);
        } else {
            escaped = QString("<span style='color:#F7FDFF;'>%1</span>").arg(escaped);
        }
        htmlLines << escaped;
    }

    return htmlLines.join("<br>");
}

QString MainWindow::cardAccentColor(const CardView& card) const
{
    const QString kind = cardKindText(card);

    if (kind.contains("攻击")) {
        return QStringLiteral("#FF8A3D");
    }
    if (kind.contains("防御")) {
        return QStringLiteral("#58C7FF");
    }
    if (kind.contains("恢复")) {
        return QStringLiteral("#6DFF9A");
    }
    if (kind.contains("对象")) {
        return QStringLiteral("#7CFFEA");
    }
    if (kind.contains("函数")) {
        return QStringLiteral("#B86CFF");
    }

    return QStringLiteral("#6FEAFF");
}

QString MainWindow::cardToolTipText(const CardView& card) const
{
    const QString accent = cardAccentColor(card);
    const QString name = qstr(card.name).toHtmlEscaped();
    const QString kind = cardKindText(card).toHtmlEscaped();
    const QString desc = qstr(card.description).toHtmlEscaped().replace("\n", "<br>");
    const QString code = cardCodeHtml(card);

    return QString(
        "<html>"
        "<body style='background-color:#07111C;color:#F7FDFF;font-family:Microsoft YaHei UI,Segoe UI;"
        "font-size:10.5pt;line-height:150%;'>"
        "<div style='color:%1;font-size:13pt;font-weight:800;'>%2</div>"
        "<div style='color:#B9EFFF;font-weight:700;'>%3 · ⚡ 费用 %4</div>"
        "<hr style='border:0;border-top:1px solid #2DC6FF;margin:6px 0;'>"
        "<div style='color:#F2FDFF;font-weight:700;'>完整描述</div>"
        "<div style='color:#E6F9FF;'>%5</div>"
        "<div style='height:6px;'></div>"
        "<div style='color:#F2FDFF;font-weight:700;'>将写入代码块</div>"
        "<pre style='background-color:#030812;color:#F7FDFF;border:1px solid #1E6F86;"
        "border-radius:6px;padding:6px;font-family:Consolas,Microsoft YaHei UI;font-size:10pt;'>%6</pre>"
        "</body></html>")
        .arg(accent, name, kind)
        .arg(card.cost)
        .arg(desc)
        .arg(code);
}

QString MainWindow::cardButtonStyle(const QString& accent, bool playable) const
{
    const QString textColor = playable ? QStringLiteral("#F4FDFF") : QStringLiteral("#77848C");
    const QString borderColor = playable ? accent : QStringLiteral("#45515A");
    const QString hoverBorder = playable ? QStringLiteral("#FFFFFF") : QStringLiteral("#45515A");
    const QString bg = playable
        ? QStringLiteral("rgba(8, 13, 24, 225)")
        : QStringLiteral("rgba(14, 16, 22, 145)");
    const QString hoverBg = playable
        ? QStringLiteral("rgba(18, 40, 62, 238)")
        : QStringLiteral("rgba(14, 16, 22, 145)");

    return QString(
        "QPushButton {"
        "background-color:%1;"
        "color:%2;"
        "border:2px solid %3;"
        "border-radius:12px;"
        "padding:8px 6px;"
        "font-family:'Microsoft YaHei UI','Segoe UI';"
        "font-size:13px;"
        "font-weight:800;"
        "text-align:center;"
        "}"
        "QPushButton:hover {"
        "background-color:%4;"
        "border:2px solid %5;"
        "}"
        "QPushButton:pressed {"
        "background-color:rgba(42, 82, 108, 245);"
        "padding-top:10px;"
        "padding-left:7px;"
        "}"
        "QPushButton:disabled {"
        "background-color:rgba(12, 14, 20, 145);"
        "color:#6A747C;"
        "border:1px solid #38434C;"
        "}")
        .arg(bg, textColor, borderColor, hoverBg, hoverBorder);
}

void MainWindow::updateCardButtonStyle(QPushButton* button, const CardView& card, bool playable)
{
    if (!button) {
        return;
    }

    button->setStyleSheet(cardButtonStyle(cardAccentColor(card), playable));
}

QString MainWindow::statusTypeText(StatusType type) const
{
    switch (type) {
    case StatusType::BURN:       return "灼烧";
    case StatusType::POISON:     return "中毒";
    case StatusType::FREEZE:     return "冰冻";
    case StatusType::STUN:       return "眩晕";
    case StatusType::WEAKEN:     return "虚弱";
    case StatusType::VULNERABLE: return "易伤";
    case StatusType::STRENGTH:   return "力量";
    case StatusType::SHIELD:     return "护盾";
    case StatusType::INVINCIBLE: return "无敌";
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

QString MainWindow::buildStatusSummary(const std::vector<Status>& statuses) const
{
    QStringList parts;
    for (const Status& status : statuses) {
        QString turns = status.turnsRemaining < 0
                            ? "永久"
                            : QString("剩余%1回合").arg(status.turnsRemaining);
        parts << QString("%1(%2，值%3)")
                     .arg(statusTypeText(status.type))
                     .arg(turns)
                     .arg(status.value);
    }
    return parts.join("，");
}


QString MainWindow::buildBossSpecialSkillText(Enemy* enemy) const
{
    if (!enemy) {
        return QStringLiteral("<html><body style='color:#F7FDFF;'>无特殊技能</body></html>");
    }

    QStringList items;
    for (const std::string& line : enemy->getDescription()) {
        const QString text = QString::fromStdString(line).toHtmlEscaped();
        if (!text.trimmed().isEmpty()) {
            items << QString("<div style='margin:3px 0;color:#F2FDFF;'>• %1</div>").arg(text);
        }
    }

    if (items.isEmpty()) {
        items << QStringLiteral("<div style='color:#F2FDFF;'>无特殊技能</div>");
    }

    return QString(
        "<html>"
        "<body style='background-color:#07111C;color:#F7FDFF;font-family:Microsoft YaHei UI,Segoe UI;"
        "font-size:10.5pt;line-height:150%;'>"
        "<div style='color:#B86CFF;font-size:13pt;font-weight:800;'>Boss 技能说明</div>"
        "<div style='color:#8BE9FF;font-weight:700;margin-bottom:4px;'>%1</div>"
        "<hr style='border:0;border-top:1px solid #6B3AA0;margin:6px 0;'>"
        "%2"
        "</body></html>")
        .arg(QString::fromStdString(enemy->name).toHtmlEscaped(), items.join(""));
}

QString MainWindow::toQString(const std::string& s) const
{
    return QString::fromStdString(s);
}

QStringList MainWindow::toQStringList(const std::vector<std::string>& lines) const
{
    QStringList result;
    result.reserve(static_cast<qsizetype>(lines.size()));
    for (const std::string& line : lines) {
        result << QString::fromStdString(line);
    }
    return result;
}

QStringList MainWindow::makeTickStatusesBlock(const std::vector<std::string>& bodyLines) const
{
    return makeTickStatusesBlock(toQStringList(bodyLines));
}

QStringList MainWindow::makeTickStatusesBlock(const QStringList& bodyLines) const
{
    if (!bodyLines.isEmpty() && bodyLines.first().trimmed().contains("tickStatuses")) {
        return bodyLines;
    }

    QStringList result;
    result << "tickStatuses() {";
    if (bodyLines.isEmpty()) {
        result << "";
    } else {
        for (const QString& line : bodyLines) {
            result << "    " + line;
        }
    }
    result << "}";
    return result;
}

QSet<int> MainWindow::allLineNumbers(QPlainTextEdit* editor) const
{
    QSet<int> lines;
    if (!editor || !editor->document()) {
        return lines;
    }

    for (int i = 0; i < editor->document()->blockCount(); ++i) {
        lines.insert(i);
    }
    return lines;
}

QSet<int> MainWindow::changedBodyLines(const QStringList& oldBody,
                                       const QStringList& newBody) const
{
    QSet<int> changed;
    if (oldBody == newBody) {
        return changed;
    }

    const int count = std::max(static_cast<int>(oldBody.size()),
                               static_cast<int>(newBody.size()));
    for (int i = 0; i < count; ++i) {
        const QString oldLine = i < oldBody.size() ? oldBody[i] : QString();
        const QString newLine = i < newBody.size() ? newBody[i] : QString();
        if (oldLine != newLine && i < newBody.size()) {
            changed.insert(i);
        }
    }
    return changed;
}
