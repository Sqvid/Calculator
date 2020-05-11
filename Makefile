CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99
LFLAGS = -lm
DBGFLAGS = -g
EXE = calculator

all: $(EXE)

debug: CFLAGS += $(DBGFLAGS)
debug: $(EXE)

calculator: calculator.c
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)


.PHONY: clean

clean:
	rm calculator
