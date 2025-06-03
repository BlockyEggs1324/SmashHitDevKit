QT       += core gui 3dcore 3drender 3dinput 3dextras openglwidgets opengl xml

LIBS += -lopengl32 -lglu32 -lz

INCLUDEPATH += "C:\GLM\glm-1.0.1-light" # Change this to your GLM directory if needed

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    LevelLoader.cpp \
    MainWindow.cpp \
    MyOpenGLWidget.cpp \
    PreferencesDialog.cpp \
    RoomLoader.cpp \
    SegmentLoader.cpp \
    SegmentWidget.cpp \
    Views2D.cpp \
    main.cpp

HEADERS += \
    LevelLoader.h \
    MainWindow.h \
    MyOpenGLWidget.h \
    PreferencesDialog.h \
    Rect3D.h \
    RoomLoader.h \
    SegmentLoader.h \
    SegmentWidget.h \
    TextureExtractor.h \
    TextureLoader.h \
    Views2D.h

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES +=
