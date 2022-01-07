QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    fileloader.cpp \
    formspecprice.cpp \
    infownd.cpp \
    main.cpp \
    mainwnd.cpp \
    moddir.cpp \
    modfinfo.cpp \
    modflyers.cpp \
    modspecsumm.cpp

HEADERS += \
    fileloader.h \
    formspecprice.h \
    infownd.h \
    mainwnd.h \
    moddir.h \
    modfinfo.h \
    modflyers.h \
    modspecsumm.h

FORMS += \
    formspecprice.ui \
    infownd.ui \
    mainwnd.ui

RESOURCES     = manisend.qrc
ICON = icon/icon.icns
RC_FILE = manisend.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
