#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPropertyAnimation>
#include <QMessageBox>
#include <QTextCursor>
#include <QEasingCurve>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initCardButtons();
    startNewGame();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ==========================
// 初始�?
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

void MainWindow::startNewGame()
{
    gameManager = std::make_unique<GameManager>();

    /*
     * 重要�?
     * 这里暂时不调�?gameManager->startBattle()�?
     * 因为你们现在�?startBattle() 会调�?startTurn()�?
     * startTurn() 又会自动 drawCards()，这会绕�?Qt 抽牌动画�?
     *
     * 如果你们后面把“初始化敌人和牌组”的逻辑写进�?startBattle()�?
     * 建议拆出一�?setupBattleOnly()，只初始化敌人和牌组，不自动抽牌�?
     */

    clearLogs();
    appendLog("游戏开始�?);

    refreshUi();

    beginTurnWithoutAutoDraw();
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

    gameManager->turnNumber++;

    gameManager->player.resetActionLimits();
    gameManager->growMaxEnergy();
    gameManager->restoreEnergy();

    appendLog(QString("�?%1 回合开始�?).arg(gameManager->turnNumber));
    appendLog("开始抽牌�?);

    refreshUi();
}

void MainWindow::startTurnDrawFive()
{
    setCardButtonsEnabled(false);
    drawNextCard(5);
}

void MainWindow::drawNextCard(int remainingCount)
{
    if (!gameManager) {
        return;
    }

    if (remainingCount <= 0) {
        appendLog("抽牌阶段结束�?);
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    // UI 只有 5 个手牌槽，达�?5 张就停止抽牌
    if (static_cast<int>(gameManager->getHandView().size()) >= cardButtons.size()) {
        appendLog("手牌已满，停止抽牌�?);
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    DrawResult result = gameManager->drawOneCard();

    if (result.needRecycle) {
        appendLog("抽牌堆为空，弃牌堆放回抽牌堆�?);

        recycleDiscardToDrawPileAnimation([this, remainingCount]() {
            gameManager->recycleDiscardToDrawPile();

            refreshUi();
            drawNextCard(remainingCount);
        });

        return;
    }

    if (!result.success) {
        appendLog("没有牌可抽�?);
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    appendLog(QString("抽到�?1】�?).arg(toQString(result.card.name)));

    // 这里只刷新牌堆数量，不刷新手牌按钮�?
    // 否则真实手牌按钮会在动画前提前出现�?
    refreshPileUi();

    drawOneCardAnimation(result.handIndex, result.card, [this, remainingCount]() {
        refreshUi();
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
    ghostCard->setText(QString("%1\n费用�?2")
                           .arg(toQString(card.name))
                           .arg(card.cost));
    ghostCard->setToolTip(toQString(card.description));
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

// ==========================
// 出牌
// ==========================

void MainWindow::playCardByIndex(int index)
{
    if (!gameManager) {
        return;
    }

    QVector<CardView> handView = gameManager->getHandView();

    if (index < 0 || index >= static_cast<int>(handView.size())) {
        return;
    }

    CardView card = handView[index];

    Enemy* target = firstAliveEnemy();

    PlayResult result = gameManager->playCard(index, target);

    if (!result.success) {
        appendLog(QString("无法打出卡牌�?1").arg(toQString(result.failReason)));
        refreshUi();
        return;
    }

    appendLog(QString("玩家打出�?1】�?).arg(toQString(result.card.name)));

    playCardToDiscardAnimation(index, card, [this]() {
        refreshUi();

        if (gameManager && gameManager->isBattleOver()) {
            QMessageBox::information(
                this,
                "游戏结束",
                gameManager->isPlayerWin() ? "胜利�? : "失败�?
                );

            setCardButtonsEnabled(false);
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
    ghostCard->setText(QString("%1\n费用�?2")
                           .arg(toQString(card.name))
                           .arg(card.cost));
    ghostCard->setToolTip(toQString(card.description));
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

                if (gameManager && !gameManager->isBattleOver()) {
                    setCardButtonsEnabled(true);
                }
            });

    animation->start();
}

// ==========================
// 结束回合
// ==========================

void MainWindow::on_endTurnButton_clicked()
{
    if (!gameManager) {
        return;
    }

    setCardButtonsEnabled(false);

    appendLog("玩家结束回合�?);

    // 记录剩余手牌进入弃牌�?
    QVector<CardView> handView = gameManager->getHandView();
    for (const CardView& card : handView) {
        appendLog(QString("�?1】进入弃牌堆�?).arg(toQString(card.name)));
    }

    // 直接使用 GameManager 的公开接口，避免调�?endTurn() 自动 startTurn()
    gameManager->discardHand();

    appendLog("怪物发动攻击�?);

    gameManager->battle.executeAttackPhase();

    if (gameManager->isBattleOver()) {
        refreshUi();

        QMessageBox::information(
            this,
            "游戏结束",
            gameManager->isPlayerWin() ? "胜利�? : "失败�?
            );

        setCardButtonsEnabled(false);
        return;
    }

    gameManager->battle.executeEndPhase();

    if (gameManager->isBattleOver()) {
        refreshUi();

        QMessageBox::information(
            this,
            "游戏结束",
            gameManager->isPlayerWin() ? "胜利�? : "失败�?
            );

        setCardButtonsEnabled(false);
        return;
    }

    refreshUi();

    beginTurnWithoutAutoDraw();
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
    refreshPileUi();
    refreshHandUi();
}

void MainWindow::refreshPlayerUi()
{
    const Player& player = gameManager->player;

    ui->playerHpLabel->setText(
        QString("玩家生命�?1/%2")
            .arg(player.hp)
            .arg(player.maxHp)
        );

    ui->playerEnergyLabel->setText(
        QString("玩家能量�?1/%2")
            .arg(player.energy)
            .arg(player.maxEnergy)
        );

    ui->playerShieldLabel->setText(
        QString("玩家护盾�?1")
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
            QString("敌人生命�?1/%2")
                .arg(enemy->hp)
                .arg(enemy->maxHp)
            );
    }

    ui->enemyIntentLabel->setText(
        toQString(gameManager->getEnemyIntentText())
        );
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
        if (i < static_cast<int>(handView.size()) && !handView[i].name.empty()) {
            const CardView& card = handView[i];

            cardButtons[i]->setText(
                QString("%1\n费用�?2")
                    .arg(toQString(card.name))
                    .arg(card.cost)
                );

            cardButtons[i]->setToolTip(toQString(card.description));
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
// 按钮槽函�?
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
        "CodeCraft：C++ 卡牌对战游戏\n\n"
        "游戏目标：\n"
        "在玩家生命归零前击败敌人。\n\n"

        "基本流程：\n"
        "1. 每回合开始时，玩家从抽牌堆随机抽牌。\n"
        "2. 抽到 5 张，或者没有牌可抽时停止抽牌。\n"
        "3. 如果抽牌堆为空，会先把弃牌堆全部放回抽牌堆。\n"
        "4. 玩家点击手牌按钮即可出牌。\n"
        "5. 出牌后，该牌进入弃牌堆。\n"
        "6. 玩家点击“结束回合”后，剩余手牌全部进入弃牌堆。\n"
        "7. 怪物攻击，然后进入下一回合。\n\n"

        "卡牌类型：\n"
        "行为牌：立即执行一次动作，例如普通攻击、防御。\n"
        "函数牌：修改玩家函数效果，例如攻击函数·强化。\n"
        "模板牌：包装函数调用，例如三连击。\n\n"

        "当前版本说明：\n"
        "本版本实现抽牌堆、手牌、弃牌堆和基础动画流程�?;

    QMessageBox::information(this, "游戏说明", helpText);
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
        bool hasCard =
            i < static_cast<int>(handView.size()) &&
            !handView[i].name.empty();

        bool enoughEnergy =
            hasCard &&
            gameManager->player.energy >= handView[i].cost;

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

QString MainWindow::toQString(const std::string& s) const
{
    return QString::fromStdString(s);
}
