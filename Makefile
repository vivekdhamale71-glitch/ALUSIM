# Virtual ALU Simulator - build with:  make        run with:  make run
# Linux:   sudo apt install g++ nasm freeglut3-dev libglu1-mesa-dev
# Windows (MSYS2 MinGW64):  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-nasm mingw-w64-x86_64-freeglut
#                           make OS=Windows_NT
#
# Sources are organised by subject and course outcome:
#   src/PL_2310214L (C++)  src/COA_2310221L (NASM)  src/CG_2310218L (C++/OpenGL)
#   src/PSOOP_2310261L (C++)  src/core  src/app

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wno-missing-field-initializers
CXXFLAGS += -Isrc -Ithird_party
SRC      := $(shell find src -name '*.cpp')
ASM      := $(shell find src/COA_2310221L -name '*.asm')
OBJ      := $(patsubst src/%.cpp,build/obj/%.o,$(SRC))
ASMOBJ   := $(patsubst src/%.asm,build/obj/%.asm.o,$(ASM))
CORE     := $(shell find src/core src/PL_2310214L src/PSOOP_2310261L -name '*.cpp')

ifeq ($(OS),Windows_NT)
  LIBS     := -lfreeglut -lglu32 -lopengl32 -lgdi32 -lwinmm
  TARGET   := build/VirtualALUSim.exe
  ASMFMT   := win64
else
  LIBS     := -lglut -lGLU -lGL -lm
  TARGET   := build/VirtualALUSim
  ASMFMT   := elf64
endif

all: $(TARGET)

$(TARGET): $(OBJ) $(ASMOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# COA assembly: the ALU, ISA helpers, arithmetic and control unit
build/obj/%.asm.o: src/%.asm src/COA_2310221L/abi.inc
	@mkdir -p $(dir $@)
	nasm -f $(ASMFMT) -Isrc/ -o $@ $<

-include $(OBJ:.o=.d)

test: build/test_core
	./build/test_core

build/test_core: tests/test_core.cpp $(CORE) $(ASMOBJ)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

.PHONY: all test run clean
