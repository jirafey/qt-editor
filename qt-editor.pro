QT += widgets gui core

TARGET = qt-editor
TEMPLATE = app

CONFIG += c++17

SOURCES += \
    editorkeyhandler.cpp \
    editorselection.cpp \
    editortabmanager.cpp \
    editorview.cpp \
    fileoperations.cpp \
    fuzzysearcher.cpp \
    incrementaction.cpp \
    linuxcommandrunner.cpp \
    linuxconsole.cpp \
    macromanager.cpp \
    macroplayerdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    mmaphandler.cpp \
    piece.cpp \
    piecetable.cpp \
    searchreplace.cpp \
    settingsdialog.cpp \
    settingsmanager.cpp \
    shortcutmanager.cpp \
    syntaxhighlighter.cpp

HEADERS += \
    editorcursor.h \
    editorkeyhandler.h \
    editorselection.h \
    editortabmanager.h \
    editorview.h \
    fileoperations.h \
    fuzzysearcher.h \
    incrementaction.h \
    linuxcommandrunner.h \
    linuxconsole.h \
    macromanager.h \
    macroplayerdialog.h \
    mainwindow.h \
    mmaphandler.h \
    piece.h \
    piecetable.h \
    searchreplace.h \
    settingsdialog.h \
    settingsmanager.h \
    shortcutmanager.h \
    syntaxhighlighter.h

FORMS += \
    mainwindow.ui

DISTFILES += \
    syntax_words.inc \
    README.md

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
