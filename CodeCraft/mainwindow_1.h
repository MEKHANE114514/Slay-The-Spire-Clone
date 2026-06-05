#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPushButton>
#include <QStringList>
#include <QRect>
#include <QTextEdit>

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
    struct CodeRange {
        int startLine = -1;   // QPlainTextEdit 中的 0-based 行号
        int lineCount = 0;    // 需要高亮几行；for/if 块会大于 1
    };

private:
    Ui::MainWindow *ui;

    std::unique_ptr<GameManager> gameManager;

    QStringList logs;
    QVector<QPushButton*> cardButtons;
    QVector<CodeRange> codeRanges;
    int activeCodeCommandIndex = -1; // 当前正在执行/高亮的代码段编号，-1 表示没有

    int executionToken = 0;       // 重新开始时让旧的 QTimer 回调失效
    bool controlsEnabled = false; // 当前是否允许玩家点击卡牌/结束回合

    // 初始化
    void initCardButtons();
    void initCodeEditor();
    void startNewGame();

    // 回合流程
    void beginTurnWithoutAutoDraw();
    void startTurnDrawFive();
    void drawNextCard(int remainingCount);

    // 出牌流程：出牌只写入代码，不立即结算效果
    void playCardByIndex(int index);

    // 结束回合后的代码执行流程
    void executeCodeQueue();
    void executeNextCode(int index, int token);
    void showGameOverMessage();

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
    void refreshCodeEditor();

    // 代码高亮
    void highlightCodeBlock(int commandIndex);
    void clearCodeHighlight();
    void applyCodeTextStyles();
    void addCommentTextStyles(QList<QTextEdit::ExtraSelection>& selections) const;

    // 日志
    void appendLog(const QString& text);
    void clearLogs();
    void refreshLogUi();

    // 工具
    QRect geometryInCentral(QWidget* widget) const;
    void setControlsEnabled(bool enabled);
    Enemy* firstAliveEnemy() const;
    QString formatCardText(const CardView& card) const;
};

#endif // MAINWINDOW_H
