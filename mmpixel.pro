TEMPLATE = app
TARGET = SpriteEditor
INCLUDEPATH += . src
QT += core gui widgets

HEADERS += \
    src/assetlistwidget.h \
    src/commands.h \
    src/compositetoolswidget.h \
    src/compositewidget.h \
    src/mainwindow.h \
    src/paletteview.h \
    src/partlist.h \
    src/parttoolswidget.h \
    src/partwidget.h \
    src/projectmodel.h \
    src/resizemodedialog.h \
    src/tarball.h \
    src/assettreewidget.h \
    src/animatorwidget.h \
    src/modelistwidget.h

FORMS += \
    src/compositetoolswidget.ui \
    src/mainwindow.ui \
    src/partlist.ui \
    src/parttoolswidget.ui \
    src/resizemodedialog.ui \
    src/animatorwidget.ui

SOURCES += \
    src/assetlistwidget.cpp \
    src/commands.cpp \
    src/compositetoolswidget.cpp \
    src/compositewidget.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/paletteview.cpp \
    src/partlist.cpp \
    src/parttoolswidget.cpp \
    src/partwidget.cpp \
    src/projectmodel.cpp \
    src/resizemodedialog.cpp \
    src/tarball.cpp \
    src/assettreewidget.cpp \
    src/animatorwidget.cpp \
    src/modelistwidget.cpp

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    README.md
