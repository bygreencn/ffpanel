#-------------------------------------------------
#
# Project created by QtCreator 2015-07-31T22:53:02
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffpanel
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    playpathdialog.cpp \
    tools.cpp

HEADERS  += mainwindow.h \
    playpathdialog.h \
    tools.h

FORMS    += mainwindow.ui \
    playpathdialog.ui

RC_FILE += ffpanel.rc

RESOURCES += \
    ffpanel.qrc

DISTFILES +=
