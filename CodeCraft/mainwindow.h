#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPushButton>
#include <QStringList>
#include <QRect>
#include <QSet>
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
class QPlainTextEdit;
class QVariantAnimation;

struct CodeRange {
    int startLine = 0;   // QPlainTextEdit 中的 0 基行号
    int lineCount = 1;   // if / for 等结构需要整段高亮
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

    std::unique_ptr<GameManager> gameManager;

    QStringList logs;
    QVector<QPushButton*> cardButtons;

    // 中间主代码块：每个 CodeCommandView 对应一个可整段执行/高亮的范围
    QVector<CodeRange> mainCodeRanges;
    int activeMainCodeIndex = -1;

    // 两侧 tickState() 函数块：函数被改变时用蓝色渐隐高亮改变行
    QStringList lastPlayerTickLines;
    QStringList lastEnemyTickLines;
    QSet<int> changedPlayerTickLines;
    QSet<int> changedEnemyTickLines;
    QVariantAnimation* functionChangeFadeAnimation = nullptr;
    qreal functionChangeFadeAlpha = 0.0;

    // 初始化
    void startNewGame();
    void initCardButtons();
    void initCodeEditors();

    // 回合流程
    void beginTurnWithoutAutoDraw();
    void startTurnDrawFive();
    void drawNextCard(int remainingCount);
    void executeCodeQueue();
    void executeNextCode(int index);
    void finishCodeExecutionAndEnterNextTurn();

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
    void refreshBossSkillUi();
    void refreshMinionUi();
    void refreshPileUi();
    void refreshHandUi();

    // 代码块显示与高亮
    void refreshAllCodeEditors(bool markFunctionChanges);
    void refreshMainCodeEditor();
    void refreshFunctionCodeEditors(bool markChanges);

    void highlightMainCodeBlock(int commandIndex);
    void clearMainCodeHighlight();

    void applyMainCodeTextStyles();
    void applyFunctionCodeTextStyles();
    void addCommentTextStyles(QPlainTextEdit* editor,
                              QList<QTextEdit::ExtraSelection>& selections) const;
    void addFullLineSelection(QPlainTextEdit* editor,
                              QList<QTextEdit::ExtraSelection>& selections,
                              int line,
                              const QTextCharFormat& format) const;

    QStringList buildPlayerTickFunctionLines() const;
    QStringList buildEnemyTickFunctionLines() const;
    void markChangedFunctionLines(const QStringList& oldLines,
                                  const QStringList& newLines,
                                  QSet<int>& changedLines) const;
    void startFunctionChangeFadeAnimation();

    // 信息文本
    QString buildBossSkillText(Enemy* enemy) const;
    QString buildMinionInfoText(int displayIndex, const Minion& minion) const;

    // 日志
    void appendLog(const QString& text);
    void clearLogs();
    void refreshLogUi();

    // 工具
    QRect geometryInCentral(QWidget* widget) const;
    void setCardButtonsEnabled(bool enabled);
    Enemy* firstAliveEnemy() const;

    QString statusTypeName(StatusType type) const;
    QString statusVariableName(StatusType type) const;
    QString statusSummary(const Status& status) const;
    QString statusTickCodeLine(const QString& ownerName, const Status& status) const;
    int statusValue(StatusType type) const;
};

#endif // MAINWINDOW_H
