# SuperChess C build.
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -Iinclude -fPIC
BUILD   := build
BIN     := superchess

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LIB         := $(BUILD)/libsuperchess.dylib
  LIB_LDFLAGS := -dynamiclib
else
  LIB         := $(BUILD)/libsuperchess.so
  LIB_LDFLAGS := -shared
endif

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

LIB_SRCS := $(filter-out src/main.c,$(SRCS))
LIB_OBJS := $(LIB_SRCS:src/%.c=$(BUILD)/%.o)

.PHONY: all lib clean

all: $(BIN)

lib: $(LIB)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(LIB): $(LIB_OBJS)
	$(CC) $(LIB_LDFLAGS) -o $@ $(LIB_OBJS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) $(BIN) $(LIB)

-include $(DEPS)
