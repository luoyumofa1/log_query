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
METRICS_SAMPLE="$SCRIPT_DIR/sample_metrics.log"
METRICS_CONFIG="$SCRIPT_DIR/test_metrics_config.json"
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
    local cmd="$3"
    printf "  %s ... " "$name"
    local count
    count=$(eval "$cmd" | wc -l)
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

run_test "filter by module"           6 "cat '$SAMPLE' | '$EXE' -f module=lidar_driver"
run_test "filter by level ERROR"      3 "cat '$SAMPLE' | '$EXE' -f level=ERROR"
run_test "filter by module + level"   1 "cat '$SAMPLE' | '$EXE' -f module=lidar_driver -f level=ERROR"
run_test "filter no match"            0 "cat '$SAMPLE' | '$EXE' -f module=radar_driver -f level=ERROR"

echo ""
echo -e "${YELLOW}[2] Edge cases${NC}"

run_test "empty input"                0 "echo '' | '$EXE' -f module=lidar"
run_test "non-matching lines"         0 "printf 'not a log\nneither\n' | '$EXE' -f module=lidar"
run_test "case insensitive level"     3 "cat '$SAMPLE' | '$EXE' -f level=error"

echo ""
echo -e "${YELLOW}[3] Time range filtering${NC}"

run_test "filter by from and to"      6 "cat '$SAMPLE' | '$EXE' --from '2024-01-15 08:00:03' --to '2024-01-15 08:00:05'"
run_test "filter by from only"        8 "cat '$SAMPLE' | '$EXE' --from '2024-01-15 08:00:07'"
run_test "filter by to only"          6 "cat '$SAMPLE' | '$EXE' --to '2024-01-15 08:00:01'"
run_test "time range + field filter"  1 "cat '$SAMPLE' | '$EXE' --from '2024-01-15 08:00:03' --to '2024-01-15 08:00:05' -f level=ERROR"

echo ""
echo -e "${YELLOW}[4] Regex filtering${NC}"

run_test "regex on message"           3 "cat '$SAMPLE' | '$EXE' --match 'message=timeout|overflow'"
run_test "regex on module"           12 "cat '$SAMPLE' | '$EXE' --match 'module=.*driver'"
run_test "regex + field filter"       2 "cat '$SAMPLE' | '$EXE' -f level=ERROR --match 'message=overflow|failed'"

echo ""
echo -e "${YELLOW}[5] Plain output mode${NC}"

run_test "plain output line count"      3 "cat '$SAMPLE' | '$EXE' -f level=ERROR --output plain"
run_test "plain output with filter"     6 "cat '$SAMPLE' | '$EXE' -f module=planner --output plain"
run_test "plain matches color count"    3 "cat '$SAMPLE' | '$EXE' -f level=WARN --output plain"

echo ""
echo -e "${YELLOW}[6] Summary output mode${NC}"

run_test "summary all lines"           10 "cat '$SAMPLE' | '$EXE' --output summary"
run_test "summary with filter"          7 "cat '$SAMPLE' | '$EXE' --output summary -f level=ERROR"
run_test "summary with time range"     10 "cat '$SAMPLE' | '$EXE' --output summary --from '2024-01-15 08:00:03' --to '2024-01-15 08:00:07'"

echo ""
echo -e "${YELLOW}[7] Numeric comparison filtering${NC}"

run_test "int field greater than"       6 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'status>400' --output plain"
run_test "int field less than"          9 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'status<300' --output plain"
run_test "float field greater than"     5 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'resp_time>100' --output plain"
run_test "float field less or equal"    7 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'resp_time<=5.0' --output plain"
run_test "int not equal + string"       4 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'status!=200' -f 'service=payment' --output plain"
run_test "comparison + level"           3 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'status>=500' -f 'level=ERROR' --output plain"
run_test "comparison + summary"         6 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'resp_time>100' --output summary"
run_test "string field comparison"      0 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'service>0' --output plain"
run_test "nonexistent field"            0 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'nonexistent>0' --output plain"
run_test "int field equal"              7 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'status=200' --output plain"
run_test "int field equal no match"     0 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'status=999' --output plain"
run_test "float field equal"            1 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'resp_time=0.5' --output plain"
run_test "int equal + level"            2 "cat '$METRICS_SAMPLE' | '$EXE' --format-config '$METRICS_CONFIG' -f 'status=200' -f 'level=DEBUG' --output plain"

echo ""
echo -e "${YELLOW}[8] Pipe mode${NC}"

run_test "pipe from cat"              6 "cat '$SAMPLE' | '$EXE' -f module=planner"

echo ""
echo -e "${CYAN}========================================"
if [ "$FAILED" -eq 0 ]; then
    echo -e "  Results: ${GREEN}$PASSED passed, $FAILED failed${NC}"
else
    echo -e "  Results: ${RED}$PASSED passed, $FAILED failed${NC}"
fi
echo -e "========================================${NC}"

exit $FAILED
