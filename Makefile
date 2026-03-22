CC=gcc
SRC=$(wildcard ./src/*.c)
CFLAGS=-O3 -Wall -Wextra -Werror -Wno-absolute-value -Wno-sign-compare -pedantic
DFLAGS=-O0 -g -Wall -Wextra -Wno-absolute-value -Wno-sign-compare -pedantic
LFLAGS_WIN=-L lib/win/ -lm -lraylib -lopengl32 -lgdi32 -lwinmm -Wl,--subsystem,windows
LFLAGS=-L lib/linux/ -lm -lraylib -lglfw
BUILD=build
EXE=$(BUILD)/wrun
DEBUG=$(BUILD)/wrun-debug
DEFS=

all: $(BUILD) $(EXE) $(DEBUG)

$(EXE): $(SRC)
	$(CC) $(DEFS) $(CFLAGS) -o $(EXE) $(SRC) $(LFLAGS)

$(DEBUG): $(SRC)
	$(CC) $(DEFS) $(DFLAGS) -o $(DEBUG) $(SRC) $(LFLAGS)

run: $(EXE)
	./$(EXE)

debug: $(DEBUG)
	./$(DEBUG)

$(BUILD):
	mkdir -p $(BUILD)

.PHONY: clean
clean:
	rm -rf build
