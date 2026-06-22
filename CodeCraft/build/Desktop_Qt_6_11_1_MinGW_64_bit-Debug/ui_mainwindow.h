/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QLabel *battleBackgroundLabel;
    QLabel *titleLabel;
    QLabel *playerHpLabel;
    QLabel *playerEnergyLabel;
    QLabel *playerShieldLabel;
    QLabel *enemyHpLabel;
    QPushButton *cardButton1;
    QPushButton *cardButton2;
    QPushButton *cardButton3;
    QPushButton *cardButton4;
    QPushButton *cardButton5;
    QTextEdit *logTextEdit;
    QPushButton *endTurnButton;
    QPushButton *restartButton;
    QPushButton *helpButton;
    QLabel *drawPileLabel;
    QLabel *discardPileLabel;
    QPlainTextEdit *codePlainTextEdit;
    QLabel *bossSkillLabel;
    QLabel *minionHpLabel1;
    QLabel *minionHpLabel2;
    QLabel *playerStrengthLabel;
    QPlainTextEdit *playerTickCodePlainTextEdit;
    QPlainTextEdit *enemyTickCodePlainTextEdit;
    QLabel *enemyIntentLabel;
    QLabel *playerImageLabel;
    QLabel *enemyImageLabel;
    QLabel *minionImageLabel1;
    QLabel *minionImageLabel2;
    QLabel *drawPileIconLabel;
    QLabel *drawPileCountLabel;
    QLabel *discardPileIconLabel;
    QLabel *discardPileCountLabel;
    QLabel *energyIconLabel;
    QLabel *energyValueLabel;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1500, 1000);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        battleBackgroundLabel = new QLabel(centralwidget);
        battleBackgroundLabel->setObjectName("battleBackgroundLabel");
        battleBackgroundLabel->setGeometry(QRect(0, 0, 1500, 980));
        battleBackgroundLabel->setScaledContents(true);
        titleLabel = new QLabel(centralwidget);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setGeometry(QRect(640, 0, 220, 42));
        QFont font;
        font.setFamilies({QString::fromUtf8("Microsoft YaHei UI")});
        font.setPointSize(20);
        font.setBold(true);
        titleLabel->setFont(font);
        playerHpLabel = new QLabel(centralwidget);
        playerHpLabel->setObjectName("playerHpLabel");
        playerHpLabel->setGeometry(QRect(24, 34, 230, 24));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Microsoft YaHei UI")});
        font1.setPointSize(10);
        font1.setBold(false);
        playerHpLabel->setFont(font1);
        playerHpLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        playerHpLabel->setWordWrap(true);
        playerEnergyLabel = new QLabel(centralwidget);
        playerEnergyLabel->setObjectName("playerEnergyLabel");
        playerEnergyLabel->setGeometry(QRect(24, 62, 190, 24));
        playerEnergyLabel->setFont(font1);
        playerEnergyLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        playerEnergyLabel->setWordWrap(true);
        playerShieldLabel = new QLabel(centralwidget);
        playerShieldLabel->setObjectName("playerShieldLabel");
        playerShieldLabel->setGeometry(QRect(24, 90, 190, 24));
        playerShieldLabel->setFont(font1);
        playerShieldLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        playerShieldLabel->setWordWrap(true);
        enemyHpLabel = new QLabel(centralwidget);
        enemyHpLabel->setObjectName("enemyHpLabel");
        enemyHpLabel->setGeometry(QRect(1150, 34, 220, 24));
        enemyHpLabel->setFont(font1);
        enemyHpLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        enemyHpLabel->setWordWrap(true);
        cardButton1 = new QPushButton(centralwidget);
        cardButton1->setObjectName("cardButton1");
        cardButton1->setGeometry(QRect(400, 835, 115, 130));
        cardButton2 = new QPushButton(centralwidget);
        cardButton2->setObjectName("cardButton2");
        cardButton2->setGeometry(QRect(530, 835, 115, 130));
        cardButton3 = new QPushButton(centralwidget);
        cardButton3->setObjectName("cardButton3");
        cardButton3->setGeometry(QRect(660, 835, 115, 130));
        cardButton4 = new QPushButton(centralwidget);
        cardButton4->setObjectName("cardButton4");
        cardButton4->setGeometry(QRect(790, 835, 115, 130));
        cardButton5 = new QPushButton(centralwidget);
        cardButton5->setObjectName("cardButton5");
        cardButton5->setGeometry(QRect(920, 835, 115, 130));
        logTextEdit = new QTextEdit(centralwidget);
        logTextEdit->setObjectName("logTextEdit");
        logTextEdit->setReadOnly(true);
        logTextEdit->setGeometry(QRect(1160, 615, 300, 210));
        endTurnButton = new QPushButton(centralwidget);
        endTurnButton->setObjectName("endTurnButton");
        endTurnButton->setGeometry(QRect(1160, 850, 95, 36));
        restartButton = new QPushButton(centralwidget);
        restartButton->setObjectName("restartButton");
        restartButton->setGeometry(QRect(1268, 850, 95, 36));
        helpButton = new QPushButton(centralwidget);
        helpButton->setObjectName("helpButton");
        helpButton->setGeometry(QRect(1376, 850, 95, 36));
        drawPileLabel = new QLabel(centralwidget);
        drawPileLabel->setObjectName("drawPileLabel");
        drawPileLabel->setGeometry(QRect(20, 943, 122, 34));
        drawPileLabel->setFont(font1);
        drawPileLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        drawPileLabel->setWordWrap(true);
        discardPileLabel = new QLabel(centralwidget);
        discardPileLabel->setObjectName("discardPileLabel");
        discardPileLabel->setGeometry(QRect(160, 943, 122, 34));
        discardPileLabel->setFont(font1);
        discardPileLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        discardPileLabel->setWordWrap(true);
        codePlainTextEdit = new QPlainTextEdit(centralwidget);
        codePlainTextEdit->setObjectName("codePlainTextEdit");
        codePlainTextEdit->setGeometry(QRect(380, 40, 700, 290));
        codePlainTextEdit->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
        codePlainTextEdit->setReadOnly(true);
        bossSkillLabel = new QLabel(centralwidget);
        bossSkillLabel->setObjectName("bossSkillLabel");
        bossSkillLabel->setGeometry(QRect(1380, 34, 95, 56));
        bossSkillLabel->setFont(font1);
        bossSkillLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        bossSkillLabel->setWordWrap(true);
        minionHpLabel1 = new QLabel(centralwidget);
        minionHpLabel1->setObjectName("minionHpLabel1");
        minionHpLabel1->setGeometry(QRect(25, 690, 140, 66));
        minionHpLabel1->setFont(font1);
        minionHpLabel1->setAlignment(Qt::AlignmentFlag::AlignCenter);
        minionHpLabel1->setWordWrap(true);
        minionHpLabel2 = new QLabel(centralwidget);
        minionHpLabel2->setObjectName("minionHpLabel2");
        minionHpLabel2->setGeometry(QRect(185, 690, 140, 66));
        minionHpLabel2->setFont(font1);
        minionHpLabel2->setAlignment(Qt::AlignmentFlag::AlignCenter);
        minionHpLabel2->setWordWrap(true);
        playerStrengthLabel = new QLabel(centralwidget);
        playerStrengthLabel->setObjectName("playerStrengthLabel");
        playerStrengthLabel->setGeometry(QRect(24, 118, 270, 48));
        playerStrengthLabel->setFont(font1);
        playerStrengthLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        playerStrengthLabel->setWordWrap(true);
        playerTickCodePlainTextEdit = new QPlainTextEdit(centralwidget);
        playerTickCodePlainTextEdit->setObjectName("playerTickCodePlainTextEdit");
        playerTickCodePlainTextEdit->setGeometry(QRect(24, 190, 320, 235));
        playerTickCodePlainTextEdit->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
        playerTickCodePlainTextEdit->setReadOnly(true);
        enemyTickCodePlainTextEdit = new QPlainTextEdit(centralwidget);
        enemyTickCodePlainTextEdit->setObjectName("enemyTickCodePlainTextEdit");
        enemyTickCodePlainTextEdit->setGeometry(QRect(1150, 190, 320, 235));
        enemyTickCodePlainTextEdit->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
        enemyTickCodePlainTextEdit->setReadOnly(true);
        enemyIntentLabel = new QLabel(centralwidget);
        enemyIntentLabel->setObjectName("enemyIntentLabel");
        enemyIntentLabel->setGeometry(QRect(1150, 66, 220, 24));
        enemyIntentLabel->setFont(font1);
        enemyIntentLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        enemyIntentLabel->setWordWrap(true);
        playerImageLabel = new QLabel(centralwidget);
        playerImageLabel->setObjectName("playerImageLabel");
        playerImageLabel->setGeometry(QRect(365, 430, 245, 365));
        playerImageLabel->setScaledContents(false);
        playerImageLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        enemyImageLabel = new QLabel(centralwidget);
        enemyImageLabel->setObjectName("enemyImageLabel");
        enemyImageLabel->setGeometry(QRect(790, 395, 315, 390));
        enemyImageLabel->setScaledContents(false);
        enemyImageLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        minionImageLabel1 = new QLabel(centralwidget);
        minionImageLabel1->setObjectName("minionImageLabel1");
        minionImageLabel1->setGeometry(QRect(45, 560, 105, 115));
        minionImageLabel1->setScaledContents(false);
        minionImageLabel1->setAlignment(Qt::AlignmentFlag::AlignCenter);
        minionImageLabel2 = new QLabel(centralwidget);
        minionImageLabel2->setObjectName("minionImageLabel2");
        minionImageLabel2->setGeometry(QRect(205, 560, 105, 115));
        minionImageLabel2->setScaledContents(false);
        minionImageLabel2->setAlignment(Qt::AlignmentFlag::AlignCenter);
        drawPileIconLabel = new QLabel(centralwidget);
        drawPileIconLabel->setObjectName("drawPileIconLabel");
        drawPileIconLabel->setGeometry(QRect(35, 805, 100, 112));
        drawPileIconLabel->setScaledContents(false);
        drawPileIconLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        drawPileCountLabel = new QLabel(centralwidget);
        drawPileCountLabel->setObjectName("drawPileCountLabel");
        drawPileCountLabel->setGeometry(QRect(92, 910, 42, 28));
        drawPileCountLabel->setScaledContents(false);
        drawPileCountLabel->setFont(font1);
        drawPileCountLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        drawPileCountLabel->setWordWrap(true);
        discardPileIconLabel = new QLabel(centralwidget);
        discardPileIconLabel->setObjectName("discardPileIconLabel");
        discardPileIconLabel->setGeometry(QRect(175, 805, 100, 112));
        discardPileIconLabel->setScaledContents(false);
        discardPileIconLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        discardPileCountLabel = new QLabel(centralwidget);
        discardPileCountLabel->setObjectName("discardPileCountLabel");
        discardPileCountLabel->setGeometry(QRect(232, 910, 42, 28));
        discardPileCountLabel->setScaledContents(false);
        discardPileCountLabel->setFont(font1);
        discardPileCountLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        discardPileCountLabel->setWordWrap(true);
        energyIconLabel = new QLabel(centralwidget);
        energyIconLabel->setObjectName("energyIconLabel");
        energyIconLabel->setGeometry(QRect(220, 56, 56, 56));
        energyIconLabel->setScaledContents(false);
        energyIconLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        energyValueLabel = new QLabel(centralwidget);
        energyValueLabel->setObjectName("energyValueLabel");
        energyValueLabel->setGeometry(QRect(275, 70, 62, 30));
        energyValueLabel->setScaledContents(false);
        energyValueLabel->setFont(font1);
        energyValueLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);
        energyValueLabel->setWordWrap(true);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1500, 18));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "CodeCraft", nullptr));
        battleBackgroundLabel->setText(QString());
        titleLabel->setText(QCoreApplication::translate("MainWindow", "CodeCraft", nullptr));
        playerHpLabel->setText(QCoreApplication::translate("MainWindow", "\347\216\251\345\256\266\347\224\237\345\221\275\357\274\23250/50", nullptr));
        playerEnergyLabel->setText(QCoreApplication::translate("MainWindow", "\347\216\251\345\256\266\350\203\275\351\207\217\357\274\2323/3", nullptr));
        playerShieldLabel->setText(QCoreApplication::translate("MainWindow", "\347\216\251\345\256\266\346\212\244\347\233\276\357\274\2320", nullptr));
        enemyHpLabel->setText(QCoreApplication::translate("MainWindow", "\346\225\214\344\272\272\347\224\237\345\221\275\357\274\23260/60", nullptr));
        cardButton1->setText(QCoreApplication::translate("MainWindow", "\346\231\256\351\200\232\346\224\273\345\207\273\n"
"\350\264\271\347\224\250\357\274\2321", nullptr));
        cardButton2->setText(QCoreApplication::translate("MainWindow", "\351\230\262\345\276\241\n"
"\350\264\271\347\224\250\357\274\2321", nullptr));
        cardButton3->setText(QCoreApplication::translate("MainWindow", "\345\205\250\345\212\233\344\270\200\345\207\273\n"
"\350\264\271\347\224\250\357\274\2322", nullptr));
        cardButton4->setText(QCoreApplication::translate("MainWindow", "\346\224\273\345\207\273\345\207\275\346\225\260\302\267\345\274\272\345\214\226\n"
"\350\264\271\347\224\250\357\274\2321", nullptr));
        cardButton5->setText(QCoreApplication::translate("MainWindow", "\344\270\211\350\277\236\345\207\273\n"
"\350\264\271\347\224\250\357\274\2322", nullptr));
        logTextEdit->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Microsoft YaHei UI'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">\347\216\251\345\256\266\345\233\236\345\220\210\345\274\200\345\247\213\343\200\202<br />\347\255\211\345\276\205\347\216\251\345\256\266\345\207\272\347\211\214\342\200\246\342\200\246</p></body></html>", nullptr));
        endTurnButton->setText(QCoreApplication::translate("MainWindow", "\347\273\223\346\235\237\345\233\236\345\220\210", nullptr));
        restartButton->setText(QCoreApplication::translate("MainWindow", "\351\207\215\346\226\260\345\274\200\345\247\213", nullptr));
        helpButton->setText(QCoreApplication::translate("MainWindow", "\346\270\270\346\210\217\350\257\264\346\230\216", nullptr));
        drawPileLabel->setText(QCoreApplication::translate("MainWindow", "\346\212\275\347\211\214\345\240\206", nullptr));
        discardPileLabel->setText(QCoreApplication::translate("MainWindow", "\345\274\203\347\211\214\345\240\206", nullptr));
        bossSkillLabel->setText(QCoreApplication::translate("MainWindow", "Boss\n"
"\346\212\200\350\203\275", nullptr));
        minionHpLabel1->setText(QCoreApplication::translate("MainWindow", "\344\273\206\344\273\2161", nullptr));
        minionHpLabel2->setText(QCoreApplication::translate("MainWindow", "\344\273\206\344\273\2162", nullptr));
        playerStrengthLabel->setText(QCoreApplication::translate("MainWindow", "\347\216\251\345\256\266\345\212\233\351\207\217\357\274\2320\n"
"\345\256\236\351\231\205\346\224\273\345\207\273\357\274\2326", nullptr));
        enemyIntentLabel->setText(QCoreApplication::translate("MainWindow", "\346\225\214\344\272\272\346\204\217\345\233\276\357\274\232", nullptr));
        playerImageLabel->setText(QString());
        enemyImageLabel->setText(QString());
        minionImageLabel1->setText(QString());
        minionImageLabel2->setText(QString());
        drawPileIconLabel->setText(QString());
        drawPileCountLabel->setText(QCoreApplication::translate("MainWindow", "0", nullptr));
        drawPileCountLabel->setStyleSheet(QCoreApplication::translate("MainWindow", "background-color: rgba(0,0,0,170); color: #7DF9FF; border: 1px solid #40D8FF; border-radius: 8px; font-weight: bold;", nullptr));
        discardPileIconLabel->setText(QString());
        discardPileCountLabel->setText(QCoreApplication::translate("MainWindow", "0", nullptr));
        discardPileCountLabel->setStyleSheet(QCoreApplication::translate("MainWindow", "background-color: rgba(0,0,0,170); color: #FFB34D; border: 1px solid #FF9A1F; border-radius: 8px; font-weight: bold;", nullptr));
        energyIconLabel->setText(QString());
        energyValueLabel->setText(QCoreApplication::translate("MainWindow", "3/3", nullptr));
        energyValueLabel->setStyleSheet(QCoreApplication::translate("MainWindow", "background-color: rgba(0,0,0,155); color: #7DF9FF; border: 1px solid #40D8FF; border-radius: 8px; font-weight: bold;", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
