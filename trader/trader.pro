TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -luv -lmongoc-1.0 -lbson-1.0

INCLUDEPATH += /usr/include/libmongoc-1.0
INCLUDEPATH += /usr/include/libbson-1.0

SOURCES += main.c \
    utility.c \
    mongo.c \
    ctp-api.cpp

HEADERS += \
    trader.h \
    utility.h \
    mongo.h \
    ctp-api.h
