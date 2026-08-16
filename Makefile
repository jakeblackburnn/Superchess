# SuperChess C build.
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -Iinclude
BUILD   := build
BIN     := superchess

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) $(BIN)

-include $(DEPS)
