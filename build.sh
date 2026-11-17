#!/bin/bash
# Builds the Virtual ALU Simulator with plain g++ (no make needed).
#   ./build.sh          build the app       -> build/VirtualALUSim
#   ./build.sh test     build + run the core unit tests
#   ./build.sh run      build and start the app
# Needs: sudo apt install g++ freeglut3-dev libglu1-mesa-dev
#
# Source layout (one folder per subject, one sub-folder per course outcome):
#   src/PL_2310214L/     src/COA_2310221L/     src/CG_2310218L/     src/PSOOP_2310261L/     src/app/
set -e
cd "$(dirname "$0")"
CXX=${CXX:-g++}
FLAGS="-std=c++17 -O2 -Wall -Wno-missing-field-initializers -Isrc -Ithird_party"
mkdir -p build/obj

if [ "$1" = "test" ]; then
    # the core needs no graphics: PL + COA + PSOOP only
    $CXX $FLAGS -o build/test_core tests/test_core.cpp \
        $(find src/PL_2310214L src/COA_2310221L src/PSOOP_2310261L -name '*.cpp')
    ./build/test_core
    exit $?
fi

SOURCES=$(find src -name '*.cpp' | sort)
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
$CXX $FLAGS -o build/VirtualALUSim $OBJS -lglut -lGLU -lGL -lm
echo "built build/VirtualALUSim"
if [ "$1" = "run" ]; then ./build/VirtualALUSim; fi
