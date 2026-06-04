#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QTextCursor>
#include <QEasingCurve>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initCardButtons();
    initUi();

    initDemoPiles();
    clearHand();
    refreshUi();

    startTurnDrawFive();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ==========================
// 初始化部分
// ==========================

void MainWindow::initUi()
{
    setWindowTitle("CodeCraft - C++ 卡牌对战");

    ui->playerHpLabel->setText("玩家生命：50/50");
    ui->playerEnergyLabel->setText("玩家能量：3");
    ui->playerShieldLabel->setText("玩家护盾：0");

    ui->enemyHpLabel->setText("敌人生命：60/60");
    ui->enemyIntentLabel->setText("敌人意图：攻击 8");

    ui->logTextEdit->setReadOnly(true);

    clearLogs();
    appendLog("游戏开始。");
    appendLog("回合开始，开始抽牌。");
}

void MainWindow::initCardButtons()
{
    cardButtons.clear();

    cardButtons.push_back(ui->cardButton1);
    cardButtons.push_back(ui->cardButton2);
    cardButtons.push_back(ui->cardButton3);
    cardButtons.push_back(ui->cardButton4);
    cardButtons.push_back(ui->cardButton5);
}

void MainWindow::initDemoPiles()
{
    drawPile.clear();
    hand.clear();
    discardPile.clear();

    hand.resize(cardButtons.size());

    // 这里是临时测试牌库，后面由 GameState 生成
    drawPile.push_back({"普通攻击", "调用 attack()，造成基础伤害。", 1});
    drawPile.push_back({"防御", "获得 5 点护盾。", 1});
    drawPile.push_back({"全力一击", "调用强化版 attack()。", 2});
    drawPile.push_back({"攻击函数·强化", "永久修改 attack()，之后攻击伤害增加。", 1});
    drawPile.push_back({"三连击", "下一次 attack() 连续执行三次。", 2});

    drawPile.push_back({"普通攻击", "调用 attack()，造成基础伤害。", 1});
    drawPile.push_back({"防御", "获得 5 点护盾。", 1});
    drawPile.push_back({"治疗", "恢复少量生命。", 1});
    drawPile.push_back({"攻击函数·吸血", "永久修改 attack()，之后攻击回血。", 2});
    drawPile.push_back({"受击函数·铁壁", "永久修改 takeDamage()，减少受伤。", 1});
}

// ==========================
// 日志部分
// ==========================

void MainWindow::appendLog(const QString& text)
{
    logs << text;
    refreshLogUi();
}

void MainWindow::appendLogs(const QStringList& texts)
{
    for (const QString& text : texts) {
        logs << text;
    }

    refreshLogUi();
}

void MainWindow::clearLogs()
{
    logs.clear();
    refreshLogUi();
}

void MainWindow::refreshLogUi()
{
    ui->logTextEdit->setPlainText(logs.join("\n"));

    QTextCursor cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logTextEdit->setTextCursor(cursor);
}

// ==========================
// 刷新界面
// ==========================

void MainWindow::refreshUi()
{
    updatePileLabels();
    refreshHandUi();
}

void MainWindow::updatePileLabels()
{
    ui->drawPileLabel->setText(QString("抽牌堆\n%1").arg(drawPile.size()));
    ui->discardPileLabel->setText(QString("弃牌堆\n%1").arg(discardPile.size()));
}

