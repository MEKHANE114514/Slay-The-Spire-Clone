QT += widgets

CONFIG += c++17

INCLUDEPATH += . ../game

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    ../game/game_manager.cpp \
    ../game/battle.cpp \
    ../game/cards.cpp \
    ../game/enemy.cpp \
    ../game/minion.cpp \
    ../game/player.cpp

HEADERS += \
    mainwindow.h \
    cards.h \
    game_manager.h \
    battle.h \
    enemy.h \
    game_text.h \
    minion.h \
    player.h \
    types.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    CodeCraft_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
