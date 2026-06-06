#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="Release"
CLEAN=false
GENERATOR=""

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}========================================"
echo -e "  log-query Build Script (Linux/macOS)"
echo -e "========================================${NC}"
echo ""

while [[ $# -gt 0 ]]; do
    case $1 in
        Debug|Release) BUILD_TYPE="$1"; shift ;;
        --clean) CLEAN=true; shift ;;
        -G) GENERATOR="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

echo -e "${YELLOW}[1/4] Checking prerequisites...${NC}"

if ! command -v cmake &>/dev/null; then
    echo -e "${RED}ERROR: cmake not found. Install with: sudo apt install cmake${NC}"
    exit 1
fi
CMAKE_VER=$(cmake --version | head -1 | sed 's/cmake version //')
echo -e "  cmake : ${GREEN}${CMAKE_VER}${NC}"

if command -v g++ &>/dev/null; then
    GCC_VER=$(g++ --version | head -1)
    echo -e "  GCC   : ${GREEN}${GCC_VER}${NC}"
elif command -v clang++ &>/dev/null; then
    CLANG_VER=$(clang++ --version | head -1)
    echo -e "  Clang : ${GREEN}${CLANG_VER}${NC}"
else
    echo -e "${RED}ERROR: No C++ compiler found (g++ or clang++).${NC}"
    echo -e "${RED}  Install: sudo apt install build-essential${NC}"
    exit 1
fi

if [ ! -f "third_party/CLI11.hpp" ] || [ ! -f "third_party/json.hpp" ]; then
    echo -e "${RED}ERROR: third-party headers not found.${NC}"
    echo -e "${RED}  Run: ./scripts/download_deps.sh  to download them first.${NC}"
    exit 1
fi
echo -e "  deps  : ${GREEN}OK (CLI11.hpp + json.hpp)${NC}"

NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo -e "  CPU   : ${GREEN}${NPROC} cores${NC}"
echo ""

if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}[*] Cleaning build directory...${NC}"
    rm -rf build
fi

echo -e "${YELLOW}[2/4] Configuring CMake...${NC}"
cmake -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ${GENERATOR:+-G "$GENERATOR"}
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: CMake configuration failed!${NC}"
    exit 1
fi
echo ""

echo -e "${YELLOW}[3/4] Building...${NC}"
cmake --build build -j"$NPROC" --config "$BUILD_TYPE"
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Build failed!${NC}"
    exit 1
fi
echo ""

echo -e "${GREEN}[4/4] Build complete!${NC}"
EXE_PATH="$PROJECT_DIR/build/log-query"
if [ -f "$EXE_PATH" ]; then
    echo -e "  Binary: ${GREEN}${EXE_PATH}${NC}"
elif [ -f "$PROJECT_DIR/build/$BUILD_TYPE/log-query" ]; then
    EXE_PATH="$PROJECT_DIR/build/$BUILD_TYPE/log-query"
    echo -e "  Binary: ${GREEN}${EXE_PATH}${NC}"
fi
echo ""

echo -e "${CYAN}Quick test:${NC}"
echo -e "  echo '[2024-01-15 14:32:01.123] [ERROR] [lidar] [rx] timeout' | ${EXE_PATH} -f module=lidar -f level=ERROR"
