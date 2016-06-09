TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../trader

LIBS += -luv
LIBS += -lgtest -lgtest_main
LIBS += -lmongoc-1.0 -lbson-1.0

INCLUDEPATH += /usr/include/libmongoc-1.0
INCLUDEPATH += /usr/include/libbson-1.0

SOURCES += \
    unit-test-1.cpp \
    ../trader/test-poll.c \
    ../trader/utility.c \
    ../trader/mongo.c

HEADERS += \
    ../trader/trader.h \
    ../trader/utility.h \
    ../trader/mongo.h
