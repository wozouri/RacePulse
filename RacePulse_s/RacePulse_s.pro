QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


# opencv
INCLUDEPATH += D:/Lib/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/include
LIBS += -LD:/Lib/OpenCV-MinGW-Build-OpenCV-4.5.5-x64/x64/mingw/lib
LIBS += -lopencv_core455 -lopencv_imgproc455 -lopencv_imgcodecs455 -lopencv_highgui455 -lopencv_objdetect455 -lopencv_dnn455


SOURCES += \
    clienthandler.cpp \
    connectionpool.cpp \
    main.cpp \
    server.cpp

HEADERS += \
    clienthandler.h \
    connectionpool.h \
    server.h

FORMS += \
    server.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

DISTFILES +=
