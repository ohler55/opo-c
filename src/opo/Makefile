CC=cc
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
OS=$(shell echo `uname`)
ifeq ($(build),release)
	CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic -D$(OS)
else
	CFLAGS=-c -Wall -g -Og -std=$(CV) -pedantic -D$(OS)
endif

# directories for this library and headers
SRC_DIR=.
LIB_DIR=../../lib
INC_DIR=../../include/opo
SRCS=$(wildcard *.c)
HEADERS=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)

PUB_HEADERS=opo.h err.h msg.h val.h builder.h
TARGET=$(LIB_DIR)/libopo.a

# external
LIB_DIRS=-L../../../ojc/lib
INC_DIRS=-I../../../ojc/include
LIBS=-lojc -lm -lpthread

all: $(LIB_DIR) $(INC_DIR) $(TARGET)
	rsync -tupRE $(PUB_HEADERS) $(INC_DIR)

clean:
	$(RM) $(OBJS)
	$(RM) *~
	$(RM) .#*
	$(RM) $(TARGET)
	$(RM) $(INC_DIR)/*
	$(RM) $(LIB_DIR)/*

$(INC_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

$(TARGET): $(OBJS)
	ar -rcs $@ $(OBJS)

%.o : %.c  $(HEADERS)
	$(CC) -I. $(INC_DIRS) $(CFLAGS) -o $@ $<
