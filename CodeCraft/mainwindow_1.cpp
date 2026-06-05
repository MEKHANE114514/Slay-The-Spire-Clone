#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPropertyAnimation>
#include <QMessageBox>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QColor>
#include <QEasingCurve>
#include <QFont>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initCardButtons();
    initCodeEditor();
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
    cardButtons.clear();
    cardButtons.push_back(ui->cardButton1);
    cardButtons.push_back(ui->cardButton2);
    cardButtons.push_back(ui->cardButton3);
    cardButtons.push_back(ui->cardButton4);
    cardButtons.push_back(ui->cardButton5);
}

void MainWindow::initCodeEditor()
{
    ui->codePlainTextEdit->setReadOnly(true);
    ui->codePlainTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont codeFont("Consolas");
    codeFont.setStyleHint(QFont::Monospace);
    codeFont.setPointSize(11);
    ui->codePlainTextEdit->setFont(codeFont);
}

void MainWindow::startNewGame()
{
    ++executionToken; // 让旧动画/旧定时回调失效

    gameManager = std::make_unique<GameManager>();

    clearLogs();
    clearCodeHighlight();
    appendLog("游戏开始。");

    refreshUi();
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
    appendLog("怪物意图已写入代码块。开始抽牌。");

    refreshUi();
    refreshCodeEditor();
}

void MainWindow::startTurnDrawFive()
{
    setControlsEnabled(false);
    drawNextCard(5);
}

void MainWindow::drawNextCard(int remainingCount)
{
    if (!gameManager) {
        return;
    }

    if (remainingCount <= 0) {
        appendLog("抽牌阶段结束。请出牌，出牌会写入代码块，暂不立即结算。 ");
        refreshUi();
        refreshCodeEditor();
        setControlsEnabled(true);
        return;
    }

    // UI 当前只有 5 个手牌按钮，到 5 张就停止抽牌。
    if (gameManager->getHandView().size() >= cardButtons.size()) {
        appendLog("手牌已满，停止抽牌。");
        refreshUi();
        refreshCodeEditor();
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
            refreshCodeEditor();
            setControlsEnabled(false);
            drawNextCard(remainingCount);
        });

        return;
    }

    if (!result.success) {
        appendLog("没有牌可抽。 ");
        refreshUi();
        refreshCodeEditor();
        setControlsEnabled(true);
        return;
    }

    appendLog(QString("抽到【%1】。").arg(result.card.name));

    // 只刷新牌堆，不刷新手牌，避免正式按钮在动画前提前出现。
    refreshPileUi();

    drawOneCardAnimation(result.handIndex, result.card, [this, remainingCount]() {
        refreshUi();
        refreshCodeEditor();
        setControlsEnabled(false); // 抽牌阶段还没结束，继续禁用点击
        drawNextCard(remainingCount - 1);
    });
}

// ============================================================
// 出牌：写入代码，不立即结算卡牌效果
// ============================================================

void MainWindow::playCardByIndex(int index)
{
    if (!gameManager || !controlsEnabled) {
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
        appendLog(QString("无法写入代码：%1").arg(result.failReason));
        refreshUi();
        refreshCodeEditor();
        return;
    }

    appendLog(QString("玩家打出【%1】，对应代码已写入代码块。").arg(result.card.name));

    playCardToDiscardAnimation(index, card, [this]() {
        refreshUi();
        refreshCodeEditor();

        if (gameManager && !gameManager->isBattleOver()) {
            setControlsEnabled(true);
        }
    });
}

// ============================================================
// 代码执行流程
// ============================================================

void MainWindow::on_endTurnButton_clicked()
{
    if (!gameManager || !controlsEnabled) {
        return;
    }

    setControlsEnabled(false);

    appendLog("玩家结束回合。剩余手牌进入弃牌堆。 ");

    QVector<CardView> handView = gameManager->getHandView();
    for (const CardView& card : handView) {
        appendLog(QString("【%1】进入弃牌堆。").arg(card.name));
    }

    gameManager->discardHand();
    refreshUi();
    refreshCodeEditor();

    appendLog("开始按顺序执行代码块。 ");
    executeCodeQueue();
}

