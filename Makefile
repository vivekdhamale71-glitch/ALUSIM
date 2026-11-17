# Virtual ALU Simulator - build with:  make        run with:  make run
# Linux:   sudo apt install g++ freeglut3-dev libglu1-mesa-dev
# Windows (MSYS2 MinGW64):  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-freeglut
#                           make OS=Windows_NT
#
# Sources are organised by subject and course outcome:
#   src/PL_2310214L  src/COA_2310221L  src/CG_2310218L  src/PSOOP_2310261L  src/app

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wno-missing-field-initializers
CXXFLAGS += -Isrc -Ithird_party
SRC      := $(shell find src -name '*.cpp')
OBJ      := $(patsubst src/%.cpp,build/obj/%.o,$(SRC))
CORE     := $(shell find src/PL_2310214L src/COA_2310221L src/PSOOP_2310261L -name '*.cpp')

ifeq ($(OS),Windows_NT)
  LIBS   := -lfreeglut -lglu32 -lopengl32 -lgdi32 -lwinmm
  TARGET := build/VirtualALUSim.exe
else
  LIBS   := -lglut -lGLU -lGL -lm
  TARGET := build/VirtualALUSim
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

build/obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(OBJ:.o=.d)

test: build/test_core
	./build/test_core

build/test_core: tests/test_core.cpp $(CORE)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

.PHONY: all test run clean
