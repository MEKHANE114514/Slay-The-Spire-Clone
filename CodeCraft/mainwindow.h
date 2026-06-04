#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPushButton>
#include <QStringList>
#include <QRect>

#include <functional>
#include <memory>

#include "game_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class Enemy;

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

    // GameManager 相当于 GameState
    std::unique_ptr<GameManager> gameManager;

    // 日志缓存，不直接到处操作 logTextEdit
    QStringList logs;

    // 固定 5 个手牌按钮
    QVector<QPushButton*> cardButtons;

    // 初始化
    void startNewGame();
    void initCardButtons();

    // 回合流程
    void beginTurnWithoutAutoDraw();
    void startTurnDrawFive();
    void drawNextCard(int remainingCount);

    // 出牌流程
    void playCardByIndex(int index);

    // 动画
    void drawOneCardAnimation(int handIndex,
                              const CardView& card,
                              std::function<void()> onFinished);

    void playCardToDiscardAnimation(int index,
                                    const CardView& card,
                                    std::function<void()> onFinished);

    void recycleDiscardToDrawPileAnimation(std::function<void()> onFinished);

    // 刷新界面
    void refreshUi();
    void refreshPlayerUi();
    void refreshEnemyUi();
    void refreshPileUi();
    void refreshHandUi();

    // 日志
    void appendLog(const QString& text);
    void clearLogs();
    void refreshLogUi();

    // 工具
    QRect geometryInCentral(QWidget* widget) const;
    void setCardButtonsEnabled(bool enabled);
    Enemy* firstAliveEnemy() const;
    QString toQString(const std::string& s) const;
};

#endif // MAINWINDOW_H