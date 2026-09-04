CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I../include
SRC = $(filter-out ../src/main.c,$(wildcard ../src/*.c))

perft_test: perft_test.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

draw_test: draw_test.c $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: perft_test
	./perft_test

test-draw: draw_test
	./draw_test

clean:
	rm -f perft_test draw_test

.PHONY: test test-draw clean
