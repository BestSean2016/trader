TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -luv

debug {
    DEFINES += _DDEBUG
    DEFINES +=
}

SOURCES += main.c \
    test-poll.c \
    utility.c

HEADERS += \
    trader.h \
    utility.h
