TEMPLATE = app
TARGET = graphicsview
CONFIG += qt warn_on
!package: CONFIG += qt3d

VPATH += $$PWD/../../src/threed/geometry
SOURCES = \
    cubeitem.cpp \
    modelitem.cpp \
    teapotitem.cpp \
    main.cpp \
    edge.cpp \
    node.cpp \
    graph.cpp \
    scene.cpp \
    view.cpp
HEADERS = \
    cubeitem.h \
    modelitem.h \
    teapotitem.h \
    edge.h \
    node.h \
    graph.h \
    scene.h \
    view.h
RESOURCES = \
    graphicsview.qrc

package {
    LIBS += -L../../../src/threed -lQt3D
    INCLUDEPATH += ../../../include
    target.path += $$[QT_INSTALL_BINS]
    INSTALLS += target
} else {
    DESTDIR = ../../../bin
}
