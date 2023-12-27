QT       += core gui network
QT       += axcontainer
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    can_protocol.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    can_protocol.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    Battery_Simulator_Tool_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

include(drivers/drivers.pri)
include(savelog/savelog.pri)
include(help/help.pri)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -lECanVci64
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -lECanVci64d

INCLUDEPATH += $$PWD/../lib
DEPENDPATH += $$PWD/../lib
