# log-query — 结构化日志查询工具 设计方案

> 一个面向智驾底层软件的轻量级命令行日志分析工具，支持结构化字段查询、时间范围过滤、统计汇总、实时监控。

---

## 目录

1. [整体架构](#一整体架构)
2. [日志格式配置](#二日志格式配置)
3. [查询 DSL 设计](#三查询-dsl-设计)
4. [核心模块设计](#四核心模块设计)
5. [关键实现细节](#五关键实现细节)
6. [依赖库选择](#六依赖库选择)
7. [文件结构总览](#七文件结构总览)
8. [分阶段实现计划](#八分阶段实现计划)
9. [使用场景示例](#九使用场景示例)

---

## 一、整体架构

```
┌──────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────┐
│  日志文件  │ →  │  格式解析器   │ →  │   过滤器链    │ →  │  输出渲染  │
│  (stdin/   │    │  (Parser)    │    │  (Filters)   │    │ (Renderer)│
│   file)    │    │              │    │              │    │           │
└──────────┘    └──────────────┘    └──────────────┘    └──────────┘
                      │                    │                  │
                      ▼                    ▼                  ▼
               ┌──────────────┐    ┌──────────────┐    ┌──────────┐
               │ 格式配置 JSON │    │  查询条件 AST │    │ 彩色/JSON │
               │ (logfmt.json)│    │              │    │  /CSV输出 │
               └──────────────┘    └──────────────┘    └──────────┘
```

### 核心设计原则

- **管道流式处理**：逐行解析，不缓存全文件，支持 GB 级日志
- **零拷贝解析**：使用 `std::string_view` 全程避免不必要的内存分配
- **短路求值**：过滤链中一旦某条件不满足，立即跳过后续检查
- **可配置格式**：日志格式通过 JSON 配置文件定义，适配不同项目

---

## 二、日志格式配置

### 2.1 典型智驾日志格式

```
[2024-01-15 14:32:01.123] [ERROR] [lidar_driver] [rx_thread] Failed to read sensor data: timeout
[2024-01-15 14:32:01.234] [INFO ] [planner]    [main]      Path calculated: 128 waypoints
```

### 2.2 格式描述 JSON

```json
{
  "name": "adas_default",
  "pattern": "[{timestamp}] [{level}] [{module}] [{thread}] {message}",
  "fields": {
    "timestamp": {
      "type": "datetime",
      "format": "%Y-%m-%d %H:%M:%S.%f"
    },
    "level": {
      "type": "enum",
      "values": ["TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"]
    },
    "module": {
      "type": "string"
    },
    "thread": {
      "type": "string"
    },
    "message": {
      "type": "string",
      "greedy": true
    }
  },
  "delimiters": ["[", "] "]
}
```

### 2.3 解析过程

1. **Tokenize**：将 `pattern` 拆分为 token 序列
   ```
   "[{timestamp}] [{level}] [{module}] [{thread}] {message}"
   → [FIXED:"[", FIELD:"timestamp", FIXED:"] [", FIELD:"level",
      FIXED:"] [", FIELD:"module", FIXED:"] [", FIELD:"thread",
      FIXED:"] ", FIELD:"message"]
   ```

2. **Match**：用 FIXED token 作为锚点分割原始行，提取 FIELD token 对应的值

3. **Convert**：按 `fields` 配置中的 `type` 对每个字段做类型转换

### 2.4 多格式支持

支持通过 `--format` 参数选择不同的格式配置：

```bash
$ log-query app.log --format adas_default    # 智驾默认格式
$ log-query app.log --format ros_log         # ROS 日志格式
$ log-query app.log --format syslog          # 标准 syslog
$ log-query app.log --format custom --pattern "..."  # 自定义格式
```

### 2.5 支持的字段类型

| 类型 | 说明 | 额外配置 |
|------|------|----------|
| `string` | 普通字符串 | `greedy: true` 表示贪婪匹配到行尾 |
| `datetime` | 日期时间 | `format`: strftime 格式字符串 |
| `enum` | 枚举值 | `values`: 可选值列表 |
| `int` | 整数 | — |
| `float` | 浮点数 | — |
| `regex` | 正则捕获组 | `pattern`: 正则表达式 |

---

## 三、查询 DSL 设计

### 3.1 命令行接口

不使用自定义语法解析器，直接用 CLI 参数组合，降低实现复杂度：

```bash
# 基础字段过滤
log-query app.log --module lidar --level ERROR

# 时间范围过滤
log-query app.log --from "2024-01-15 14:30" --to "2024-01-15 14:35"

# 消息内容正则匹配
log-query app.log --msg-regex "timeout|fail"

# 字段比较（支持 >、>=、<、<=、==、!=）
log-query app.log --level ">=WARN"

# 取反
log-query app.log --module lidar --level-not INFO

# 组合查询：lidar 模块在 14:30~14:35 的所有 ERROR，且消息包含 timeout
log-query app.log \
  --module lidar \
  --level ERROR \
  --from "14:30" --to "14:35" \
  --msg-regex "timeout"

# 统计模式
log-query app.log --summary --from "14:00" --to "15:00"

# Tail 模式（持续监控）
log-query app.log --tail --level ">=WARN" --highlight "timeout|fail"

# 输出格式
log-query app.log --output json
log-query app.log --output csv --fields timestamp,module,level,message

# 从 stdin 读取（管道模式）
cat app.log | log-query --module lidar --level ERROR

# 上下文行（类似 grep -A/-B）
log-query app.log --level ERROR --context-before 2 --context-after 3
```

### 3.2 内部查询模型

每条过滤条件是一个 `Filter` 对象，链式组合：

```
LineFilter (抽象基类)
├── TimeRangeFilter(from, to)          // 时间范围过滤
├── FieldEqualFilter(field, value)     // 字段精确匹配
├── FieldRegexFilter(field, regex)     // 字段正则匹配
├── FieldCompareFilter(field, op, val) // 字段比较 (>=, <=, etc.)
├── FieldNotEqualFilter(field, value)  // 字段取反
└── MessageRegexFilter(regex)          // 消息内容正则（特殊处理）
```

**FilterChain** 将所有 Filter 串联，只有通过全部 Filter 的行才输出：

```cpp
class FilterChain {
    std::vector<std::unique_ptr<Filter>> filters_;

public:
    bool match(const LogLine& line) const {
        for (auto& f : filters_) {
            if (!f->match(line)) return false;  // 短路求值
        }
        return true;
    }
};
```

### 3.3 比较运算符支持

字段比较中 `op` 支持：

| 运算符 | 写法 | 说明 |
|--------|------|------|
| `>=` | `--level ">=WARN"` | 枚举值按定义顺序比较 |
| `<=` | `--level "<=INFO"` | |
| `>` | `--count ">100"` | 数值比较 |
| `<` | `--latency "<50"` | |
| `==` | `--module "==lidar"` | 等价于 `--module lidar` |
| `!=` | `--module "!=lidar"` | 等价于 `--module-not lidar` |

---

## 四、核心模块设计

### 4.1 目录结构

```
src/
├── main.cpp                 # 入口，CLI 参数解析与分发
├── config/
│   └── log_format.cpp       # 日志格式配置加载
├── parser/
│   ├── line_parser.cpp      # 逐行解析器（模板匹配）
│   └── field.cpp            # 字段类型定义 + 类型转换
├── filter/
│   ├── filter.h             # Filter 抽象基类
│   ├── time_filter.cpp      # 时间范围过滤
│   ├── field_filter.cpp     # 字段值过滤（等值/正则/比较）
│   └── filter_chain.cpp     # 过滤器链
├── output/
│   ├── renderer.h           # 输出渲染抽象基类
│   ├── color_renderer.cpp   # 终端彩色输出
│   ├── json_renderer.cpp    # JSON 行输出
│   ├── csv_renderer.cpp     # CSV 输出
│   └── summary_renderer.cpp # 统计表格输出
└── util/
    ├── ansi.h               # ANSI 颜色码定义
    └── time_util.cpp        # 时间解析与比较工具
```

### 4.2 核心数据结构

#### LogLine — 解析后的日志行

```cpp
struct LogLine {
    int64_t line_number;                          // 原始行号
    std::string raw_text;                         // 原始文本（保留用于输出）
    std::map<std::string, FieldValue> fields;     // 解析后的字段值
    std::chrono::system_clock::time_point timestamp; // 解析后的时间戳
};
```

#### FieldValue — 字段值变体

```cpp
using FieldValue = std::variant<
    std::string,
    int64_t,
    double,
    std::chrono::system_clock::time_point
>;
```

#### Filter — 过滤器抽象

```cpp
class Filter {
public:
    virtual ~Filter() = default;
    virtual bool match(const LogLine& line) const = 0;
    virtual std::string describe() const = 0;  // 用于调试/日志
};
```

#### Renderer — 输出渲染抽象

```cpp
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void render_header() {}                    // 输出表头
    virtual void render_line(const LogLine& line) = 0; // 输出单行
    virtual void render_footer() {}                    // 输出统计
};
```

### 4.3 主流程

```cpp
int main(int argc, char** argv) {
    // 1. 解析 CLI 参数
    CLI::App app;
    // ... 参数定义 ...

    // 2. 加载日志格式配置
    auto format = LogFormat::load(config_path);

    // 3. 构建解析器
    LineParser parser(format);

    // 4. 构建过滤器链
    FilterChain chain;
    if (from_time) chain.add<TimeRangeFilter>(*from_time, *to_time);
    if (module)    chain.add<FieldEqualFilter>("module", *module);
    if (level)     chain.add<FieldCompareFilter>("level", op, *level);
    if (msg_regex) chain.add<MessageRegexFilter>(*msg_regex);
    // ...

    // 5. 选择渲染器
    auto renderer = create_renderer(output_mode);

    // 6. 打开日志文件，逐行处理
    renderer->render_header();
    for (auto& raw_line : file_reader) {
        auto parsed = parser.parse(raw_line);
        if (parsed && chain.match(*parsed)) {
            renderer->render_line(*parsed);
        }
    }
    renderer->render_footer();
}
```

---

## 五、关键实现细节

### 5.1 行解析器 — 模板匹配算法

```cpp
struct Token {
    enum Type { FIXED, FIELD };
    Type type;
    std::string value;
};

class LineParser {
    std::vector<Token> tokens_;
    std::map<std::string, FieldConfig> field_configs_;

public:
    std::optional<LogLine> parse(std::string_view raw) {
        LogLine line;
        size_t pos = 0;

        for (size_t i = 0; i < tokens_.size(); ++i) {
            auto& tok = tokens_[i];

            if (tok.type == Token::FIXED) {
                if (!raw.substr(pos).starts_with(tok.value))
                    return std::nullopt;  // 格式不匹配
                pos += tok.value.size();
            } else {
                // 找到下一个 FIXED token 作为字段结束锚点
                size_t end = find_next_fixed(raw, pos, i);
                std::string_view field_value = raw.substr(pos, end - pos);

                // 类型转换
                auto& config = field_configs_[tok.value];
                line.fields[tok.value] = convert_field(field_value, config);

                pos = end;
            }
        }
        return line;
    }

private:
    size_t find_next_fixed(std::string_view raw, size_t start, size_t token_idx) {
        // 找后续第一个 FIXED token 的位置
        for (size_t i = token_idx + 1; i < tokens_.size(); ++i) {
            if (tokens_[i].type == Token::FIXED) {
                auto found = raw.find(tokens_[i].value, start);
                if (found != std::string_view::npos)
                    return found;
            }
        }
        return raw.size();  // 最后一个字段到行尾
    }
};
```

**性能要点：**
- 全程使用 `string_view`，零字符串拷贝
- 只有最终匹配成功的行才构造 `LogLine` 对象
- 不匹配的行直接 `return nullopt`，跳过后续处理

### 5.2 时间范围过滤优化

日志通常是按时间顺序排列的，对于大文件可以用二分查找跳过无关部分：

```cpp
// 第一阶段实现：顺序扫描（简单可靠）
// 第二阶段优化：二分定位起始行

size_t binary_search_start(FileReader& reader, TimePoint from) {
    // 策略：每隔 N 行采样时间戳，二分定位到起始行附近
    // 然后顺序扫描到精确起始位置
}
```

第一版先顺序扫描，性能对于几 GB 的日志也足够（主要瓶颈在磁盘 IO 而非 CPU）。

### 5.3 统计模式实现

```cpp
class SummaryRenderer : public Renderer {
    // module -> level -> count
    std::map<std::string, std::map<std::string, int>> stats_;
    std::vector<std::string> level_order_;  // 保持列顺序
    int64_t total_lines_ = 0;

public:
    void render_line(const LogLine& line) override {
        auto module = std::get<std::string>(line.fields.at("module"));
        auto level  = std::get<std::string>(line.fields.at("level"));
        stats_[module][level]++;
        total_lines_++;
    }

    void render_footer() override {
        // 打印表格
        print_table_header();
        int total_info = 0, total_warn = 0, total_error = 0;
        for (auto& [module, level_counts] : stats_) {
            print_row(module, level_counts);
            total_info  += level_counts["INFO"];
            total_warn  += level_counts["WARN"];
            total_error += level_counts["ERROR"];
        }
        print_separator();
        print_total_row(total_info, total_warn, total_error);
    }
};
```

输出效果：

```
Module          INFO   WARN   ERROR  FATAL  总计
─────────────────────────────────────────────────
lidar_driver    1203    12      3      0   1218
planner          892    45      7      0    944
can_driver      3400     5      0      0   3405
fusion_engine    567    23      2      0    592
─────────────────────────────────────────────────
总计            6062    85     12      0   6159

时间范围: 2024-01-15 08:00:00 ~ 17:30:00
```

### 5.4 Tail 模式实现

```cpp
void tail_mode(const std::string& path) {
    // 1. seek 到文件末尾
    std::ifstream file(path, std::ios::ate);
    auto inode = get_inode(path);

    while (running) {
        std::string line;
        while (std::getline(file, line)) {
            process_line(line);
        }

        // 2. 检测 logrotate（inode 变化）
        auto current_inode = get_inode(path);
        if (current_inode != inode) {
            file.close();
            file.open(path);
            inode = current_inode;
        }

        // 3. 清除 EOF 标志，等待新数据
        file.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
```

### 5.5 上下文行支持

类似 `grep -A/-B`，维护一个固定大小的环形缓冲区：

```cpp
class ContextBuffer {
    std::deque<LogLine> before_buffer_;  // 最近 N 行
    int after_count_ = 0;                // 还需输出的后续行数

public:
    void push(const LogLine& line, bool matched, int before, int after) {
        if (matched) {
            // 输出之前缓存的行
            for (auto& ctx : before_buffer_)
                output(ctx, true);  // true = 上下文行，可灰显
            output(line, false);   // false = 匹配行，高亮
            after_count_ = after;
        } else if (after_count_ > 0) {
            output(line, true);
            after_count_--;
        } else {
            before_buffer_.push_back(line);
            if (before_buffer_.size() > before)
                before_buffer_.pop_front();
        }
    }
};
```

### 5.6 ANSI 彩色输出

```cpp
// util/ansi.h
namespace ansi {
    constexpr auto RESET   = "\033[0m";
    constexpr auto RED     = "\033[31m";
    constexpr auto GREEN   = "\033[32m";
    constexpr auto YELLOW  = "\033[33m";
    constexpr auto BLUE    = "\033[34m";
    constexpr auto MAGENTA = "\033[35m";
    constexpr auto CYAN    = "\033[36m";
    constexpr auto BOLD    = "\033[1m";
    constexpr auto DIM     = "\033[2m";
    constexpr auto GRAY    = "\033[90m";

    inline std::string colorize(const std::string& text, const char* color) {
        return std::string(color) + text + RESET;
    }
}
```

按日志级别着色：

| 级别 | 颜色 | 样式 |
|------|------|------|
| TRACE | 灰色 | 普通 |
| DEBUG | 灰色 | 普通 |
| INFO  | 白色 | 普通 |
| WARN  | 黄色 | 加粗 |
| ERROR | 红色 | 加粗 |
| FATAL | 红色背景 | 加粗 |

---

## 六、依赖库选择

| 功能 | 推荐库 | 选择理由 |
|------|--------|----------|
| CLI 参数解析 | [CLI11](https://github.com/CLIUtils/CLI11) | header-only，C++11，标准库风格 API |
| JSON 解析 | [nlohmann/json](https://github.com/nlohmann/json) | header-only，业界标准，语法简洁 |
| 正则表达式 | `std::regex` | 标准库自带，对于日志匹配场景性能足够 |
| 终端颜色 | 手写 ANSI 宏 | 几行代码，不值得引入第三方库 |
| 测试框架 | [Catch2](https://github.com/catchorg/Catch2) | header-only，BDD 风格 |
| 构建系统 | CMake 3.16+ | C++17 标准，智驾部门标配 |

**全部 header-only 依赖**，零编译依赖，开箱即用。

---

## 七、文件结构总览

```
log-query/
├── CMakeLists.txt
├── config/
│   └── adas_default.json            # 默认智驾日志格式配置
├── src/
│   ├── main.cpp                     # 入口
│   ├── config/
│   │   └── log_format.cpp           # 格式配置加载
│   ├── parser/
│   │   ├── line_parser.cpp          # 模板匹配解析器
│   │   └── field.cpp                # 字段类型与转换
│   ├── filter/
│   │   ├── filter.h                 # Filter 抽象基类
│   │   ├── filter_chain.cpp         # 过滤器链
│   │   ├── time_filter.cpp          # 时间范围过滤
│   │   └── field_filter.cpp         # 字段值过滤
│   ├── output/
│   │   ├── renderer.h               # Renderer 抽象基类
│   │   ├── color_renderer.cpp       # 终端彩色输出
│   │   ├── json_renderer.cpp        # JSON 行输出
│   │   ├── csv_renderer.cpp         # CSV 输出
│   │   └── summary_renderer.cpp     # 统计表格输出
│   └── util/
│       ├── ansi.h                   # ANSI 颜色码
│       ├── file_reader.h            # 文件读取（支持 stdin）
│       └── time_util.cpp            # 时间解析工具
├── include/
│   └── log_query/
│       ├── log_line.h               # LogLine 数据结构
│       ├── filter.h                 # Filter 接口
│       └── renderer.h               # Renderer 接口
├── tests/
│   ├── test_parser.cpp              # 解析器单元测试
│   ├── test_filter.cpp              # 过滤器单元测试
│   └── test_integration.cpp         # 集成测试
└── third_party/
    ├── CLI11.hpp
    └── json.hpp
```

---

## 八、分阶段实现计划

### Phase 1 — 基础框架（MVP）

**目标：** 跑通基本流程，实现核心功能

| 任务 | 说明 |
|------|------|
| 项目骨架搭建 | CMakeLists.txt、目录结构、依赖引入 |
| CLI 参数解析 | 支持 `--module`、`--level`、`--format`、`--file` |
| 格式配置加载 | 从 JSON 加载日志格式配置 |
| 行解析器 | 模板匹配解析，输出 `LogLine` 结构 |
| 字段过滤 | `FieldEqualFilter`，支持精确匹配 |
| 彩色渲染器 | 终端彩色输出，按级别着色 |
| 集成测试 | 端到端测试基本流程 |

**预估代码量：** ~400 行

### Phase 2 — 增强过滤

**目标：** 完善过滤能力

| 任务 | 说明 |
|------|------|
| 时间范围过滤 | `TimeRangeFilter`，时间解析与比较 |
| 正则匹配 | `FieldRegexFilter`、`MessageRegexFilter` |
| 字段比较 | `FieldCompareFilter`，支持 `>=`、`<=` 等运算符 |
| 统计模式 | `SummaryRenderer`，按模块+级别统计表格 |
| 取反过滤 | `FieldNotEqualFilter` |

**预估代码量：** ~300 行

### Phase 3 — 高级功能

**目标：** 实时监控 + 多格式输出

| 任务 | 说明 |
|------|------|
| Tail 模式 | `--tail` 实时监控，支持 logrotate 检测 |
| JSON 输出 | `JsonRenderer`，每行一个 JSON 对象 |
| CSV 输出 | `CsvRenderer`，支持自定义字段选择 |
| 上下文行 | `--context-before`、`--context-after` |
| 高亮功能 | `--highlight` 关键字高亮 |
| stdin 输入 | 支持管道模式 `cat log | log-query ...` |

**预估代码量：** ~300 行

### Phase 4 — 优化与完善

**目标：** 性能优化 + 测试覆盖

| 任务 | 说明 |
|------|------|
| 二分查找优化 | 大文件时间范围过滤的二分定位 |
| 多格式配置 | 内置 ROS、syslog 等常用格式 |
| 单元测试 | 解析器、过滤器、渲染器全覆盖 |
| 性能测试 | 大文件基准测试 |
| 错误处理 | 异常日志行处理、格式不匹配提示 |

**预估代码量：** ~300 行

**总代码量预估：1300~1500 行 C++。**

---

## 九、使用场景示例

### 场景 1：排查 lidar 模块最近的错误

```bash
$ log-query vehicle.log --module lidar --level ">=WARN" --tail --highlight "fail|timeout"

[14:32:01.123] [WARN ] [lidar] [rx] Lost 3 packets, seq gap detected
[14:32:05.456] [ERROR] [lidar] [rx] DMA buffer overflow       👈 高亮
[14:32:10.789] [WARN ] [lidar] [proc] Point cloud density low: 1200 pts
[14:33:01.234] [ERROR] [lidar] [rx] Sensor heartbeat timeout  👈 高亮
```

### 场景 2：确认规划模块从错误中恢复了没

```bash
$ log-query vehicle.log --module planner --from 14:32 --to 14:34 --level ">=WARN" -C 2

[14:32:04] [INFO ] [planner] [main] New trajectory planned: 200 waypoints
[14:32:05] [ERROR] [planner] [main] Path planning failed: obstacle too close  ← 出错了
[14:32:06] [WARN ] [planner] [main] Fallback to conservative path
[14:32:07] [INFO ] [planner] [main] Recovery successful                        ← 恢复了
[14:32:08] [INFO ] [planner] [main] Resume normal planning
```

### 场景 3：统计今天各模块的日志分布

```bash
$ log-query vehicle.log --from "08:00" --to "17:30" --summary

Module            INFO   WARN   ERROR  FATAL  总计
────────────────────────────────────────────────────
lidar_driver      1203    12      3      0   1218
radar_driver       892    45      7      0    944
can_driver        3400     5      0      0   3405
fusion_engine      567    23      2      0    592
planner           1200    34      5      0   1239
control            890    12      1      0    903
────────────────────────────────────────────────────
总计              8152   131     18      0   8301

⚠ planner 模块 WARN 数量偏高 (34)，建议关注
```

### 场景 4：管道组合使用

```bash
# 先过滤出 ERROR，再按模块统计
$ log-query vehicle.log --level ERROR | log-query --summary

# 配合 jq 进一步处理 JSON 输出
$ log-query vehicle.log --module lidar --output json | jq '.timestamp,.message'

# 搜索特定时间段内两个模块的交互
$ log-query vehicle.log --from 14:30 --to 14:31 --module lidar,planner \
  | grep -E "sensor|trajectory"
```

### 场景 5：CI/CD 集成

```bash
# 在 CI 中检查是否有 FATAL 日志
$ log-query build_output.log --level FATAL --output json
# 如果存在 FATAL，退出码为 1（失败），配合 CI 阻断流水线

# 检查是否有模块产生异常多的 WARN
$ warn_count=$(log-query vehicle.log --level WARN --output count)
$ if [ "$warn_count" -gt 100 ]; then exit 1; fi
```
