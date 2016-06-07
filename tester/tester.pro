TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../trader

LIBS += -luv -lgtest -lgtest_main

SOURCES += \
    unit-test-1.cpp \
    ../trader/test-poll.c \
    ../trader/utility.c

HEADERS += \
    ../trader/trader.h \
    ../trader/utility.h
