QT       += core gui network multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



SOURCES += \
    contest.cpp \
    custom_controls/facedetection.cpp \
    network_modules/apiclient.cpp \
    custom_controls/avatarcrop.cpp \
    custom_controls/avatarwidget.cpp \
    custom_controls/dialog.cpp \
    home.cpp \
    login.cpp \
    main.cpp \
    register.cpp \
    rewritten_widgets/label.cpp \
    rewritten_widgets/lineedit.cpp \
    rewritten_widgets/listview.cpp \
    rewritten_widgets/textedit.cpp

HEADERS += \
    contest.h \
    custom_controls/facedetection.h \
    network_modules/apiclient.h \
    custom_controls/avatarcrop.h \
    custom_controls/avatarwidget.h \
    custom_controls/dialog.h \
    home.h \
    login.h \
    register.h \
    rewritten_widgets/label.h \
    rewritten_widgets/lineedit.h \
    rewritten_widgets/listview.h \
    rewritten_widgets/textedit.h

FORMS += \
    contest.ui \
    custom_controls/avatarcrop.ui \
    custom_controls/dialog.ui \
    custom_controls/facedetection.ui \
    home.ui \
    login.ui \
    register.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
