CC=gcc
SRC=$(wildcard ./src/*.c)
HEADERS=$(wildcard ./src/*.h)
CFLAGS=-O3 -Wall -Wextra -Werror -Wno-absolute-value -Wno-sign-compare -pedantic
DFLAGS=-O0 -g -Wall -Wextra -Wno-absolute-value -Wno-sign-compare -pedantic
LFLAGS_LNX=-L lib/linux/ -lm -lraylib -lglfw
LFLAGS_WIN=-static -L lib/win/ -lm -lraylib -lopengl32 -lgdi32 -lwinmm -lwinpthread -Wl,--subsystem,windows
LFLAGS=
BUILD=build
EXE=$(BUILD)/wrun
DEBUG=$(BUILD)/wrun-debug
DEFS=

ifeq ($(OS),Windows_NT)
	LFLAGS+=$(LFLAGS_WIN)
else
	LFLAGS+=$(LFLAGS_LNX)
endif

all: $(BUILD) $(EXE) $(DEBUG)

$(EXE): $(SRC)
	$(CC) $(DEFS) $(CFLAGS) -o $(EXE) $(SRC) $(HEADERS) $(LFLAGS)

$(DEBUG): $(SRC)
	$(CC) $(DEFS) $(DFLAGS) -o $(DEBUG) $(SRC) $(HEADERS) $(LFLAGS)

run: $(EXE)
	./$(EXE)

debug: $(DEBUG)
	./$(DEBUG)

win: $(SRC)
	x86_64-w64-mingw32-gcc $(DEFS) $(CFLAGS) -o $(EXE) $(SRC) $(HEADERS) $(LFLAGS_WIN)

$(BUILD):
	mkdir -p $(BUILD)

.PHONY: clean
clean:
	rm -rf $(BUILD)
