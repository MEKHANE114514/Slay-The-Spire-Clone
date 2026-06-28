#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRect>
#include <QSet>
#include <QVector>
#include <QStringList>
#include <QEasingCurve>
#include <QLabel>
#include <QColor>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "game_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Enemy;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;

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
    enum class Side { Player, Enemy };

    struct CodeRange {
        int startLine = -1;
        int lineCount = 0;
        bool callsPlayerTick = false;
        bool callsEnemyTick = false;
    };

    struct SideCodeState {
        QPlainTextEdit* editor = nullptr;
        QStringList displayedLines;
        QStringList bodyLines;
        QSet<int> changedLines;
        QSet<int> executingLines;
    };

    struct SideHighlightRequest {
        Side side = Side::Player;
        QSet<int> lines;
    };

    struct EnemySlotWidgets {
        QWidget* root = nullptr;
        QLabel* image = nullptr;
        QLabel* info = nullptr;
        QLabel* intent = nullptr;
        int enemyIndex = -1;
    };

private:
    Ui::MainWindow *ui = nullptr;
    std::unique_ptr<GameManager> gameManager;

    QStringList logs;
    QVector<QPushButton*> cardButtons;
    QVector<CodeRange> codeRanges;
    QVector<EnemySlotWidgets> enemySlots;

    SideCodeState playerCode;
    SideCodeState enemyCode;
    QVector<SideHighlightRequest> sideHighlightQueue;

    QWidget* gameOverOverlay = nullptr;
    QWidget* helpOverlay = nullptr;
    QWidget* mapOverlay = nullptr;
    QWidget* rewardOverlay = nullptr;
    QWidget* seedOverlay = nullptr;
    QWidget* startOverlay = nullptr;

    int activeCodeIndex = -1;
    int selectedEnemyIndex = -1;
    int inspectedEnemyIndex = -1;
    int executingEnemyIndex = -1;
    int executionToken = 0;
    int sideHighlightToken = 0;
    bool controlsEnabled = false;
    bool sideChangeHighlightActive = false;

    // 初始化 / 流程
    void initCardButtons();
    void initCodeEditors();
    void setupCodeEditor(QPlainTextEdit* editor);
    void initTheme();
    void initImageAssets();
    void initCharacterContrast();
    void initResourceContrast();
    void initOverlays();
    void positionOverlays();
    void positionTitleLabel();
    void resetRuntimeState();
    void startNewGame();
    void beginNewGameWithSeed(int seed);
    void showStartOverlay();
    void hideStartOverlay();
    void showSeedOverlay();
    void hideSeedOverlay();
    void beginTurnWithoutAutoDraw();
    void startTurnDrawFive();
    void drawNextCard(int remainingCount);

    // 出牌 / 执行
    void playCardByIndex(int index);
    void executeCodeQueue();
    void executeNextCode(int index, int token);
    void showGameOverMessage();
    void showGameResultOverlay(bool playerWin);
    void hideGameResultOverlay();
    void showMapOverlay();
    void hideMapOverlay();
    void showRewardOverlay();
    void hideRewardOverlay();
    void finishRewardAndShowMap();
    void enterMapNode(int nodeId);
    void showHelpOverlay();
    void hideHelpOverlay();

    // 执行反馈动画
    void playCommandFeedback(const CodeCommandView& command,
                             int oldPlayerHp,
                             int oldPlayerShield,
                             int oldEnemyHp,
                             int oldEnemyShield);
    void animateActorNudge(QWidget* widget, int dx);
    void animateActorShake(QWidget* widget);
    void showFloatingText(QWidget* anchor,
                          const QString& text,
                          const QColor& color);

    // 动画
    void animateGhost(const QRect& startRect,
                      const QRect& endRect,
                      const QString& text,
                      const QString& toolTip,
                      int duration,
                      QEasingCurve::Type easing,
                      std::function<void()> onFinished);
    void drawOneCardAnimation(int handIndex, const CardView& card,
                              std::function<void()> onFinished);
    void playCardToDiscardAnimation(int index, const CardView& card,
                                    std::function<void()> onFinished);
    void recycleDiscardToDrawPileAnimation(std::function<void()> onFinished);

    // 刷新
    void refreshUi();
    void refreshPlayerUi();
    void refreshEnemyUi();
    void refreshEnemySlots();
    void clearEnemySlots();
    void positionEnemySlots();
    void updateEnemySlotStyles();
    void refreshPileUi();
    void refreshHandUi();
    void refreshMinionUi();
    void refreshImageUi();
    void refreshMainCodeEditor();
    void refreshSideCodeEditors();
    void updateSideCode(Side side, const QStringList& newDisplayedLines);

    // 代码高亮
    void highlightCodeBlock(int commandIndex);
    void clearCodeHighlight();
    void applyMainCodeStyle();
    void applySideCodeStyle(SideCodeState& state);
    void applyLineTextStyle(QPlainTextEdit* editor, const QSet<int>& lines,
                            const QColor& color, bool bold = true);
    void addCommentStyle(QPlainTextEdit* editor,
                         QList<QTextEdit::ExtraSelection>& selections) const;

    void clearSideExecutionHighlight();
    void setSideExecutionHighlight(const CodeRange& range);
    void syncSideExecutionHighlightWithActiveCode();
    void clearSideChangeHighlight();
    void enqueueSideChangeHighlight(Side side, const QSet<int>& lines);
    void startNextSideChangeHighlight();

    // 日志 / 工具
    void appendLog(const QString& text);
    void clearLogs();
    void refreshLogUi();
    void setControlsEnabled(bool enabled);

    QRect geometryInCentral(QWidget* widget) const;
    void setScaledPixmap(QLabel* label,
                         const QString& resourcePath,
                         Qt::AspectRatioMode mode = Qt::KeepAspectRatio);
    void applyActorImageStyle(QLabel* label, const QColor& glowColor, int blurRadius);
    void applyResourceIconStyle(QLabel* label, const QColor& glowColor, int blurRadius, bool circular = false);
    void applyResourceCountStyle(QLabel* label, const QColor& glowColor);
    Enemy* firstAliveEnemy() const;
    Enemy* enemyByIndex(int index) const;
    Enemy* selectedEnemy() const;
    Enemy* inspectedEnemy() const;
    QVector<int> aliveEnemyIndexes() const;
    int enemyIndexOf(Enemy* enemy) const;
    int enemyIndexFromCommand(const CodeCommandView& command) const;
    void ensureSelectedEnemyValid();
    void setSelectedEnemyIndex(int index);
    QWidget* enemyAnchorWidget(int enemyIndex) const;
    QString formatCardText(const CardView& card) const;
    QString cardKindText(const CardView& card) const;
    QString cardCodePreview(const CardView& card) const;
    QString cardCodeHtml(const CardView& card) const;
    QString cardAccentColor(const CardView& card) const;
    QString cardToolTipText(const CardView& card) const;
    QString cardButtonStyle(const QString& accent, bool playable) const;
    void updateCardButtonStyle(QPushButton* button, const CardView& card, bool playable);
    QString statusTypeText(StatusType type) const;
    QString buildStatusSummary(const std::vector<Status>& statuses) const;
    QString buildBossSpecialSkillText(Enemy* enemy) const;
    QString toQString(const std::string& s) const;
    QStringList toQStringList(const std::vector<std::string>& lines) const;
    QStringList makeTickStatusesBlock(const QStringList& bodyLines) const;
    QStringList makeTickStatusesBlock(const std::vector<std::string>& bodyLines) const;
    QSet<int> allLineNumbers(QPlainTextEdit* editor) const;
    QSet<int> changedBodyLines(const QStringList& oldBody,
                               const QStringList& newBody) const;
};

#endif // MAINWINDOW_H