void MainWindow::executeCodeQueue()
{
    int token = ++executionToken;
    executeNextCode(0, token);
}

void MainWindow::executeNextCode(int index, int token)
{
    if (token != executionToken || !gameManager) {
        return;
    }

    if (index >= gameManager->pendingCodeCount()) {
        clearCodeHighlight();

        TurnResult result = gameManager->finishTurnAfterCodeExecution();
        refreshUi();
        refreshCodeEditor();

        if (result.gameOver || gameManager->isBattleOver()) {
            showGameOverMessage();
            return;
        }

        beginTurnWithoutAutoDraw();
        startTurnDrawFive();
        return;
    }

    highlightCodeBlock(index);

    QTimer::singleShot(700, this, [this, index, token]() {
        if (token != executionToken || !gameManager) {
            return;
        }

        gameManager->executePendingCode(index);
        refreshUi();
        refreshCodeEditor();

        if (gameManager->isBattleOver()) {
            clearCodeHighlight();
            showGameOverMessage();
            return;
        }

        QTimer::singleShot(350, this, [this, index, token]() {
            if (token != executionToken) {
                return;
            }
            executeNextCode(index + 1, token);
        });
    });
}

void MainWindow::showGameOverMessage()
{
    clearCodeHighlight();
    refreshUi();

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
// 抽牌、洗牌、出牌动画
// ============================================================

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
    ghostCard->setText(formatCardText(card));
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

    QRect startRect = geometryInCentral(cardButtons[index]);
    QRect endRect = geometryInCentral(ui->discardPileLabel);

    QPushButton* ghostCard = new QPushButton(ui->centralwidget);
    ghostCard->setText(formatCardText(card));
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
    ghostPile->setText("洗牌");
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
}

void MainWindow::refreshPlayerUi()
{
    const Player& player = gameManager->player;

    ui->playerHpLabel->setText(
        QString("玩家生命：%1/%2")
            .arg(player.hp)
            .arg(player.maxHp)
    );

    ui->playerEnergyLabel->setText(
        QString("玩家能量：%1/%2")
            .arg(player.energy)
            .arg(player.maxEnergy)
    );

    ui->playerShieldLabel->setText(
        QString("玩家护盾：%1")
            .arg(player.shield)
    );
}

void MainWindow::refreshEnemyUi()
{
    Enemy* enemy = firstAliveEnemy();

    if (enemy == nullptr) {
        ui->enemyHpLabel->setText("敌人生命：无");
    } else {
        ui->enemyHpLabel->setText(
            QString("敌人生命：%1/%2")
                .arg(enemy->hp)
                .arg(enemy->maxHp)
        );
    }
}

void MainWindow::refreshPileUi()
{
    ui->drawPileLabel->setText(
        QString("抽牌堆\n%1")
            .arg(gameManager->getDrawPileCount())
    );

    ui->discardPileLabel->setText(
        QString("弃牌堆\n%1")
            .arg(gameManager->getDiscardPileCount())
    );
}

void MainWindow::refreshHandUi()
{
    QVector<CardView> handView = gameManager->getHandView();

    for (int i = 0; i < cardButtons.size(); ++i) {
        if (i < handView.size() && !handView[i].name.isEmpty()) {
            const CardView& card = handView[i];

            cardButtons[i]->setText(formatCardText(card));
            cardButtons[i]->setToolTip(card.description);
            cardButtons[i]->show();

            bool enoughEnergy = gameManager->player.energy >= card.cost;
            cardButtons[i]->setEnabled(controlsEnabled && enoughEnergy);
        } else {
            cardButtons[i]->setText("");
            cardButtons[i]->hide();
            cardButtons[i]->setEnabled(false);
        }
    }
}

