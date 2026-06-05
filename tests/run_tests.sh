#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"

EXE="$PROJECT_DIR/build/log-query"
if [ ! -f "$EXE" ]; then
    echo -e "\033[0;31mERROR: log-query not found. Run ./scripts/build.sh first.\033[0m"
    exit 1
fi

SAMPLE="$SCRIPT_DIR/sample.log"
PASSED=0
FAILED=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

run_test() {
    local name="$1"
    local expected="$2"
    shift 2
    printf "  %s ... " "$name"
    local count
    count=$("$@" | wc -l)
    count="${count## }"
    if [ "$count" -eq "$expected" ]; then
        echo -e "${GREEN}PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        echo -e "    ${RED}Expected $expected lines, got $count${NC}"
        FAILED=$((FAILED + 1))
    fi
}

echo -e "${CYAN}========================================"
echo -e "  log-query Integration Tests"
echo -e "========================================${NC}"
echo ""

echo -e "${YELLOW}[1] Basic field filtering${NC}"

run_test "filter by module"           6 cat "$SAMPLE" \| "$EXE" --module lidar_driver
run_test "filter by level ERROR"      3 cat "$SAMPLE" \| "$EXE" --level ERROR
run_test "filter by module + level"   1 cat "$SAMPLE" \| "$EXE" --module lidar_driver --level ERROR
run_test "filter no match"            0 cat "$SAMPLE" \| "$EXE" --module radar_driver --level ERROR

echo ""
echo -e "${YELLOW}[2] Edge cases${NC}"

run_test "empty input"                0 echo "" \| "$EXE" --module lidar
run_test "non-matching lines"         0 printf "not a log\nneither\n" \| "$EXE" --module lidar
run_test "case insensitive level"     3 cat "$SAMPLE" \| "$EXE" --level error

echo ""
echo -e "${YELLOW}[3] Pipe mode${NC}"

run_test "pipe from cat"              6 cat "$SAMPLE" \| "$EXE" --module planner

echo ""
echo -e "${CYAN}========================================"
if [ "$FAILED" -eq 0 ]; then
    echo -e "  Results: ${GREEN}$PASSED passed, $FAILED failed${NC}"
else
    echo -e "  Results: ${RED}$PASSED passed, $FAILED failed${NC}"
fi
echo -e "========================================${NC}"

exit $FAILED
