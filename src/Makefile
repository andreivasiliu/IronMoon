# Project: IronMoon

# Small and ugly, for now.

# Modules to be built in:
OBJ = obj/main.o obj/i_mapper.o obj/i_lua.o obj/im_merger.o
BIN = ironmoon

# Dependecies:
C_FLAGS = -g -Wall -I/usr/include/lua5.1
L_FLAGS = -llua5.1 -lz -lpcre -ldl

# Windows support
ifeq ($(OS),Win32)
 OBJ += obj/winmain.o obj/winres.res
 BIN = IronMoon.exe
 L_FLAGS = lua51.dll zlib1.dll pcre.dll -lwsock32 -lcomctl32 -mwindows
 C_FLAGS += -Ideps -DFOR_WINDOWS
endif


all: $(BIN)

$(BIN): $(OBJ)
	gcc -o $@ $^ $(L_FLAGS)

obj/%.o: src/%.c src/header.h
	gcc -o $@ $< -c $(C_FLAGS)

obj/%.res: src/%.rc src/winres.h
	windres --input-format=rc -O coff -I src -i $< -o $@

