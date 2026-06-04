#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QRect>

#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct CardView {
    QString name;
    QString description;
    int cost = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_cardButton1_clicked();
    void on_cardButton2_clicked();
    void on_cardButton3_clicked();
    void on_cardButton4_clicked();
    void on_cardButton5_clicked();

    void on_endTurnButton_clicked();
    void on_restartButton_clicked();
    void on_helpButton_clicked();

private:
    Ui::MainWindow *ui;

    // 日志缓存：不要到处直接操作 logTextEdit
    QStringList logs;

    // 手牌按钮：固定 5 个手牌槽位
    QVector<QPushButton*> cardButtons;

    // 当前阶段的临时牌堆，后续会被 GameState 替代
    QVector<CardView> drawPile;
    QVector<CardView> hand;
    QVector<CardView> discardPile;

    // 初始化
    void initUi();
    void initCardButtons();
    void initDemoPiles();

    // 刷新界面
    void refreshUi();
    void refreshHandUi();
    void updatePileLabels();

    // 日志
    void appendLog(const QString& text);
    void appendLogs(const QStringList& texts);
    void clearLogs();
    void refreshLogUi();

    // 手牌与按钮状态
    void clearHand();
    void setCardButtonsEnabled(bool enabled);
    int firstEmptyHandIndex() const;

    // 坐标转换
    QRect geometryInCentral(QWidget* widget) const;

    // 抽牌相关
    void startTurnDrawFive();
    void drawNextCard(int remainingCount);
    void drawOneCardAnimation(int handIndex,
                              const CardView& card,
                              std::function<void()> onFinished);

    // 出牌相关
    void playCardByIndex(int index);
    void playCardToDiscardAnimation(int index,
                                    const CardView& card,
                                    std::function<void()> onFinished);

    // 洗牌动画：弃牌堆 -> 抽牌堆
    void recycleDiscardToDrawPileAnimation(std::function<void()> onFinished);
};

#endif // MAINWINDOW_H