void MainWindow::refreshHandUi()
{
    for (int i = 0; i < cardButtons.size(); ++i) {
        if (i < hand.size() && !hand[i].name.isEmpty()) {
            const CardView& card = hand[i];

            cardButtons[i]->setText(
                QString("%1\n费用：%2").arg(card.name).arg(card.cost)
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
// 手牌状态
// ==========================

void MainWindow::clearHand()
{
    if (hand.size() != cardButtons.size()) {
        hand.resize(cardButtons.size());
    }

    for (int i = 0; i < hand.size(); ++i) {
        hand[i] = CardView{};
    }

    refreshHandUi();
}

void MainWindow::setCardButtonsEnabled(bool enabled)
{
    for (int i = 0; i < cardButtons.size(); ++i) {
        bool hasCard = (i < hand.size() && !hand[i].name.isEmpty());
        cardButtons[i]->setEnabled(enabled && hasCard);
    }

    ui->endTurnButton->setEnabled(enabled);
    ui->restartButton->setEnabled(enabled);
    ui->helpButton->setEnabled(enabled);
}

int MainWindow::firstEmptyHandIndex() const
{
    for (int i = 0; i < hand.size(); ++i) {
        if (hand[i].name.isEmpty()) {
            return i;
        }
    }

    return -1;
}

// ==========================
// 坐标转换
// ==========================

QRect MainWindow::geometryInCentral(QWidget* widget) const
{
    QPoint topLeft = widget->mapTo(ui->centralwidget, QPoint(0, 0));
    return QRect(topLeft, widget->size());
}

// ==========================
// 抽牌逻辑与动画
// ==========================

void MainWindow::startTurnDrawFive()
{
    setCardButtonsEnabled(false);
    drawNextCard(5);
}

void MainWindow::drawNextCard(int remainingCount)
{
    if (remainingCount <= 0) {
        appendLog("抽牌阶段结束。");
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    int handIndex = firstEmptyHandIndex();

    if (handIndex == -1) {
        appendLog("手牌已满，停止抽牌。");
        refreshUi();
        setCardButtonsEnabled(true);
        return;
    }

    if (drawPile.isEmpty()) {
        if (discardPile.isEmpty()) {
            appendLog("抽牌堆和弃牌堆都为空，无法继续抽牌。");
            refreshUi();
            setCardButtonsEnabled(true);
            return;
        }

        appendLog("抽牌堆为空，弃牌堆重新放入抽牌堆。");

        recycleDiscardToDrawPileAnimation([this, remainingCount]() {
            drawPile = discardPile;
            discardPile.clear();

            refreshUi();

            drawNextCard(remainingCount);
        });

        return;
    }

    int randomIndex = QRandomGenerator::global()->bounded(drawPile.size());
    CardView card = drawPile[randomIndex];
    drawPile.removeAt(randomIndex);

    appendLog(QString("抽到【%1】。").arg(card.name));
    updatePileLabels();

    drawOneCardAnimation(handIndex, card, [this, remainingCount]() {
        refreshUi();
        drawNextCard(remainingCount - 1);
    });
}

void MainWindow::drawOneCardAnimation(int handIndex,
                                      const CardView& card,
                                      std::function<void()> onFinished)
{
    QRect startRect = geometryInCentral(ui->drawPileLabel);
    QRect endRect = geometryInCentral(cardButtons[handIndex]);

    QPushButton* ghostCard = new QPushButton(ui->centralwidget);
    ghostCard->setText(QString("%1\n费用：%2").arg(card.name).arg(card.cost));
    ghostCard->setGeometry(startRect);
    ghostCard->show();
    ghostCard->raise();

    QPropertyAnimation* animation = new QPropertyAnimation(ghostCard, "geometry");
    animation->setDuration(220);
    animation->setStartValue(startRect);
    animation->setEndValue(endRect);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(animation, &QPropertyAnimation::finished, this,
            [this, handIndex, card, ghostCard, animation, onFinished]() {
                hand[handIndex] = card;

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
    ghostPile->setText(QString("洗牌\n%1").arg(discardPile.size()));
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
// 出牌逻辑与动画
// ==========================

void MainWindow::playCardByIndex(int index)
{
    if (index < 0 || index >= hand.size()) {
        return;
    }

    if (hand[index].name.isEmpty()) {
        return;
    }

    CardView card = hand[index];

    appendLog(QString("玩家打出【%1】。").arg(card.name));

    playCardToDiscardAnimation(index, card, [this]() {
        refreshUi();
    });
}

void MainWindow::playCardToDiscardAnimation(int index,
                                            const CardView& card,
                                            std::function<void()> onFinished)
{
    if (index < 0 || index >= cardButtons.size()) {
        return;
    }

    setCardButtonsEnabled(false);

    QRect startRect = geometryInCentral(cardButtons[index]);
    QRect endRect = geometryInCentral(ui->discardPileLabel);

    QPushButton* ghostCard = new QPushButton(ui->centralwidget);
    ghostCard->setText(QString("%1\n费用：%2").arg(card.name).arg(card.cost));
    ghostCard->setGeometry(startRect);
    ghostCard->show();
    ghostCard->raise();

    cardButtons[index]->hide();

    // 当前阶段：真实数据在 MainWindow 中临时维护
    hand[index] = CardView{};
    discardPile.push_back(card);

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

                setCardButtonsEnabled(true);
            });

    animation->start();
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

void MainWindow::on_endTurnButton_clicked()
{
    setCardButtonsEnabled(false);

    appendLog("玩家结束回合。");

    // 剩余手牌全部进入弃牌堆
    for (int i = 0; i < hand.size(); ++i) {
        if (!hand[i].name.isEmpty()) {
            discardPile.push_back(hand[i]);
            appendLog(QString("【%1】进入弃牌堆。").arg(hand[i].name));
            hand[i] = CardView{};
        }
    }

    refreshUi();

    // 当前阶段先用文本模拟怪物攻击，后续交给 GameState
    appendLog("怪物发动攻击。");
    appendLog("新的回合开始。");

    startTurnDrawFive();
}

void MainWindow::on_restartButton_clicked()
{
    initUi();

    initDemoPiles();
    clearHand();
    refreshUi();

    appendLog("游戏已重新开始。");
    appendLog("回合开始，开始抽牌。");

    startTurnDrawFive();
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
        "本版本先实现抽牌堆、手牌、弃牌堆和基础动画流程。";

    QMessageBox::information(this, "游戏说明", helpText);
}