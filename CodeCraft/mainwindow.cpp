#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPropertyAnimation>
#include <QMessageBox>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QTextCharFormat>
#include <QEasingCurve>
#include <QTimer>
#include <QVariantAnimation>
#include <QColor>
#include <QFont>
#include <algorithm>

namespace {
constexpr int HAND_SLOT_COUNT = 5;
constexpr int MINION_SLOT_COUNT = 2;
constexpr int CODE_EXECUTE_DELAY_MS = 700;
constexpr int CODE_EXECUTE_INTERVAL_MS = 350;
constexpr int FUNCTION_FADE_DURATION_MS = 1200;

QString turnsText(int turns)
{
    return turns < 0 ? QStringLiteral("永久") : QStringLiteral("剩余 %1 回合").arg(turns);
}

QColor mixWithNormalText(const QColor& highlight, qreal alpha)
{
    const QColor normal(31, 35, 40);
    if (alpha < 0.0) {
        alpha = 0.0;
    }
    if (alpha > 1.0) {
        alpha = 1.0;
    }
    return QColor(
        static_cast<int>(normal.red()   * (1.0 - alpha) + highlight.red()   * alpha),
        static_cast<int>(normal.green() * (1.0 - alpha) + highlight.green() * alpha),
        static_cast<int>(normal.blue()  * (1.0 - alpha) + highlight.blue()  * alpha)
    );
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initCardButtons();
    initCodeEditors();
    startNewGame();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ==========================
// 初始化
// ==========================

void MainWindow::initCardButtons()
{
    cardButtons.clear();
    cardButtons.push_back(ui->cardButton1);
    cardButtons.push_back(ui->cardButton2);
    cardButtons.push_back(ui->cardButton3);
    cardButtons.push_back(ui->cardButton4);
    cardButtons.push_back(ui->cardButton5);
}

void MainWindow::initCodeEditors()
{
    QVector<QPlainTextEdit*> editors = {
        ui->codePlainTextEdit,
        ui->playerTickCodePlainTextEdit,
        ui->enemyTickCodePlainTextEdit
    };

    QFont codeFont("Consolas");
    codeFont.setStyleHint(QFont::Monospace);
    codeFont.setPointSize(10);

    for (QPlainTextEdit* editor : editors) {
        editor->setReadOnly(true);
        editor->setLineWrapMode(QPlainTextEdit::NoWrap);
        editor->setFont(codeFont);
        editor->setStyleSheet(
            "QPlainTextEdit {"
            "background-color: #F8F9FB;"
            "color: #1F2328;"
            "border: 1px solid #D0D7DE;"
            "border-radius: 6px;"
            "padding: 8px;"
            "}"
        );
    }
}

void MainWindow::startNewGame()
{
    gameManager = std::make_unique<GameManager>();

    activeMainCodeIndex = -1;
    lastPlayerTickLines.clear();
    lastEnemyTickLines.clear();
    changedPlayerTickLines.clear();
    changedEnemyTickLines.clear();
    functionChangeFadeAlpha = 0.0;

    clearLogs();
    appendLog(QStringLiteral("游戏开始。"));

    refreshUi();

    beginTurnWithoutAutoDraw();
    gameManager->prepareTurnCodeBlock();
    refreshAllCodeEditors(false);
    startTurnDrawFive();
}

// ==========================
// 日志
// ==========================

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

// ==========================
// 回合流程
// ==========================

void MainWindow::beginTurnWithoutAutoDraw()
{
    if (!gameManager) {
        return;
    }

    gameManager->beginTurnWithoutDraw();

    appendLog(QStringLiteral("第 %1 回合开始。").arg(gameManager->turnNumber));
    appendLog(QStringLiteral("开始抽牌。"));

    refreshUi();
}

void MainWindow::startTurnDrawFive()
{
    setCardButtonsEnabled(false);
    drawNextCard(HAND_SLOT_COUNT);
}

void MainWindow::drawNextCard(int remainingCount)
{
    if (!gameManager) {
        return;
    }

    if (remainingCount <= 0) {
        appendLog(QStringLiteral("抽牌阶段结束。"));
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    QVector<CardView> handView = gameManager->getHandView();
    if (handView.size() >= HAND_SLOT_COUNT) {
        appendLog(QStringLiteral("手牌已满，停止抽牌。"));
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    DrawResult result = gameManager->drawOneCard();

    if (result.needRecycle) {
        appendLog(QStringLiteral("抽牌堆为空，弃牌堆放回抽牌堆。"));

        recycleDiscardToDrawPileAnimation([this, remainingCount]() {
            gameManager->recycleDiscardToDrawPile();
            refreshUi();
            drawNextCard(remainingCount);
        });
        return;
    }

    if (!result.success) {
        appendLog(QStringLiteral("没有牌可抽。"));
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    appendLog(QStringLiteral("抽到【%1】。").arg(result.card.name));
    refreshPileUi();

    drawOneCardAnimation(result.handIndex, result.card, [this, remainingCount]() {
        refreshUi();
        setCardButtonsEnabled(false);
        drawNextCard(remainingCount - 1);
    });
}

// ==========================
// 抽牌动画
// ==========================

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

    QRect startRect = geometryInCentral(ui->drawPileLabel);
    QRect endRect = geometryInCentral(cardButtons[handIndex]);

    QPushButton* ghostCard = new QPushButton(ui->centralwidget);
    ghostCard->setText(QStringLiteral("%1\n费用：%2").arg(card.name).arg(card.cost));
    ghostCard->setToolTip(card.description);
    ghostCard->setGeometry(startRect);
    ghostCard->show();
    ghostCard->raise();

    QPropertyAnimation* animation = new QPropertyAnimation(ghostCard, "geometry");
    animation->setDuration(220);
    animation->setStartValue(startRect);
    animation->setEndValue(endRect);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(animation, &QPropertyAnimation::finished, this,
            [ghostCard, animation, onFinished]() {
        ghostCard->deleteLater();
        animation->deleteLater();

        if (onFinished) {
            onFinished();
        }
    });

    animation->start();
}

void MainWindow::recycleDiscardToDrawPileAnimation(std::function<void()> onFinished)
{
    QRect startRect = geometryInCentral(ui->discardPileLabel);
    QRect endRect = geometryInCentral(ui->drawPileLabel);

    QPushButton* ghostPile = new QPushButton(ui->centralwidget);
    ghostPile->setText(QStringLiteral("洗牌"));
    ghostPile->setGeometry(startRect);
    ghostPile->show();
    ghostPile->raise();

    QPropertyAnimation* animation = new QPropertyAnimation(ghostPile, "geometry");
    animation->setDuration(300);
    animation->setStartValue(startRect);
    animation->setEndValue(endRect);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(animation, &QPropertyAnimation::finished, this,
            [ghostPile, animation, onFinished]() {
        ghostPile->deleteLater();
        animation->deleteLater();

        if (onFinished) {
            onFinished();
        }
    });

    animation->start();
}

// ==========================
// 出牌：写入代码块，不立即结算效果
// ==========================

void MainWindow::playCardByIndex(int index)
{
    if (!gameManager) {
        return;
    }

    QVector<CardView> handView = gameManager->getHandView();

    if (index < 0 || index >= handView.size()) {
        return;
    }

    CardView card = handView[index];
    Enemy* target = firstAliveEnemy();

    PlayResult result = gameManager->playCardAsCode(index, target);

    if (!result.success) {
        appendLog(QStringLiteral("无法写入代码：%1").arg(result.failReason));
        refreshUi();
        return;
    }

    appendLog(QStringLiteral("写入代码：【%1】。").arg(result.card.name));

    playCardToDiscardAnimation(index, card, [this]() {
        refreshUi();
        refreshAllCodeEditors(true);

        if (gameManager && !gameManager->isBattleOver()) {
            setCardButtonsEnabled(true);
        }
    });
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

    setCardButtonsEnabled(false);

    QRect startRect = geometryInCentral(cardButtons[index]);
    QRect endRect = geometryInCentral(ui->discardPileLabel);

    QPushButton* ghostCard = new QPushButton(ui->centralwidget);
    ghostCard->setText(QStringLiteral("%1\n费用：%2").arg(card.name).arg(card.cost));
    ghostCard->setToolTip(card.description);
    ghostCard->setGeometry(startRect);
    ghostCard->show();
    ghostCard->raise();

    cardButtons[index]->hide();

    QPropertyAnimation* animation = new QPropertyAnimation(ghostCard, "geometry");
    animation->setDuration(250);
    animation->setStartValue(startRect);
    animation->setEndValue(endRect);
    animation->setEasingCurve(QEasingCurve::InCubic);

    connect(animation, &QPropertyAnimation::finished, this,
            [this, ghostCard, animation, onFinished]() {
        ghostCard->deleteLater();
        animation->deleteLater();

        if (onFinished) {
            onFinished();
        }
    });

    animation->start();
}

// ==========================
// 结束回合：逐段高亮执行代码
// ==========================

void MainWindow::on_endTurnButton_clicked()
{
    if (!gameManager) {
        return;
    }

    setCardButtonsEnabled(false);
    appendLog(QStringLiteral("玩家结束回合，开始按顺序执行代码块。"));
    executeCodeQueue();
}

void MainWindow::executeCodeQueue()
{
    refreshMainCodeEditor();
    executeNextCode(0);
}

void MainWindow::executeNextCode(int index)
{
    if (!gameManager) {
        return;
    }

    const int pendingCount = gameManager->pendingCodeCount();

    if (index < pendingCount) {
        highlightMainCodeBlock(index);

        QTimer::singleShot(CODE_EXECUTE_DELAY_MS, this, [this, index]() {
            gameManager->executePendingCode(index);
            appendLog(QStringLiteral("执行代码段 %1。").arg(index + 1));
            refreshUi();
            refreshAllCodeEditors(true);

            QTimer::singleShot(CODE_EXECUTE_INTERVAL_MS, this, [this, index]() {
                executeNextCode(index + 1);
            });
        });
        return;
    }

    if (index == pendingCount) {
        highlightMainCodeBlock(index); // player.tickState();
        QTimer::singleShot(CODE_EXECUTE_DELAY_MS, this, [this, index]() {
            QTimer::singleShot(CODE_EXECUTE_INTERVAL_MS, this, [this, index]() {
                executeNextCode(index + 1);
            });
        });
        return;
    }

    if (index == pendingCount + 1) {
        highlightMainCodeBlock(index); // boss.tickState();
        QTimer::singleShot(CODE_EXECUTE_DELAY_MS, this, [this]() {
            finishCodeExecutionAndEnterNextTurn();
        });
        return;
    }

    finishCodeExecutionAndEnterNextTurn();
}

void MainWindow::finishCodeExecutionAndEnterNextTurn()
{
    if (!gameManager) {
        return;
    }

    TurnResult result = gameManager->finishTurnAfterCodeExecution();

    clearMainCodeHighlight();
    refreshUi();
    refreshAllCodeEditors(true);

    if (result.gameOver) {
        QMessageBox::information(
            this,
            QStringLiteral("游戏结束"),
            result.playerWin ? QStringLiteral("胜利！") : QStringLiteral("失败！")
        );
        setCardButtonsEnabled(false);
        return;
    }

    beginTurnWithoutAutoDraw();
    gameManager->prepareTurnCodeBlock();
    refreshAllCodeEditors(false);
    startTurnDrawFive();
}

// ==========================
// 刷新界面
// ==========================

void MainWindow::refreshUi()
{
    if (!gameManager) {
        return;
    }

    refreshPlayerUi();
    refreshEnemyUi();
    refreshBossSkillUi();
    refreshMinionUi();
    refreshPileUi();
    refreshHandUi();
}

void MainWindow::refreshPlayerUi()
{
    const Player& player = gameManager->player;

    ui->playerHpLabel->setText(
        QStringLiteral("玩家生命：%1/%2").arg(player.hp).arg(player.maxHp)
    );

    ui->playerEnergyLabel->setText(
        QStringLiteral("玩家能量：%1/%2").arg(player.energy).arg(player.maxEnergy)
    );

    ui->playerShieldLabel->setText(
        QStringLiteral("玩家护盾：%1").arg(player.shield)
    );

    ui->playerStrengthLabel->setText(
        QStringLiteral("玩家力量：%1    实际攻击：%2")
            .arg(statusValue(StatusType::STRENGTH))
            .arg(player.getEffectiveAttack())
    );
}

void MainWindow::refreshEnemyUi()
{
    Enemy* enemy = firstAliveEnemy();

    if (enemy == nullptr) {
        ui->enemyHpLabel->setText(QStringLiteral("Boss生命：无"));
        return;
    }

    ui->enemyHpLabel->setText(
        QStringLiteral("Boss生命：%1/%2    护盾：%3")
            .arg(enemy->hp)
            .arg(enemy->maxHp)
            .arg(enemy->shield)
    );
}

void MainWindow::refreshBossSkillUi()
{
    Enemy* enemy = firstAliveEnemy();

    if (!enemy) {
        ui->bossSkillLabel->setText(QStringLiteral("Boss 技能：无"));
        ui->bossSkillLabel->hide();
        return;
    }

    ui->bossSkillLabel->setText(buildBossSkillText(enemy));
    ui->bossSkillLabel->show();
}

void MainWindow::refreshMinionUi()
{
    QVector<QLabel*> labels = { ui->minionHpLabel1, ui->minionHpLabel2 };

    for (QLabel* label : labels) {
        label->clear();
        label->hide();
    }

    if (!gameManager) {
        return;
    }

    int displayed = 0;
    for (const Minion& minion : gameManager->player.minions) {
        if (!minion.isAlive()) {
            continue;
        }

        if (displayed >= MINION_SLOT_COUNT) {
            break;
        }

        labels[displayed]->setText(buildMinionInfoText(displayed + 1, minion));
        labels[displayed]->show();
        ++displayed;
    }
}

void MainWindow::refreshPileUi()
{
    ui->drawPileLabel->setText(
        QStringLiteral("抽牌堆\n%1").arg(gameManager->getDrawPileCount())
    );

    ui->discardPileLabel->setText(
        QStringLiteral("弃牌堆\n%1").arg(gameManager->getDiscardPileCount())
    );
}

void MainWindow::refreshHandUi()
{
    QVector<CardView> handView = gameManager->getHandView();

    for (int i = 0; i < cardButtons.size(); ++i) {
        if (i < handView.size() && !handView[i].name.isEmpty()) {
            const CardView& card = handView[i];

            cardButtons[i]->setText(
                QStringLiteral("%1\n费用：%2").arg(card.name).arg(card.cost)
            );
            cardButtons[i]->setToolTip(card.description);
            cardButtons[i]->show();
            cardButtons[i]->setEnabled(true);
        } else {
            cardButtons[i]->setText("");
            cardButtons[i]->hide();
            cardButtons[i]->setEnabled(false);
        }
    }
}

// ==========================
// 代码块显示与高亮
// ==========================

void MainWindow::refreshAllCodeEditors(bool markFunctionChanges)
{
    refreshMainCodeEditor();
    refreshFunctionCodeEditors(markFunctionChanges);
}

void MainWindow::refreshMainCodeEditor()
{
    if (!gameManager) {
        return;
    }

    QVector<CodeCommandView> commands = gameManager->getCodeCommandViews();

    QStringList lines;
    QVector<CodeRange> ranges;

    lines << QStringLiteral("void BattleTurn::execute() {");
    lines << QStringLiteral("    // 玩家出牌会被翻译成函数调用");

    for (const CodeCommandView& command : commands) {
        CodeRange range;
        range.startLine = lines.size();
        range.lineCount = std::max(1, static_cast<int>(command.lines.size()));

        if (command.lines.isEmpty()) {
            lines << QStringLiteral("    // %1").arg(command.title);
        } else {
            for (const QString& line : command.lines) {
                lines << QStringLiteral("    %1").arg(line);
            }
        }

        ranges.push_back(range);
    }

    CodeRange playerTickRange;
    playerTickRange.startLine = lines.size();
    playerTickRange.lineCount = 1;
    lines << QStringLiteral("    player.tickState();");
    ranges.push_back(playerTickRange);

    CodeRange enemyTickRange;
    enemyTickRange.startLine = lines.size();
    enemyTickRange.lineCount = 1;
    lines << QStringLiteral("    boss.tickState();");
    ranges.push_back(enemyTickRange);

    lines << QStringLiteral("}");

    mainCodeRanges = ranges;
    ui->codePlainTextEdit->setPlainText(lines.join("\n"));
    applyMainCodeTextStyles();
}

void MainWindow::refreshFunctionCodeEditors(bool markChanges)
{
    QStringList playerLines = buildPlayerTickFunctionLines();
    QStringList enemyLines = buildEnemyTickFunctionLines();

    if (markChanges) {
        markChangedFunctionLines(lastPlayerTickLines, playerLines, changedPlayerTickLines);
        markChangedFunctionLines(lastEnemyTickLines, enemyLines, changedEnemyTickLines);
    } else {
        changedPlayerTickLines.clear();
        changedEnemyTickLines.clear();
    }

    lastPlayerTickLines = playerLines;
    lastEnemyTickLines = enemyLines;

    ui->playerTickCodePlainTextEdit->setPlainText(playerLines.join("\n"));
    ui->enemyTickCodePlainTextEdit->setPlainText(enemyLines.join("\n"));

    if (markChanges && (!changedPlayerTickLines.isEmpty() || !changedEnemyTickLines.isEmpty())) {
        startFunctionChangeFadeAnimation();
    } else {
        applyFunctionCodeTextStyles();
    }
}

void MainWindow::highlightMainCodeBlock(int commandIndex)
{
    if (commandIndex < 0 || commandIndex >= mainCodeRanges.size()) {
        clearMainCodeHighlight();
        return;
    }

    activeMainCodeIndex = commandIndex;
    applyMainCodeTextStyles();
}

void MainWindow::clearMainCodeHighlight()
{
    activeMainCodeIndex = -1;
    applyMainCodeTextStyles();
}

void MainWindow::applyMainCodeTextStyles()
{
    QList<QTextEdit::ExtraSelection> selections;

    if (activeMainCodeIndex >= 0 && activeMainCodeIndex < mainCodeRanges.size()) {
        QTextCharFormat format;
        format.setForeground(QColor(220, 90, 40));
        format.setFontWeight(QFont::Bold);

        const CodeRange& range = mainCodeRanges[activeMainCodeIndex];
        for (int i = 0; i < range.lineCount; ++i) {
            addFullLineSelection(ui->codePlainTextEdit, selections, range.startLine + i, format);
        }
    }

    addCommentTextStyles(ui->codePlainTextEdit, selections);
    ui->codePlainTextEdit->setExtraSelections(selections);
}

void MainWindow::applyFunctionCodeTextStyles()
{
    QList<QTextEdit::ExtraSelection> playerSelections;
    QList<QTextEdit::ExtraSelection> enemySelections;

    if (functionChangeFadeAlpha > 0.0) {
        QTextCharFormat changedFormat;
        changedFormat.setForeground(mixWithNormalText(QColor(45, 120, 230), functionChangeFadeAlpha));
        changedFormat.setFontWeight(QFont::Bold);

        for (int line : changedPlayerTickLines) {
            addFullLineSelection(ui->playerTickCodePlainTextEdit, playerSelections, line, changedFormat);
        }
        for (int line : changedEnemyTickLines) {
            addFullLineSelection(ui->enemyTickCodePlainTextEdit, enemySelections, line, changedFormat);
        }
    }

    addCommentTextStyles(ui->playerTickCodePlainTextEdit, playerSelections);
    addCommentTextStyles(ui->enemyTickCodePlainTextEdit, enemySelections);

    ui->playerTickCodePlainTextEdit->setExtraSelections(playerSelections);
    ui->enemyTickCodePlainTextEdit->setExtraSelections(enemySelections);
}

void MainWindow::addCommentTextStyles(QPlainTextEdit* editor,
                                      QList<QTextEdit::ExtraSelection>& selections) const
{
    if (!editor) {
        return;
    }

    QTextDocument* document = editor->document();
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const QString text = block.text();
        const int commentPos = text.indexOf(QStringLiteral("//"));
        if (commentPos < 0) {
            continue;
        }

        QTextCursor cursor(document);
        cursor.setPosition(block.position() + commentPos);
        cursor.setPosition(block.position() + text.length(), QTextCursor::KeepAnchor);

        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format.setForeground(QColor(80, 150, 80));
        selection.format.setFontItalic(true);
        selections.append(selection);
    }
}

void MainWindow::addFullLineSelection(QPlainTextEdit* editor,
                                      QList<QTextEdit::ExtraSelection>& selections,
                                      int line,
                                      const QTextCharFormat& format) const
{
    if (!editor || line < 0) {
        return;
    }

    QTextBlock block = editor->document()->findBlockByNumber(line);
    if (!block.isValid()) {
        return;
    }

    QTextEdit::ExtraSelection selection;
    selection.cursor = QTextCursor(block);
    selection.cursor.select(QTextCursor::LineUnderCursor);
    selection.format = format;
    selections.append(selection);
}

QStringList MainWindow::buildPlayerTickFunctionLines() const
{
    QStringList lines;
    lines << QStringLiteral("void Player::tickState() {");

    if (!gameManager || gameManager->player.statuses.empty()) {
        lines << QStringLiteral("    // 当前没有需要结算的状态");
    } else {
        for (const Status& status : gameManager->player.statuses) {
            lines << QStringLiteral("    // %1").arg(statusSummary(status));
            lines << QStringLiteral("    %1").arg(statusTickCodeLine(QStringLiteral("player"), status));
        }
    }

    lines << QStringLiteral("}");
    return lines;
}

QStringList MainWindow::buildEnemyTickFunctionLines() const
{
    QStringList lines;
    lines << QStringLiteral("void Boss::tickState() {");

    Enemy* enemy = firstAliveEnemy();
    if (!enemy || enemy->statuses.empty()) {
        lines << QStringLiteral("    // 当前没有需要结算的状态");
    } else {
        for (const Status& status : enemy->statuses) {
            lines << QStringLiteral("    // %1").arg(statusSummary(status));
            lines << QStringLiteral("    %1").arg(statusTickCodeLine(QStringLiteral("boss"), status));
        }
    }

    lines << QStringLiteral("}");
    return lines;
}

void MainWindow::markChangedFunctionLines(const QStringList& oldLines,
                                          const QStringList& newLines,
                                          QSet<int>& changedLines) const
{
    changedLines.clear();
    const int maxCount = std::max(oldLines.size(), newLines.size());

    for (int i = 0; i < maxCount; ++i) {
        const QString oldLine = i < oldLines.size() ? oldLines[i] : QString();
        const QString newLine = i < newLines.size() ? newLines[i] : QString();

        if (oldLine != newLine && i < newLines.size()) {
            changedLines.insert(i);
        }
    }
}

void MainWindow::startFunctionChangeFadeAnimation()
{
    if (functionChangeFadeAnimation) {
        functionChangeFadeAnimation->stop();
        functionChangeFadeAnimation->deleteLater();
        functionChangeFadeAnimation = nullptr;
    }

    functionChangeFadeAnimation = new QVariantAnimation(this);
    functionChangeFadeAnimation->setDuration(FUNCTION_FADE_DURATION_MS);
    functionChangeFadeAnimation->setStartValue(1.0);
    functionChangeFadeAnimation->setEndValue(0.0);
    functionChangeFadeAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(functionChangeFadeAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value) {
        functionChangeFadeAlpha = value.toReal();
        applyFunctionCodeTextStyles();
    });

    connect(functionChangeFadeAnimation, &QVariantAnimation::finished, this, [this]() {
        functionChangeFadeAlpha = 0.0;
        changedPlayerTickLines.clear();
        changedEnemyTickLines.clear();
        applyFunctionCodeTextStyles();

        if (functionChangeFadeAnimation) {
            functionChangeFadeAnimation->deleteLater();
            functionChangeFadeAnimation = nullptr;
        }
    });

    functionChangeFadeAnimation->start();
}

// ==========================
// 信息文本
// ==========================

QString MainWindow::buildBossSkillText(Enemy* enemy) const
{
    if (!enemy) {
        return QStringLiteral("Boss 技能：无");
    }

    QString text;
    const QString name = QString::fromStdString(enemy->name);

    text += QStringLiteral("Boss：%1\n").arg(name);
    text += QStringLiteral("基础攻击：%1    实际攻击：%2\n")
                .arg(enemy->baseAttack)
                .arg(enemy->getEffectiveAttack());

    if (!enemy->statuses.empty()) {
        QStringList statusTexts;
        for (const Status& status : enemy->statuses) {
            statusTexts << statusSummary(status);
        }
        text += QStringLiteral("状态：%1\n").arg(statusTexts.join(QStringLiteral("；")));
    }

    if (name.contains(QStringLiteral("Exception"), Qt::CaseInsensitive)
        || name.contains(QStringLiteral("异常"))) {
        text += QStringLiteral("技能：try-catch-finally 异常链；低血量时可能捕获致命伤害。");
    } else if (name.contains(QStringLiteral("Template"), Qt::CaseInsensitive)
               || name.contains(QStringLiteral("模板"))) {
        text += QStringLiteral("技能：模板展开；可能重复调用攻击函数。");
    } else if (name.contains(QStringLiteral("Fire"), Qt::CaseInsensitive)
               || name.contains(QStringLiteral("火"))) {
        text += QStringLiteral("技能：火焰攻击；可能施加 Burn 状态。");
    } else if (name.contains(QStringLiteral("Frozen"), Qt::CaseInsensitive)
               || name.contains(QStringLiteral("冰"))) {
        text += QStringLiteral("技能：冰冻攻击；可能施加 Freeze 状态。");
    } else if (name.contains(QStringLiteral("Caster"), Qt::CaseInsensitive)
               || name.contains(QStringLiteral("法"))) {
        text += QStringLiteral("技能：castState(player)；施加随机状态。 ");
    } else {
        text += QStringLiteral("技能：attack(player)；执行普通攻击函数。");
    }

    return text;
}

QString MainWindow::buildMinionInfoText(int displayIndex, const Minion& minion) const
{
    return QStringLiteral("仆从%1：%2\n生命：%3/%4    护盾：%5    攻击：%6")
        .arg(displayIndex)
        .arg(QString::fromStdString(minion.name))
        .arg(minion.hp)
        .arg(minion.maxHp)
        .arg(minion.shield)
        .arg(minion.getEffectiveAttack());
}

// ==========================
// 按钮槽函数
// ==========================

void MainWindow::on_cardButton1_clicked()
{
    playCardByIndex(0);
}

void MainWindow::on_cardButton2_clicked()
{
    playCardByIndex(1);
}

void MainWindow::on_cardButton3_clicked()
{
    playCardByIndex(2);
}

void MainWindow::on_cardButton4_clicked()
{
    playCardByIndex(3);
}

void MainWindow::on_cardButton5_clicked()
{
    playCardByIndex(4);
}

void MainWindow::on_restartButton_clicked()
{
    startNewGame();
}

void MainWindow::on_helpButton_clicked()
{
    QString helpText =
        QStringLiteral("CodeCraft：C++ 卡牌对战游戏\n\n")
        + QStringLiteral("新的函数调用流程：\n")
        + QStringLiteral("1. 玩家和 Boss 都被视作 C++ 对象。\n")
        + QStringLiteral("2. 玩家出牌不会立即结算，而是写入中间代码块。\n")
        + QStringLiteral("3. 点击结束回合后，代码块会按顺序逐段执行。\n")
        + QStringLiteral("4. 当前执行的语句会用橙红色字体高亮。\n")
        + QStringLiteral("5. player.tickState() 和 boss.tickState() 的实现显示在两侧代码块中。\n")
        + QStringLiteral("6. 中毒、冰冻、力量等状态会改变 tickState() 函数内容，变化行会蓝色渐隐高亮。\n\n")
        + QStringLiteral("目标：在玩家生命归零前击败 Boss。");

    QMessageBox::information(this, QStringLiteral("游戏说明"), helpText);
}

// ==========================
// 工具函数
// ==========================

QRect MainWindow::geometryInCentral(QWidget* widget) const
{
    QPoint topLeft = widget->mapTo(ui->centralwidget, QPoint(0, 0));
    return QRect(topLeft, widget->size());
}

void MainWindow::setCardButtonsEnabled(bool enabled)
{
    if (!gameManager) {
        return;
    }

    QVector<CardView> handView = gameManager->getHandView();

    for (int i = 0; i < cardButtons.size(); ++i) {
        const bool hasCard = i < handView.size() && !handView[i].name.isEmpty();
        const bool enoughEnergy = hasCard && gameManager->player.energy >= handView[i].cost;
        cardButtons[i]->setEnabled(enabled && hasCard && enoughEnergy);
    }

    ui->endTurnButton->setEnabled(enabled);
    ui->restartButton->setEnabled(enabled);
    ui->helpButton->setEnabled(enabled);
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

QString MainWindow::statusTypeName(StatusType type) const
{
    switch (type) {
    case StatusType::BURN:       return QStringLiteral("灼烧");
    case StatusType::POISON:     return QStringLiteral("中毒");
    case StatusType::FREEZE:     return QStringLiteral("冰冻");
    case StatusType::STUN:       return QStringLiteral("眩晕");
    case StatusType::WEAKEN:     return QStringLiteral("虚弱");
    case StatusType::VULNERABLE: return QStringLiteral("易伤");
    case StatusType::STRENGTH:   return QStringLiteral("力量");
    case StatusType::SHIELD:     return QStringLiteral("护盾");
    case StatusType::INVINCIBLE: return QStringLiteral("无敌");
    case StatusType::REGEN:      return QStringLiteral("再生");
    case StatusType::MARK:       return QStringLiteral("标记");
    case StatusType::RAGE:       return QStringLiteral("怒气");
    case StatusType::FORTIFY:    return QStringLiteral("固守");
    case StatusType::CORRODE:    return QStringLiteral("腐蚀");
    case StatusType::DODGE:      return QStringLiteral("闪避");
    case StatusType::CHARGE:     return QStringLiteral("蓄力");
    case StatusType::ECHO:       return QStringLiteral("回响");
    default:                     return QStringLiteral("状态");
    }
}

QString MainWindow::statusVariableName(StatusType type) const
{
    switch (type) {
    case StatusType::BURN:       return QStringLiteral("burn");
    case StatusType::POISON:     return QStringLiteral("poison");
    case StatusType::FREEZE:     return QStringLiteral("freeze");
    case StatusType::STUN:       return QStringLiteral("stun");
    case StatusType::WEAKEN:     return QStringLiteral("weaken");
    case StatusType::VULNERABLE: return QStringLiteral("vulnerable");
    case StatusType::STRENGTH:   return QStringLiteral("strength");
    case StatusType::SHIELD:     return QStringLiteral("shieldState");
    case StatusType::INVINCIBLE: return QStringLiteral("invincible");
    case StatusType::REGEN:      return QStringLiteral("regen");
    case StatusType::MARK:       return QStringLiteral("mark");
    case StatusType::RAGE:       return QStringLiteral("rage");
    case StatusType::FORTIFY:    return QStringLiteral("fortify");
    case StatusType::CORRODE:    return QStringLiteral("corrode");
    case StatusType::DODGE:      return QStringLiteral("dodge");
    case StatusType::CHARGE:     return QStringLiteral("charge");
    case StatusType::ECHO:       return QStringLiteral("echo");
    default:                     return QStringLiteral("state");
    }
}

QString MainWindow::statusSummary(const Status& status) const
{
    return QStringLiteral("%1：数值 %2，%3")
        .arg(statusTypeName(status.type))
        .arg(status.value)
        .arg(turnsText(status.turnsRemaining));
}

QString MainWindow::statusTickCodeLine(const QString& ownerName, const Status& status) const
{
    const QString varName = statusVariableName(status.type);

    switch (status.type) {
    case StatusType::POISON:
        return QStringLiteral("%1.takeDamage(%2, DamageType::POISON); %3.turnsRemaining--;")
            .arg(ownerName).arg(status.value).arg(varName);
    case StatusType::BURN:
        return QStringLiteral("%1.takeDamage(%2, DamageType::FIRE); %3.turnsRemaining--;")
            .arg(ownerName).arg(status.value).arg(varName);
    case StatusType::REGEN:
        return QStringLiteral("%1.heal(%2); %3.turnsRemaining--;")
            .arg(ownerName).arg(status.value).arg(varName);
    case StatusType::FREEZE:
    case StatusType::STUN:
        return QStringLiteral("%1.skipAction = true; %2.turnsRemaining--;")
            .arg(ownerName).arg(varName);
    case StatusType::STRENGTH:
        return QStringLiteral("%1.attackBonus += %2; %3.turnsRemaining--;")
            .arg(ownerName).arg(status.value).arg(varName);
    case StatusType::WEAKEN:
        return QStringLiteral("%1.attackPenalty += %2; %3.turnsRemaining--;")
            .arg(ownerName).arg(status.value).arg(varName);
    case StatusType::VULNERABLE:
        return QStringLiteral("%1.damageTakenBonus += %2; %3.turnsRemaining--;")
            .arg(ownerName).arg(status.value).arg(varName);
    default:
        return QStringLiteral("%1.updateStatus(%2, %3); %4.turnsRemaining--;")
            .arg(ownerName)
            .arg(static_cast<int>(status.type))
            .arg(status.value)
            .arg(varName);
    }
}

int MainWindow::statusValue(StatusType type) const
{
    if (!gameManager) {
        return 0;
    }

    int total = 0;
    for (const Status& status : gameManager->player.statuses) {
        if (status.type == type) {
            total += status.value;
        }
    }
    return total;
}
