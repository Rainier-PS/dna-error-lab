CC		= gcc
CFLAGS	= -Wall -Wextra -std=c11 -I include -I third_party
LDFLAGS	= -lm

SRC		=	src/main.c \
			src/sequence.c \
			src/game.c \
			src/solver.c \
			src/challenge.c \
			src/levels.c \
			src/scoring.c \
			src/tui.c \
			src/execution.c \
			src/codon.c \
			src/mutation.c

OBJ	= $(patsubst src/%.c, build/%.o, $(SRC))
TARGET = build/dna-error-lab

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf build

run: $(TARGET)
	./$(TARGET)

dev: $(TARGET)
	./$(TARGET) --dev

info:
	@echo "DNA Error Lab"
	@echo "Headers: $(words $(wildcard include/*.h))"
	@echo "Sources: $(words $(SRC))"
	@echo "Target: $(TARGET)"
	@echo ""
	@echo "To build: make"
	@echo "To run: make run"
	@echo "To dev: make dev"
	@echo "To clean: make clean"

.PHONY: all clean run dev info
