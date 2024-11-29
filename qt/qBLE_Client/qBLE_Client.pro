#---------------------------------------------------------------------------------------
# 2024-11-29  Bluetooth client (status machine) for arBLE_Preipheral (arduino server)
#---------------------------------------------------------------------------------------

QT       += core gui bluetooth widgets
CONFIG   += c++11

TARGET   = qBLE_Client
TEMPLATE = app

SOURCES += main.cpp  mainwindow.cpp bleinterface.cpp
HEADERS  += mainwindow.h bleinterface.h qqmlhelpers.h
FORMS    += mainwindow.ui


