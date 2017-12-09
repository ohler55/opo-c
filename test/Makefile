
CC=cc
CV=$(shell if [ `uname` = "Darwin" ]; then echo "c11"; elif [ `uname` = "Linux" ]; then echo "gnu11"; fi;)
OS=$(shell echo `uname`)
ifeq ($(build),release)
	CFLAGS=-c -Wall -O3 -std=$(CV) -pedantic -D$(OS)
else
	CFLAGS=-c -Wall -g -Og -std=$(CV) -pedantic -D$(OS)
endif

SRC_DIR=.
LIB_DIR=../lib
INC_DIR=../include
SRCS=$(shell find $(SRC_DIR) -type f -name "*.c" -print)
OBJS=$(SRCS:.c=.o)

LIB_DIRS=-L../../ojc/lib -L../../oak/lib
INC_DIRS=-I../../ojc/include -I../../oak/include
#INC_DIRS=-I../../ojc/include
#LIB_DIRS=-L../../ojc/lib
LIBS=-lopoc -lojc -loak -lm -lpthread
TARGET=run_tests

all: $(TARGET)

clean:
	$(RM) *.o
	$(RM) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) -L$(LIB_DIR) $(LIB_DIRS) $(LIBS)

%.o : %.c
	$(CC) -I. $(INC_DIRS) -I$(INC_DIR) $(CFLAGS) -o $@ $<

test: $(TARGET)
	$(TARGET)
