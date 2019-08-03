#-------------------------------------------------
#
# Project created by QtCreator 2019-07-27T22:49:20
#
#-------------------------------------------------

QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EHentaiSearch
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

win32{
	QMAKE_CXXFLAGS += /utf-8
}
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../vcpkg/installed/x86-windows/lib/ -lcryptopp-static
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../vcpkg/installed/x86-windows/debug/lib/ -lcryptopp-static

INCLUDEPATH += $$PWD/../../../../vcpkg/installed/x86-windows/include
DEPENDPATH += $$PWD/../../../../vcpkg/installed/x86-windows/include

win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../../vcpkg/installed/x86-windows/lib/cryptopp-static.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../../vcpkg/installed/x86-windows/debug/lib/cryptopp-static.lib

RESOURCES += \
	resource.qrc

DISTFILES += \
	EHentaiSearch.ini
