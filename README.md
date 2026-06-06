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
# 按模块过滤
log-query app.log -f module=lidar

# 按日志级别过滤
log-query app.log -f level=ERROR

# 组合过滤（可重复使用 -f）
log-query app.log -f module=lidar -f level=ERROR

# 从管道读取
cat app.log | log-query -f module=planner -f level=WARN
```

### 时间范围过滤

```bash
# 指定时间区间
log-query app.log --from "2024-01-15 08:00:03" --to "2024-01-15 08:00:05"

# 只指定起始时间
log-query app.log --from "2024-01-15 08:00:07"

# 只指定结束时间
log-query app.log --to "2024-01-15 08:00:01"

# 时间 + 字段组合
log-query app.log --from "2024-01-15 08:00:03" --to "2024-01-15 08:00:05" -f level=ERROR
```

支持格式：`YYYY-MM-DD` 或 `YYYY-MM-DD HH:MM:SS`

### 正则过滤

```bash
# 在 message 字段中搜索关键词
log-query app.log --match "message=timeout|overflow"

# 在 module 字段中正则匹配
log-query app.log --match "module=.*driver"

# 正则 + 字段组合
log-query app.log -f level=ERROR --match "message=overflow|failed"
```

### 命令行参数

| 参数 | 说明 |
|------|------|
| `file` | 日志文件路径（`-` 表示 stdin） |
| `-f, --filter` | 字段精确过滤 `field=value`，可重复使用 |
| `--from` | 起始时间（`YYYY-MM-DD` 或 `YYYY-MM-DD HH:MM:SS`） |
| `--to` | 结束时间（`YYYY-MM-DD` 或 `YYYY-MM-DD HH:MM:SS`） |
| `-m, --match` | 正则过滤 `field=pattern`，可重复使用 |
| `--format-config` | 日志格式配置文件路径（默认 `config/adas_default.json`） |
| `--output` | 输出模式：`color`（默认） |

### 日志格式配置

默认支持智驾日志格式：

```
[2024-01-15 14:32:01.123] [ERROR] [lidar_driver] [rx_thread] Failed to read sensor data
```

可通过 `--format-config` 指定自定义格式的 JSON 配置文件。JSON 中通过 `level_field` 指定级别字段名，用于着色。

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
| Phase 2 | 🔄 | 增强过滤：时间范围 ✅、正则 ✅、统计模式 ⬜ |
| Phase 3 | ⬜ | 高级功能：Tail 模式、JSON/CSV 输出 |
| Phase 4 | ⬜ | 优化完善：性能优化、测试覆盖 |
