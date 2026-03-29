CC=gcc
SRC=$(wildcard ./src/*.c)
HEADERS=$(wildcard ./src/*.h)
CFLAGS=-O3 -Wall -Wextra -Werror -Wno-absolute-value -Wno-sign-compare -pedantic
DFLAGS=-O0 -g -Wall -Wextra -Wno-absolute-value -Wno-sign-compare -pedantic
LIBDIR=lib
RAYLIB_PATH=thirdparty/raylib
RAYLIB=$(RAYLIB_PATH)/src/libraylib.a
RAYLIB_LNX=libraylib.a
RAYLIB_WIN=libraylib-win.a
LFLAGS_LNX=-L$(LIBDIR) -lm -l:$(RAYLIB_LNX) -lX11 -lglfw
LFLAGS_WIN=-static -L$(LIBDIR) -lm -l:$(RAYLIB_WIN) -lopengl32 -lgdi32 -lwinmm -lwinpthread -Wl,--subsystem,windows
LFLAGS=
MINGW=
ISWIN=
BUILD=build
EXE=$(BUILD)/wrun
DEBUG=$(BUILD)/wrun-debug
DEFS=

ifeq ($(OS),Windows_NT)
	LFLAGS+=$(LFLAGS_WIN)
	MINGW+=gcc
	ISWIN+=true
else
	LFLAGS+=$(LFLAGS_LNX)
	MINGW+=x86_64-w64-mingw32-gcc
	ISWIN+=false
endif

all: $(BUILD) $(EXE) $(DEBUG)

$(EXE): $(SRC) $(RAYLIB)
	$(CC) $(DEFS) $(CFLAGS) -o $(EXE) $(SRC) $(LFLAGS)

$(DEBUG): $(SRC) $(RAYLIB)
	$(CC) $(DEFS) $(DFLAGS) -o $(DEBUG) $(SRC) $(LFLAGS)

$(RAYLIB):
	git clone --depth=1 https://github.com/raysan5/raylib.git $(RAYLIB_PATH)
	make -j8 -C $(RAYLIB_PATH)/src
	$(ISWIN) || cp $(RAYLIB) $(LIBDIR)/$(RAYLIB_LNX)
	$(ISWIN) || make -j8 -C $(RAYLIB_PATH)/src clean
	$(ISWIN) || make -j8 -C $(RAYLIB_PATH)/src CC=$(MINGW) PLATFORM_OS=WINDOWS
	cp $(RAYLIB) $(LIBDIR)/$(RAYLIB_WIN)

run: $(EXE)
	./$(EXE)

debug: $(DEBUG)
	./$(DEBUG)

win: $(SRC) $(RAYLIB)
	$(MINGW) $(DEFS) $(CFLAGS) -o $(EXE) $(SRC) $(LFLAGS_WIN)

$(BUILD):
	mkdir -p $(LIBDIR)
	mkdir -p $(BUILD)

.PHONY: clean
clean:
	rm -rf $(BUILD) $(LIBDIR) $(RAYLIB_PATH)
