#-------------------------------------------------
#
# Project created by QtCreator 2016-08-14T15:28:53
#
#-------------------------------------------------

QT       += core gui multimedia testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DemoStereoMix
TEMPLATE = app

# WINDOWSSDKDIR = C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0
INCLUDEPATH += "$$WINDOWSSDKDIR/um/x64"
LIBS += Ole32.lib Winmm.lib Shlwapi.lib propsys.lib


SOURCES += main.cpp\
        widget.cpp

HEADERS  += widget.h \
    PolicyConfig.h

FORMS    += widget.ui
