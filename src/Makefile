# Project: IronMoon

# Small and ugly, for now.
# ..actually, now it got bigger. But still ugly.

# Object, Source, and Binary directories.
ODIR = ../obj
SDIR = .
BDIR = ..

# Modules to be built in:
OBJ = ${ODIR}/main.o ${ODIR}/i_mapper.o ${ODIR}/i_lua.o ${ODIR}/im_merger.o
BIN = ${BDIR}/ironmoon

# Dependecies:
C_FLAGS = -g -Wall -I/usr/include/lua5.1
L_FLAGS = -llua5.1 -lz -lpcre -ldl

# Windows support
ifeq ($(OS),Win32)
 OBJ += ${ODIR}/winmain.o ${ODIR}/winres.res
 BIN = ${BDIR}/IronMoon.exe
 L_FLAGS = ../lua51.dll ../zlib1.dll ../pcre.dll -lwsock32 -lcomctl32 -mwindows
 C_FLAGS += -I../deps -DFOR_WINDOWS
endif


all: $(BIN)


# Extra dependencies
${ODIR}/i_mapper.o: ${SDIR}/i_mapper.h


$(BIN): $(OBJ)
	gcc -o $@ $^ $(L_FLAGS)

${ODIR}/%.o: ${SDIR}/%.c ${SDIR}/header.h ${SDIR}/module.h
	gcc -o $@ $< -c $(C_FLAGS)

${ODIR}/%.res: ${SDIR}/%.rc ${SDIR}/winres.h
	windres --input-format=rc -O coff -I src -i $< -o $@

