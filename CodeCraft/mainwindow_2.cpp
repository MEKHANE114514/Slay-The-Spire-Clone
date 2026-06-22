#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QLabel>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>

#include <algorithm>

namespace {
constexpr int MAIN_EXEC_MS = 800;
constexpr int MAIN_GAP_MS = 450;
constexpr int SIDE_CHANGE_MS = 1400;
constexpr int SIDE_POLL_MS = 80;

const QColor kMainRunColor(210, 70, 0);
const QColor kSideChangeColor(170, 0, 255);
const QColor kCommentColor(80, 150, 80);

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
        "background-color: #F8F9FB;"
        "color: #1F2328;"
        "border: 1px solid #D0D7DE;"
        "border-radius: 6px;"
        "padding: 8px;"
        "}"
        );
}

void MainWindow::resetRuntimeState()
{
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
    gameManager = std::make_unique<GameManager>();

    clearLogs();
    clearCodeHighlight();
    clearSideChangeHighlight();
    appendLog("游戏开始。");

    refreshUi();
    refreshMainCodeEditor();
    beginTurnWithoutAutoDraw();
    startTurnDrawFive();
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
        recycleDiscardToDrawPileAnimation([this, remainingCount]() {
            if (!gameManager) {
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

    drawOneCardAnimation(result.handIndex, result.card, [this, remainingCount]() {
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

    playCardToDiscardAnimation(index, card, [this]() {
        refreshUi();
        refreshMainCodeEditor();
        if (gameManager && !gameManager->isBattleOver()) {
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

        if (result.gameOver || gameManager->isBattleOver()) {
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

        gameManager->executePendingCode(index);
        refreshUi();
        refreshMainCodeEditor();

        if (gameManager->isBattleOver()) {
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
    refreshUi();
    refreshMainCodeEditor();

    QMessageBox::information(
        this,
        "游戏结束",
        gameManager && gameManager->isPlayerWin() ? "胜利！" : "失败！"
        );

    setControlsEnabled(false);
    ui->restartButton->setEnabled(true);
    ui->helpButton->setEnabled(true);
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
    refreshSideCodeEditors();
}

void MainWindow::refreshPlayerUi()
{
    const Player& player = gameManager->player;

    ui->playerHpLabel->setText(QString("玩家生命：%1/%2").arg(player.hp).arg(player.maxHp));
    ui->playerEnergyLabel->setText(QString("玩家能量：%1/%2").arg(player.energy).arg(player.maxEnergy));
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

    // bossSkillLabel 只作为“技能卡片入口”显示。
    // 具体技能说明放在 ToolTip 中，不再混入基础攻击、实际攻击、护盾等属性信息。
    ui->bossSkillLabel->setText("Boss 技能");
    ui->bossSkillLabel->setToolTip(buildBossSpecialSkillText(enemy));
}

void MainWindow::refreshPileUi()
{
    ui->drawPileLabel->setText(QString("抽牌堆\n%1").arg(gameManager->getDrawPileCount()));
    ui->discardPileLabel->setText(QString("弃牌堆\n%1").arg(gameManager->getDiscardPileCount()));
}

void MainWindow::refreshHandUi()
{
    auto handView = gameManager->getHandView();

    for (int i = 0; i < cardButtons.size(); ++i) {
        const bool hasCard = i < static_cast<int>(handView.size())
        && !qstr(handView[i].name).isEmpty();

        if (!hasCard) {
            cardButtons[i]->setText("");
            cardButtons[i]->hide();
            cardButtons[i]->setEnabled(false);
            continue;
        }

        const CardView& card = handView[i];
        cardButtons[i]->setText(formatCardText(card));
        cardButtons[i]->setToolTip(qstr(card.description));
        cardButtons[i]->show();
        cardButtons[i]->setEnabled(controlsEnabled && gameManager->player.energy >= card.cost);
    }
}

void MainWindow::refreshMinionUi()
{
    QLabel* labels[2] = { ui->minionHpLabel1, ui->minionHpLabel2 };
    for (QLabel* label : labels) {
        label->hide();
        label->clear();
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
        ++slot;
    }
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
    QString helpText =
        "CodeCraft：C++ 卡牌对战游戏\n\n"
        "新的函数调用模式：\n"
        "1. 玩家和怪物分别视作由不同类创建的对象。\n"
        "2. 中间代码块展示本回合将依次执行的函数调用。\n"
        "3. 玩家每打出一张牌，不会立刻结算效果，而是向代码块写入对应语句。\n"
        "4. 玩家点击结束回合后，代码块会从上到下依次执行。\n"
        "5. 当前执行语句使用橙红色文字高亮。\n"
        "6. 左右两侧显示 tickStatuses() 的具体实现。\n"
        "7. 状态或技能导致函数实现改变时，改变的代码行会用紫色文字高亮。";

    QMessageBox::information(this, "游戏说明", helpText);
}

// ============================================================
// 工具函数
// ============================================================

QRect MainWindow::geometryInCentral(QWidget* widget) const
{
    QPoint topLeft = widget->mapTo(ui->centralwidget, QPoint(0, 0));
    return QRect(topLeft, widget->size());
}

void MainWindow::setControlsEnabled(bool enabled)
{
    controlsEnabled = enabled;
    refreshHandUi();
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

QString MainWindow::formatCardText(const CardView& card) const
{
    return QString("%1\n费用：%2").arg(qstr(card.name)).arg(card.cost);
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
        return QStringLiteral("无特殊技能");
    }

    QStringList lines;
    for (const std::string& line : enemy->getDescription()) {
        lines << QString::fromStdString(line);
    }

    return lines.isEmpty() ? QStringLiteral("无特殊技能") : lines.join(QStringLiteral("\n"));
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
