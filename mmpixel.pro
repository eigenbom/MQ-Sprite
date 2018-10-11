TEMPLATE = app
TARGET = MQSprite
INCLUDEPATH += . src
QT += core gui widgets
CONFIG += c++11

HEADERS += \
    src/commands.h \
    src/compositetoolswidget.h \
    src/compositewidget.h \
    src/mainwindow.h \
    src/paletteview.h \
    src/partlist.h \
    src/partwidget.h \
    src/projectmodel.h \
    src/resizemodedialog.h \
    src/assettreewidget.h \
    src/modelistwidget.h \
    src/drawingtools.h \
    src/propertieswidget.h \
    src/animationwidget.h \
    src/spritezoomwidget.h \
    src/optionswidget.h \
    src/zip.h

FORMS += \
    src/compositetoolswidget.ui \
    src/mainwindow.ui \
    src/partlist.ui \
    src/resizemodedialog.ui \
    src/drawingtools.ui \
    src/propertieswidget.ui \
    src/animationwidget.ui \
    src/spritezoomwidget.ui \
    src/optionswidget.ui

SOURCES += \
    src/commands.cpp \
    src/compositetoolswidget.cpp \
    src/compositewidget.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/paletteview.cpp \
    src/partlist.cpp \
    src/partwidget.cpp \
    src/projectmodel.cpp \
    src/resizemodedialog.cpp \
    src/assettreewidget.cpp \
    src/modelistwidget.cpp \
    src/drawingtools.cpp \
    src/propertieswidget.cpp \
    src/animationwidget.cpp \
    src/spritezoomwidget.cpp \
    src/optionswidget.cpp \
    src/zip.cpp

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    README.md
