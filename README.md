# log-query

面向智驾底层软件的轻量级命令行日志分析工具，支持结构化字段查询、时间范围过滤、统计汇总、实时监控。

## 快速开始

### 环境要求

- CMake >= 3.16
- C++17 编译器（GCC 8+ / Clang 7+ / MSVC 2019+）

### 下载依赖（仅首次）

```powershell
# Windows
.\scripts\download_deps.ps1

# Linux / macOS
chmod +x scripts/download_deps.sh
./scripts/download_deps.sh
```

### 编译

```powershell
# Windows
.\scripts\build.ps1

# Linux / macOS
chmod +x scripts/build.sh
./scripts/build.sh
```

编译产物：`build/log-query`（Linux）或 `build/log-query.exe`（Windows）

## 使用方式

### 基本过滤

```bash
# 按源文件过滤
log-query app.log -f source=heart_beat.cpp

# 按日志级别过滤
log-query app.log -f level=E

# 组合过滤（可重复使用 -f）
log-query app.log -f source=lidar_driver.cpp -f level=E

# 数值比较过滤（支持 > >= < <= !=）
log-query app.log -f "line>200"
log-query app.log -f "pid!=2640"

# 从管道读取
cat app.log | log-query -f source=planner.cpp -f level=W
```

### 时间范围过滤

```bash
# 指定时间区间
log-query app.log --from "2026-01-01 06:56:05" --to "2026-01-01 06:56:10"

# 只指定起始时间
log-query app.log --from "2026-01-01 06:56:14"

# 只指定结束时间
log-query app.log --to "2026-01-01 06:56:02"

# 时间 + 字段组合
log-query app.log --from "2026-01-01 06:56:05" --to "2026-01-01 06:56:10" -f level=E
```

支持格式：`YYYY-MM-DD` 或 `YYYY-MM-DD HH:MM:SS`

### 正则过滤

```bash
# 在 message 字段中搜索关键词
log-query app.log --match "message=timeout|overflow"

# 在 source 字段中正则匹配
log-query app.log --match "source=.*driver"

# 正则 + 字段组合
log-query app.log -f level=E --match "message=overflow|failed"
```

### 输出模式

默认彩色输出到终端。`plain` 模式在终端无颜色输出，适合重定向到文件或在不支持 ANSI 的终端中使用。`summary` 模式按源文件和日志级别统计汇总，输出交叉统计表。JSON/CSV 模式将匹配结果写入本地文件，便于后续处理。

```bash
# 默认：彩色终端输出
log-query app.log -f level=E

# 无颜色终端输出（适合管道重定向）
log-query app.log -f level=E --output plain

# 统计汇总模式（按源文件 × 级别统计）
log-query app.log --output summary
log-query app.log --output summary -f level=E
log-query app.log --output summary --from "2026-01-01 06:56:05" --to "2026-01-01 06:56:14"

# JSON 输出（自动生成带时间戳的文件名，如 log-query-result-20260606-133217.json）
log-query app.log -f level=E --output json

# CSV 输出（自动生成带时间戳的文件名，如 log-query-result-20260606-133217.csv）
log-query app.log -f level=E --output csv

# 指定输出文件路径（不自动加时间戳）
log-query app.log -f level=E --output json --output-file result.json
```

统计模式输出示例：

```
Module              T      D      I      W      E      F  Total
----------------------------------------------------------------------
can_driver.cpp      0      1      1      0      1      1      4
control.cpp         0      0      3      0      0      0      3
fusion_engine.cpp   0      0      3      1      0      0      4
heart_beat.cpp      0      0      3      1      0      0      4
lidar_driver.cpp    0      1      2      1      1      0      5
planner.cpp         0      0      2      1      1      0      4
radar_driver.cpp    0      0      1      0      0      0      1
----------------------------------------------------------------------
Total               0      2     15      4      3      1     25
```

### 命令行参数

| 参数 | 说明 |
|------|------|
| `file` | 日志文件路径（`-` 表示 stdin） |
| `-f, --filter` | 字段过滤，支持 `field=value`（精确匹配）和 `field>value`（数值比较，支持 `>` `>=` `<` `<=` `!=`），可重复使用 |
| `--from` | 起始时间（`YYYY-MM-DD` 或 `YYYY-MM-DD HH:MM:SS`） |
| `--to` | 结束时间（`YYYY-MM-DD` 或 `YYYY-MM-DD HH:MM:SS`） |
| `-m, --match` | 正则过滤 `field=pattern`，可重复使用 |
| `--format-config` | 日志格式配置文件路径（默认 `config/adas_default.json`） |
| `--output` | 输出模式：`color`（默认，终端彩色）、`plain`（终端无颜色）、`summary`（统计汇总）、`json`（写入文件）、`csv`（写入文件） |
| `--output-file` | 输出文件路径（json/csv 模式，不指定则自动生成带时间戳的文件名） |

### 日志格式配置

默认支持智驾底层软件日志格式：

```
[I][2640][6846][01-01 06:55:57.154][heart_beat.cpp:31] Heartbeat new record : 81 with heartBeat_timeout 500
[W][2640][6828][01-01 06:55:57.697][heart_beat.cpp:190] Process GUBMLabel with AppId 81 heartbeat timeout
```

默认配置字段：level（枚举 T/D/I/W/E/F）、pid（int）、tid（int）、timestamp（datetime，`%m-%d %H:%M:%S.%f`）、source（string）、line（int）、message（string）。

可通过 `--format-config` 指定自定义格式的 JSON 配置文件。JSON 中通过 `level_field` 指定级别字段名（用于着色），`module_field` 指定模块字段名（用于统计汇总）。

## 项目结构

```
log-query/
├── CMakeLists.txt
├── config/                  # 日志格式配置
├── include/log_query/       # 公共头文件
├── scripts/                 # 编译 & 工具脚本
├── src/                     # 源代码
│   ├── main.cpp
│   ├── config/              # 格式配置加载
│   ├── parser/              # 行解析器
│   ├── filter/              # 过滤器
│   ├── output/              # 输出渲染
│   └── util/                # 工具类
├── third_party/             # 第三方依赖（header-only）
└── tests/                   # 测试
```

## 开发阶段

| 阶段 | 状态 | 内容 |
|------|:----:|------|
| Phase 1 | ✅ | 基础框架：字段过滤 + 彩色输出 |
| Phase 2 | ✅ | 增强过滤：时间范围 ✅、正则 ✅、统计模式 ✅ |
| Phase 3 | 🔄 | 高级功能：JSON/CSV 输出 ✅、Plain 无颜色输出 ✅、Tail 模式 ⬜ |
| Phase 4 | 🔄 | 实用增强：上下文行（`-C N`） ⬜、数值比较过滤（`-f latency>100`） ✅、多文件输入 ⬜ |
| Phase 5 | ⬜ | 优化完善：性能优化、测试覆盖 |
