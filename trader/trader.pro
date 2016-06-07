TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -luv

SOURCES += main.c \
    utility.c

HEADERS += \
    trader.h \
    utility.h
