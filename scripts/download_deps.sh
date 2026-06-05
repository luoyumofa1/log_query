#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}Downloading third-party dependencies...${NC}"
echo ""

mkdir -p third_party

echo -e "${YELLOW}[1/2] CLI11.hpp (v2.4.2)...${NC}"
if command -v wget &>/dev/null; then
    wget -q -O third_party/CLI11.hpp "https://github.com/CLIUtils/CLI11/releases/download/v2.4.2/CLI11.hpp"
elif command -v curl &>/dev/null; then
    curl -sL -o third_party/CLI11.hpp "https://github.com/CLIUtils/CLI11/releases/download/v2.4.2/CLI11.hpp"
else
    echo -e "${RED}ERROR: wget or curl required.${NC}"
    exit 1
fi
echo -e "  ${GREEN}OK${NC}"

echo -e "${YELLOW}[2/2] json.hpp (v3.11.3)...${NC}"
if command -v wget &>/dev/null; then
    wget -q -O third_party/json.hpp "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
elif command -v curl &>/dev/null; then
    curl -sL -o third_party/json.hpp "https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"
fi
echo -e "  ${GREEN}OK${NC}"

echo ""
echo -e "${GREEN}All dependencies downloaded. Ready to build!${NC}"
echo -e "  Run: ${CYAN}./scripts/build.sh${NC}"