void MainWindow::refreshCodeEditor()
{
    if (!gameManager) {
        return;
    }

    QVector<CodeCommandView> commands = gameManager->getCodeCommandViews();

    QStringList lines;
    QVector<CodeRange> ranges;

    lines << "{";
    lines << "    // 请在此输入代码";

    int currentLine = 2;

    for (const CodeCommandView& cmd : commands) {
        CodeRange range;
        range.startLine = currentLine;
        range.lineCount = cmd.lines.size();

        if (range.lineCount <= 0) {
            continue;
        }

        for (const QString& line : cmd.lines) {
            lines << "    " + line;
        }

        ranges.push_back(range);
        currentLine += range.lineCount;
    }

    lines << "}";

    codeRanges = ranges;
    ui->codePlainTextEdit->setPlainText(lines.join("\n"));
    applyCodeTextStyles();
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

    activeCodeCommandIndex = commandIndex;
    applyCodeTextStyles();
}

void MainWindow::clearCodeHighlight()
{
    activeCodeCommandIndex = -1;

    if (ui && ui->codePlainTextEdit) {
        applyCodeTextStyles();
    }
}

void MainWindow::applyCodeTextStyles()
{
    if (!ui || !ui->codePlainTextEdit) {
        return;
    }

    QList<QTextEdit::ExtraSelection> selections;
    QTextDocument* document = ui->codePlainTextEdit->document();

    // 1. 当前执行代码段：只改变字体颜色和加粗，不再设置背景色。
    if (activeCodeCommandIndex >= 0 && activeCodeCommandIndex < codeRanges.size()) {
        const CodeRange& range = codeRanges[activeCodeCommandIndex];

        for (int i = 0; i < range.lineCount; ++i) {
            QTextBlock block = document->findBlockByNumber(range.startLine + i);

            if (!block.isValid()) {
                continue;
            }

            QTextEdit::ExtraSelection selection;
            selection.cursor = QTextCursor(block);
            selection.cursor.select(QTextCursor::LineUnderCursor);
            selection.format.setForeground(QColor(220, 90, 40));
            selection.format.setFontWeight(QFont::Bold);
            selections.append(selection);
        }
    }

    // 2. 注释颜色：把 // 后面的内容显示为绿色。
    //    放在后面添加，可以保证当前执行行中的注释部分仍然保持注释颜色。
    addCommentTextStyles(selections);

    ui->codePlainTextEdit->setExtraSelections(selections);
}

void MainWindow::addCommentTextStyles(QList<QTextEdit::ExtraSelection>& selections) const
{
    if (!ui || !ui->codePlainTextEdit) {
        return;
    }

    QTextDocument* document = ui->codePlainTextEdit->document();

    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        const QString text = block.text();
        const int commentPos = text.indexOf("//");

        if (commentPos < 0) {
            continue;
        }

        QTextEdit::ExtraSelection selection;
        QTextCursor cursor(document);
        cursor.setPosition(block.position() + commentPos);
        cursor.setPosition(block.position() + text.length(), QTextCursor::KeepAnchor);

        selection.cursor = cursor;
        selection.format.setForeground(QColor(80, 150, 80));
        selection.format.setFontItalic(true);
        selections.append(selection);
    }
}

// ============================================================
// 按钮槽函数
// ============================================================

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
        "CodeCraft：C++ 卡牌对战游戏\n\n"
        "新的代码执行模式：\n"
        "1. 玩家和怪物分别视作由不同类创建的对象。\n"
        "2. 每回合开始时，代码块会展示怪物将调用的函数。\n"
        "3. 玩家每打出一张牌，不会立刻产生效果，而是向代码块写入对应语句。\n"
        "4. 玩家点击结束回合后，代码块会从上到下依次执行。\n"
        "5. 执行到哪一条语句，界面就会高亮哪一条。\n"
        "6. if / for 等复合语句会作为一个整体高亮和执行。\n\n"
        "示例：\n"
        "player.attack(enemy);\n"
        "for (int i = 0; i < 3; ++i) {\n"
        "    player.attack(enemy);\n"
        "}\n"
        "enemy.attack(player);";

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
    return QString("%1\n费用：%2").arg(card.name).arg(card.cost);
}
