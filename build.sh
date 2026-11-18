#!/bin/bash
# Builds the Virtual ALU Simulator with g++ and nasm (no make needed).
#   ./build.sh          build the app       -> build/VirtualALUSim
#   ./build.sh test     build + run the core unit tests
#   ./build.sh run      build and start the app
# Needs: sudo apt install g++ nasm freeglut3-dev libglu1-mesa-dev
#
# Source layout
#   src/app      window, screens and widgets (the main program)
#   src/core     the CPU model that ties everything together
#   src/engine   everything the program runs, split by purpose:
#                  ds (data structures)  lang (compiler, English, patterns)
#                  gfx (OpenGL drawing)  asm (ALU, ISA, arithmetic, control unit - NASM)
#   src/<subject>/<CO>/   the CO files: complete stand-alone programs, one folder per course outcome
set -e
cd "$(dirname "$0")"
CXX=${CXX:-g++}
FLAGS="-std=c++17 -O2 -Wall -Wno-missing-field-initializers -Isrc -Ithird_party"
mkdir -p build/obj

# assemble the COA .asm files (only when changed)
ASM_OBJS=""
for src in $(find src/engine/asm -name '*.asm' | sort); do
    obj="build/obj/$(echo "$src" | tr "/" "_" | sed "s/\.asm$/.o/")"
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ] || [ src/engine/asm/abi.inc -nt "$obj" ]; then
        echo "  assembling $src"
        nasm -f elf64 -Isrc/ -o "$obj" "$src"
    fi
    ASM_OBJS="$ASM_OBJS $obj"
done

if [ "$1" = "test" ]; then
    # the core needs no graphics: data structures, language, CPU model and the pure-math clipping
    $CXX $FLAGS -o build/test_core tests/test_core.cpp \
        $(find src/core src/engine/ds src/engine/lang -name '*.cpp') \
        src/engine/gfx/Math.cpp src/engine/gfx/Clip.cpp src/engine/gfx/PolygonClip.cpp $ASM_OBJS
    ./build/test_core
    exit $?
fi

SOURCES=$(find src/app src/core src/engine -name '*.cpp' | sort)
JOBS=$(nproc 2>/dev/null || echo 4)
# compile changed files in parallel
echo "$SOURCES" | xargs -P "$JOBS" -I{} bash -c '
    src="{}"; obj="build/obj/$(echo "$src" | tr "/" "_" | sed "s/\.cpp$/.o/")"
    if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ] || [ -n "$(find src -name "*.h" -newer "$obj" 2>/dev/null | head -1)" ]; then
        echo "  compiling $src"
        '"$CXX $FLAGS"' -c "$src" -o "$obj" || exit 255
    fi'
# link only the objects that belong to current sources
OBJS=$(echo "$SOURCES" | sed 's#/#_#g; s#^#build/obj/#; s#\.cpp$#.o#')
$CXX $FLAGS -o build/VirtualALUSim $OBJS $ASM_OBJS -lglut -lGLU -lGL -lX11 -lm
echo "built build/VirtualALUSim"
if [ "$1" = "run" ]; then ./build/VirtualALUSim; fi
