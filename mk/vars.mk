CXX    := g++
CC     := gcc
LN     := gcc
AR     := ar
RANLIB := ranlib

BASEDIR    := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../)
BUILDBASE  := $(BASEDIR)/build
BUILDDIR   := $(addprefix $(BASEDIR)/build/tmp, $(CURDIR))
LIBDIR     := $(BUILDBASE)/lib
BINDIR     := $(BUILDBASE)/bin
INCLUDES   := $(BASEDIR)/include
DEBUGFLAGS := -ggdb -O0 -D_DEBUG
MKDEFINES  := -DAPP_BASE_DIR=\"$(BASEDIR)\"
CFLAGS      = -W -Wall -Wwrite-strings -Wno-comment -fPIC -DPIC -D_REENTRANT -fno-strict-aliasing -fstack-protector-strong $(MKDEFINES) $(DEBUGFLAGS) $(addprefix -I,$(INCLUDES))
CXXFLAGS    = -W -Wall -Wwrite-strings -Wno-comment -fPIC -DPIC -D_REENTRANT -fno-strict-aliasing -fstack-protector-strong -std=c++14 $(MKDEFINES) $(DEBUGFLAGS) $(addprefix -I,$(INCLUDES))
LDFLAGS    := -L$(LIBDIR) -Wl,-rpath,$(LIBDIR)/lib
LIBS       :=
