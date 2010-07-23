TEMPLATE = lib
VERSION = 1.0.0
TARGET = qicnsicon
CONFIG += qt plugin debug_and_release

HEADERS += qicnsiconengine.h
SOURCES += main.cpp qicnsiconengine.cpp

CONFIG(universal) {
  CONFIG += x86 ppc
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
  QMAKE_CFLAGS += -mmacosx-version-min=10.4 -Xarch_i386 -mmmx -Xarch_i386 -msse -Xarch_i386 -msse2
  QMAKE_CXXFLAGS += -mmacosx-version-min=10.4 -Xarch_i386 -mmmx -Xarch_i386 -msse -Xarch_i386 -msse2
}
