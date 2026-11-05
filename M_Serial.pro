QT += core gui widgets serialport

CONFIG += c++17

TARGET = M_Serial
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui
