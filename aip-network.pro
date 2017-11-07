QT += core
QT -= gui

TARGET = aip-network
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

HEADERS += \
    app/CTcpNetwork.h

SOURCES += \
    app/aip-network.cpp \
    app/CTcpNetwork.cpp


