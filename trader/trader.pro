TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -luv

SOURCES += main.c

HEADERS += \
    trader.h
