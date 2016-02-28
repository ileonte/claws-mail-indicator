include ../mk/vars.mk

TARGET := indicator.so
SRCS   := $(wildcard *.c)
CFLAGS += $(shell pkg-config --cflags claws-mail) $(shell pkg-config --cflags appindicator-0.1) $(shell pkg-config --cflags gtk+-2.0)
LIBS   += $(shell pkg-config --libs claws-mail) $(shell pkg-config --libs appindicator-0.1) $(shell pkg-config --libs gtk+-2.0)

DEBUGFLAGS := -ggdb -O2 -DNDEBUG

include ../mk/dynamiclib.mk
