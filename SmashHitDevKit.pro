QT       += core gui 3dcore 3drender 3dinput 3dextras openglwidgets opengl xml

LIBS += -lopengl32 -lglu32

INCLUDEPATH += "C:\Users\samue\Downloads\glm-1.0.1-light"

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    LevelLoader.cpp \
    PreferencesDialog.cpp \
    RoomLoader.cpp \
    SegmentLoader.cpp \
    SegmentWidget.cpp \
    Views2D.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    LevelLoader.h \
    PreferencesDialog.h \
    Rect3D.h \
    RoomLoader.h \
    SegmentLoader.h \
    SegmentWidget.h \
    Views2D.h \
    mainwindow.h

FORMS += \
    #mainwindow.ui

TRANSLATIONS +=

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES +=
