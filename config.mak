PREFIX = /usr/
prefix = $(DESTDIR)$(PREFIX)

#CFLAGS += -DDEBUG

CFLAGS += -Os -fomit-frame-pointer -g -Wall

CFLAGS += -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC

CC = cc
RANLIB  = ranlib
AR = ar
