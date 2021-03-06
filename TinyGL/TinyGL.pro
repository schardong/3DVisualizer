TARGET = tinygl
TEMPLATE = lib
CONFIG += static c++11

INCLUDEPATH += ../Kernel/src
DEPENDPATH += ../Kernel/src

DESTDIR = $$OUT_PWD/../

win32 {
    CONFIG(release, debug|release) {
      QMAKE_CXXFLAGS += /O2
    }

    CONFIG(debug, debug|release) {
      QMAKE_CXXFLAGS += /Od
    }

    INCLUDEPATH += ../../glew/include
    INCLUDEPATH += ../../glm
    INCLUDEPATH += ../../tinyxml
}

unix {
    QMAKE_CXXFLAGS += -MMD

    CONFIG(release, debug|release) {
      QMAKE_CXXFLAGS += -g0 -O2
    }

    CONFIG(debug, debug|release) {
      QMAKE_CXXFLAGS += -g3 -O0 -pg
    }
}

HEADERS += \
    src/axis.h \
    src/bufferobject.h \
    src/cube.h \
    src/framebufferobject.h \
    src/grid.h \
    src/light.h \
    src/mesh.h \
    src/quad.h \
    src/shader.h \
    src/sphere.h \
    src/tglconfig.h \
    src/tinygl.h

SOURCES += \
    src/axis.cpp \
    src/bufferobject.cpp \
    src/cube.cpp \
    src/framebufferobject.cpp \
    src/grid.cpp \
    src/light.cpp \
    src/mesh.cpp \
    src/quad.cpp \
    src/shader.cpp \
    src/sphere.cpp \
    src/tinygl.cpp
