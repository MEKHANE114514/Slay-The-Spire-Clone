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
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(900, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        titleLabel = new QLabel(centralwidget);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setGeometry(QRect(320, 20, 121, 41));
        QFont font;
        font.setPointSize(18);
        font.setBold(true);
        titleLabel->setFont(font);
        playerHpLabel = new QLabel(centralwidget);
        playerHpLabel->setObjectName("playerHpLabel");
        playerHpLabel->setGeometry(QRect(60, 90, 111, 20));
        playerEnergyLabel = new QLabel(centralwidget);
        playerEnergyLabel->setObjectName("playerEnergyLabel");
        playerEnergyLabel->setGeometry(QRect(60, 120, 101, 20));
        playerShieldLabel = new QLabel(centralwidget);
        playerShieldLabel->setObjectName("playerShieldLabel");
        playerShieldLabel->setGeometry(QRect(60, 140, 111, 20));
        enemyHpLabel = new QLabel(centralwidget);
        enemyHpLabel->setObjectName("enemyHpLabel");
        enemyHpLabel->setGeometry(QRect(590, 90, 101, 21));
        cardButton1 = new QPushButton(centralwidget);
        cardButton1->setObjectName("cardButton1");
        cardButton1->setGeometry(QRect(130, 370, 81, 101));
        cardButton2 = new QPushButton(centralwidget);
        cardButton2->setObjectName("cardButton2");
        cardButton2->setGeometry(QRect(240, 370, 81, 101));
        cardButton3 = new QPushButton(centralwidget);
        cardButton3->setObjectName("cardButton3");
        cardButton3->setGeometry(QRect(340, 370, 81, 101));
        cardButton4 = new QPushButton(centralwidget);
        cardButton4->setObjectName("cardButton4");
        cardButton4->setGeometry(QRect(450, 370, 81, 101));
        cardButton5 = new QPushButton(centralwidget);
        cardButton5->setObjectName("cardButton5");
        cardButton5->setGeometry(QRect(550, 370, 71, 101));
        logTextEdit = new QTextEdit(centralwidget);
        logTextEdit->setObjectName("logTextEdit");
        logTextEdit->setGeometry(QRect(750, 60, 131, 491));
        logTextEdit->setReadOnly(true);
        endTurnButton = new QPushButton(centralwidget);
        endTurnButton->setObjectName("endTurnButton");
        endTurnButton->setGeometry(QRect(660, 10, 61, 31));
        restartButton = new QPushButton(centralwidget);
        restartButton->setObjectName("restartButton");
        restartButton->setGeometry(QRect(740, 10, 61, 31));
        helpButton = new QPushButton(centralwidget);
        helpButton->setObjectName("helpButton");
        helpButton->setGeometry(QRect(820, 10, 61, 31));
        drawPileLabel = new QLabel(centralwidget);
        drawPileLabel->setObjectName("drawPileLabel");
        drawPileLabel->setGeometry(QRect(40, 470, 51, 71));
        discardPileLabel = new QLabel(centralwidget);
        discardPileLabel->setObjectName("discardPileLabel");
        discardPileLabel->setGeometry(QRect(680, 480, 51, 71));
        codePlainTextEdit = new QPlainTextEdit(centralwidget);
        codePlainTextEdit->setObjectName("codePlainTextEdit");
        codePlainTextEdit->setGeometry(QRect(260, 80, 221, 271));
        codePlainTextEdit->setReadOnly(true);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 900, 18));
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
        titleLabel->setText(QCoreApplication::translate("MainWindow", "CodeCraft", nullptr));
        playerHpLabel->setText(QCoreApplication::translate("MainWindow", "\347\216\251\345\256\266\347\224\237\345\221\275\357\274\23250/50", nullptr));
        playerEnergyLabel->setText(QCoreApplication::translate("MainWindow", "\347\216\251\345\256\266\350\203\275\351\207\217\357\274\2323", nullptr));
        playerShieldLabel->setText(QCoreApplication::translate("MainWindow", "\347\216\251\345\256\266\346\212\244\347\224\262\357\274\2320", nullptr));
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
        drawPileLabel->setText(QCoreApplication::translate("MainWindow", "\346\212\275\347\211\214\345\240\206\n"
"10", nullptr));
        discardPileLabel->setText(QCoreApplication::translate("MainWindow", "\345\274\203\347\211\214\345\240\206\n"
"0", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
