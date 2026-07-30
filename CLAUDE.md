# CLAUDE - IceClean 项目开发规范

本文件为 Claude Code 在工作时提供指导。整合了 ECC (Everything Claude Code) 通用规则和 C++ 编码标准。

---

## 项目概述

IceClean 是一款 Windows 智能C盘清理工具，使用 C++20 + wxWidgets 3.3.1 构建。

- **语言**: C++20（MSVC 14.44）
- **GUI框架**: wxWidgets 3.3.1
- **构建系统**: CMake 3.25+ + vcpkg（x64-windows-static triplet）
- **平台**: Windows 10/11 (64位)
- **依赖**: wxWidgets, nlohmann_json, spdlog, WebP
- **CI/CD**: GitHub Actions

---

## 核心指导原则（Guiding Principles）

这些是所有代码变更必须遵循的最高准则。

- **可读性优先（Readability First）**: C++代码是写给同行看的，不仅仅是编译器。优先选择清晰直白的写法，而非晦涩但"巧妙"的实现。
- **KISS（Keep It Simple, Stupid）**: 优先选择能实际工作的最简单方案；避免过早优化；清晰优于聪明。
- **DRY（Don't Repeat Yourself）**: 消除重复、提取公共逻辑到工具函数或基类中。在真正存在重复时再引入抽象。
- **YAGNI（You Aren't Gonna Need It）**: 不要在需要之前构建功能或抽象；避免投机性的通用设计；从简单开始，在需求明确时再重构。
- **高内聚，低耦合**: 模块化设计，core/gui/models/utils 之间保持清晰的依赖方向。
- **安全第一**: 本工具直接操作用户文件和注册表，必须始终考虑操作的安全性和可恢复性。
- **不可变性优先（Immutability by Default）**: 优先使用 `const`/`constexpr`，可修改是例外；创建新对象而非修改现有对象，避免隐藏的副作用。
- **类型安全（Type Safety）**: 利用 C++ 类型系统在编译期防止错误；使用 `enum class`、`nullptr`、`constexpr`、Concepts。
- **RAII 无处不在**: 将资源生命周期绑定到对象生命周期；所有资源（文件句柄、注册表键、Windows句柄）必须用 RAII 包装器管理。

---

## 架构约束

项目采用分层架构，严格遵守依赖方向：

```
gui/  →  core/  →  models/  →  utils/
  │         │         │          │
  └── 只依赖 ──→ 不能反向依赖 ──→┘
```

- `gui/` 只能调用 `core/` 的公共接口，不能直接操作文件系统或注册表
- `core/` 不能引用任何 wxWidgets 头文件
- `models/` 为纯数据模型，不包含业务逻辑
- `utils/` 为无状态的 Win32/文件/注册表工具函数

### 子模块结构

```
src/core/
├── scanner/    # 扫描器（12类）- 继承 ScannerBase
├── cleaner/    # 清理器（5类）- 继承 CleanerBase
├── migrator/   # 迁移器（6类）- 继承 MigratorBase
├── optimizer/  # 优化器 - 启动项/服务/进程
├── analyzer/   # 进程分析器
└── safety/     # 安全保护（还原点/白名单/操作日志）

src/gui/
├── controls/   # 自绘控件（CircularProgress/CardPanel/NavSidebar）
├── panels/     # 功能面板（7个）
└── dialogs/    # 对话框
```

---

## Git 工作流（来源：ECC git-workflow）

### 提交信息格式

所有提交信息必须遵循以下规范：

```
<类型>: <描述>
```

| 类型       | 含义           |
| ---------- | -------------- |
| `feat`     | 新功能         |
| `fix`      | Bug 修复       |
| `refactor` | 代码重构       |
| `docs`     | 文档更新       |
| `test`     | 测试相关       |
| `chore`    | 杂项/构建辅助  |
| `perf`     | 性能优化       |
| `ci`       | CI/CD 配置变更 |

### Pull Request 工作流

1. **分析完整提交历史** — 审查整个提交记录，不只看最新一次提交
2. **使用 `git diff [base-branch]...HEAD`** — 查看相对于基础分支的所有变更
3. **撰写全面的 PR 摘要** — 对变更内容进行完整描述
4. **包含测试计划及待办事项（TODOs）** — 说明测试覆盖情况和遗留问题
5. **新分支推送时使用 `-u` 标志** — 设置上游追踪分支

---

## 编码规范

### C++ 特定规则（融合 C++ Core Guidelines）

以下规则整合了 C++ Core Guidelines (isocpp.github.io) 与项目实践。标注 `CG.` 开头的规则来自 C++ Core Guidelines。

#### 类型安全与表达式（CG.ES.*）
- **初始化**: 始终初始化对象（ES.20），优先使用 `{}` 初始化语法（ES.23）
- **不可变默认**: 将对象声明为 `const` 或 `constexpr`（Con.1, ES.25），成员函数默认为 `const`（Con.2）
- **nullptr**: 使用 `nullptr` 而非 `0` 或 `NULL`（ES.47）
- **枚举**: 优先使用 `enum class` 而非普通 `enum`（Enum.3）；枚举值不要使用 ALL_CAPS（Enum.5）
- **强制转换**: 避免 C 风格强制转换，使用 `static_cast`/`const_cast` 等（ES.48）；不要去除 `const`（ES.50）
- **窄化**: 避免窄化/有损算术转换（ES.46）
- **魔法数字**: 使用命名的符号常量，避免魔法数字（ES.45）
- **作用域**: 保持作用域小（ES.5）

#### 函数设计（CG.F.*）
- 函数应执行单一逻辑操作（F.2），保持短小简洁（F.3）
- 可能编译期求值的函数声明为 `constexpr`（F.4）
- 不抛异常的函数声明为 `noexcept`（F.6）
- 优先使用纯函数（F.8）
- **参数传递**（F.16）：廉价拷贝类型按值传递，其他类型通过 `const&` 传递
- **输出值**（F.20）：优先通过返回值而非输出参数返回结果
- **多返回值**（F.21）：返回结构体/元组，而非多个输出参数
- 禁止返回局部对象的指针或引用（F.43）

```cpp
// F.16 + F.20 + F.21: 正确的参数与返回值
struct ParseResult {
    std::wstring token;
    int position;
};

ParseResult parse(std::wstring_view input);   // GOOD: 返回结构体

// BAD: 输出参数
void parse(std::wstring_view input, std::wstring& token, int& pos); // 避免这样
```

#### 类与类层次结构（CG.C.*）
- 存在不变式时使用 `class`，数据成员可独立变化时使用 `struct`（C.2）
- 最小化成员暴露（C.9）
- **零法则（Rule of Zero）**: 如能避免定义默认操作，则不要定义（C.20）
- **五法则（Rule of Five）**: 如定义或 `=delete` 任一拷贝/移动/析构，则全部处理（C.21）
- 单参数构造函数声明为 `explicit`（C.46）
- 构造函数应创建完全初始化的对象（C.41）
- 基类析构函数：public virtual 或 protected non-virtual（C.35）
- 多态类应禁止 public 拷贝/移动（C.67）
- 虚函数：恰好指定 `virtual`、`override` 或 `final` 之一（C.128）
- 禁止在构造函数/析构函数中调用虚函数（C.82）

#### 资源管理（CG.R.*）
- **RAII**: 自动管理资源，使用 RAII 包装器（R.1）
- **裸指针**: 裸指针 `T*` 表示非拥有（R.3），不传递所有权
- **栈对象优先**: 优先使用作用域对象，避免不必要的堆分配（R.5）
- **禁止 malloc/free**: 在 C++ 中避免 `malloc()`/`free()`（R.10）
- **禁止显式 new/delete**: 避免显式调用 `new` 和 `delete`（R.11）
- **智能指针**: 使用 `unique_ptr` 或 `shared_ptr` 表示所有权（R.20）
- **unique_ptr 优先**: 除非需要共享所有权，否则优先使用 `unique_ptr`（R.21）

```cpp
// R.11 + R.20 + R.21: RAII with smart pointers
auto widget = std::make_unique<Widget>(L"config");   // 唯一所有权
auto cache  = std::make_shared<Cache>(1024);           // 共享所有权

// R.3: 裸指针 = 非拥有的观察者
void render(const Widget* w) {  // 不拥有 w
    if (w) w->draw();
}
render(widget.get());
```

#### 错误处理（CG.E.*）
- 尽早设计错误处理策略（E.1）
- 抛出异常表示函数无法完成其任务（E.2）
- 使用 RAII 防止泄漏（E.6）
- 使用 `noexcept` 当不可能或不接受抛出时（E.12）
- 使用自定义异常类型（E.14），按值抛出、按引用捕获（E.15）
- 析构函数、释放和 swap 永不失败（E.16）
- 不要在每个函数中捕获所有异常（E.17）

```cpp
// E.14 + E.15: 自定义异常，按值抛出，按引用捕获
class CleanerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class FileAccessError : public CleanerError {
public:
    FileAccessError(const std::string& msg, DWORD code)
        : CleanerError(msg), errorCode(code) {}
    DWORD errorCode;
};

void cleanFile(const std::wstring& path) {
    // E.2: 抛出异常表示失败
    throw FileAccessError("access denied", ERROR_ACCESS_DENIED);
}

void run() {
    try {
        cleanFile(L"C:\\temp");
    } catch (const FileAccessError& e) {
        spdlog::error("{} (code: {})", e.what(), e.errorCode);
    } catch (const CleanerError& e) {
        spdlog::error(e.what());
    }
    // E.17: 不要在这里捕获所有异常
}
```

#### 模板与泛型编程（CG.T.*）
- 为所有模板参数指定 Concepts（C++20）（T.10）
- 优先使用标准 Concepts（T.11）
- 简单 Concept 使用简写语法（T.13）
- 优先使用 `using` 而非 `typedef`（T.43）
- 只在真正需要时使用模板元编程（T.120）
- 不要特化函数模板，使用重载代替（T.144）

```cpp
#include <concepts>

// T.10 + T.11: 用标准 Concepts 约束模板
template<std::integral T>
T gcd(T a, T b) {
    while (b != 0) {
        a = std::exchange(b, a % b);
    }
    return a;
}

// T.13: 简洁 Concept 语法
void sort(std::ranges::random_access_range auto& range) {
    std::ranges::sort(range);
}
```

#### 并发与并行（CG.CP.*）
- 避免数据竞争（CP.2），最小化可写数据的显式共享（CP.3）
- 以任务思维而非线程思维（CP.4）
- 不使用 `volatile` 进行同步（CP.8 — 仅用于硬件 I/O）
- 使用 RAII 锁，绝不用裸 `lock()`/`unlock()`（CP.20）
- 多个互斥量使用 `std::scoped_lock` 避免死锁（CP.21）
- 持有锁时不调用未知代码（CP.22 — 死锁风险）
- 必须命名 `lock_guard` 和 `unique_lock`（CP.44）
- 没有条件时不等待（CP.42）
- 不要使用无锁编程，除非绝对必要（CP.100）

```cpp
// CP.20 + CP.44: RAII 锁，必须命名
class ThreadSafeQueue {
public:
    void push(int value) {
        std::lock_guard<std::mutex> lock(mutex_);  // CP.44: 命名的!
        queue_.push(value);
        cv_.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); }); // CP.42
        int value = queue_.front();
        queue_.pop();
        return value;
    }
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<int> queue_;
};

// CP.21: 多个互斥量使用 scoped_lock
void transfer(Account& from, Account& to, double amount) {
    std::scoped_lock lock(from.mutex_, to.mutex_);
    from.balance_ -= amount;
    to.balance_ += amount;
}
```

#### 标准库使用（CG.SL.*）
- 尽可能使用库（SL.1），优先使用标准库（SL.2）
- 优先使用 `std::array` 或 `std::vector` 而非 C 数组（SL.con.1, SL.con.2）
- 使用 `std::string` 拥有字符序列，`std::string_view` 引用字符序列（SL.str.1, SL.str.2）
- 避免 `std::endl`，使用 `'\n'`（SL.io.50 — `endl` 强制刷新）

#### 头文件与命名（CG.SF.*, CG.NL.*）
- 使用 `.cpp` 用于代码文件，`.h` 用于接口文件（SF.1）
- 头文件使用 `#pragma once` 作为包含保护（SF.8）
- 头文件应是自包含的（SF.11）
- 禁止在头文件全局作用域使用 `using namespace`（SF.7）
- 避免在命名中编码类型信息（不使用匈牙利命名法）（NL.5）
- 使用一致的命名风格（NL.8）
- 只有宏名使用 ALL_CAPS（NL.9）

#### 性能（CG.Per.*）
- 没有理由不要优化（Per.1），不要过早优化（Per.2）
- 没有测量数据不要声称性能（Per.6）
- 设计上为优化留空间（Per.7）
- 依赖静态类型系统（Per.10）
- 将计算从运行期移至编译期（Per.11）
- 可预测地访问内存（Per.19）：优先使用连续数据结构

### 项目特有命名规范

- **类名**: `PascalCase` (如 `ScannerBase`, `PrivacyCleaner`)
- **函数/方法**: `camelCase` (如 `scanFiles`, `cleanRegistry`)
- **成员变量**: `m_` 前缀 + `camelCase` (如 `m_scanResult`, `m_hWnd`)
- **常量**: `kUpperCamelCase` (如 `kMaxLogEntries`)
- **枚举**: `enum class` + `PascalCase`

### 字符串处理

- 内部逻辑使用 `std::wstring`（Windows API 兼容）
- 与 wxWidgets 交互时使用 `wxString`，通过 `wxString::ToStdWstring()` / `wxString(wstr)` 转换
- 禁止使用 `strcpy`/`strcat`/`sprintf`，使用 `std::format` 或 `wxString::Format`

### 错误处理与日志

- Windows API 调用必须检查返回值，禁止忽略 NULL 或错误码
- 使用 `spdlog` 记录错误：`spdlog::error(...)`, `spdlog::warn(...)`
- 危险操作（删除文件、修改注册表）必须有前置检查和日志记录
- 在每个层级显式处理错误；绝不静默吞掉错误
- 在系统边界验证所有输入；绝不信任外部数据

### C++20 特性

优先使用 `std::format`、`std::ranges`、`concept`、`std::jthread`、`constexpr` 等现代特性。

### wxWidgets GUI 规则

- 事件绑定使用 `Bind()`，不使用事件表宏
- 自绘控件必须处理 HiDPI 缩放
- 长时间操作必须使用 `wxBusyCursor` 或后台线程，避免阻塞 UI
- 线程间通信使用 `wxThreadEvent` / `CallAfter()`
- 面板布局使用 `wxSizer` 体系，禁止硬编码像素值

### 文件组织

- **多小文件优于少大文件**：高内聚、低耦合
- 典型行数：200-400 行，上限 800 行
- 从大型模块中提取工具函数
- 按功能/领域组织，而非按类型

---

## 开发工作流

### TDD 工作流（来源：ECC testing）

遵循 RED → GREEN → REFACTOR 循环：

1. **RED**: 先写一个失败的测试，捕获新行为
2. **GREEN**: 实现最小改动使测试通过
3. **REFACTOR**: 在测试保持绿色的前提下清理代码

### 测试结构（AAA 模式）

```cpp
TEST(CleanerTest, RemovesEmptyDirectories) {
    // Arrange（准备）
    auto tempDir = createTestDirectory();
    auto cleaner = PrivacyCleaner{};

    // Act（执行）
    auto result = cleaner.clean(tempDir);

    // Assert（断言）
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.removedCount, 1);
}
```

### 测试命名规范

使用描述性名称，清晰表达被测试的行为：

```cpp
TEST(RegistryCleanerTest, ReturnsFalseWhenKeyIsWhitelisted) {}
TEST(FileScannerTest, SkipsSystemProtectedPaths) {}
TEST(StartupOptimizerTest, FallsBackToDefaultWhenRegistryUnavailable) {}
```

---

## 测试与质量保障

### 测试框架

- **单元测试**: GoogleTest + GoogleMock
- **集成测试**: 针对 API 端点和关键模块交互
- **E2E 测试**: 关键用户流程（清理、扫描、优化）
- **CMake/CTest**: 使用 `gtest_discover_tests()` 进行测试发现

### 覆盖率目标

- 最低 **80%** 测试覆盖率
- 优先测试边界条件、错误路径和安全关键路径

### Sanitizers（消毒器）

```cmake
# AddressSanitizer - 内存错误检测
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# UndefinedBehaviorSanitizer - 未定义行为检测
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# ThreadSanitizer - 数据竞争检测
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
```

### 测试运行

```bash
# 全部测试
ctest --test-dir build --output-on-failure

# 按名称过滤
ctest --test-dir build -R CleanerTest

# 单测可执行文件
./build/iceclean_tests --gtest_filter=PrivacyCleanerTest.*
```

### 测试失败排查

1. 重新运行单个失败测试
2. 检查测试隔离性
3. 验证 mock 是否正确
4. 修复实现而非测试（除非测试确实有误）
5. 添加 sanitizers 重新运行

---

## 安全编码规则（强化）

### 强制性安全检查（每次提交前）

- [ ] 无硬编码密钥（API密钥、密码、令牌）
- [ ] 所有用户输入已验证
- [ ] 错误消息不泄露敏感信息
- [ ] 密钥通过环境变量或密钥管理器管理，绝不硬编码

### 安全响应协议

如发现安全问题：
1. 立即停止当前操作
2. 修复 CRITICAL 级别问题后再继续
3. 如密钥泄露，立即轮换
4. 审查整个代码库中类似问题

### 项目特有安全规则

- **三级安全标识**: 🟢安全 / 🟡谨慎 / 🔴危险，所有扫描结果和操作必须标注
- **白名单路径保护**: 禁止删除白名单路径下的文件（`SafetyManager` 负责）
- **操作前确认**: 危险操作（🔴级别）必须弹出确认对话框
- **操作日志**: 所有清理/迁移操作必须记录到 JSON 日志
- **系统还原点**: 清理前自动创建系统还原点

---

## 代码质量检查清单

在标记 C++ 工作完成前：

- [ ] 无裸 `new`/`delete` — 使用智能指针或 RAII（R.11）
- [ ] 对象在声明处初始化（ES.20）
- [ ] 变量默认为 `const`/`constexpr`（Con.1, ES.25）
- [ ] 成员函数尽可能为 `const`（Con.2）
- [ ] `enum class` 而非普通 `enum`（Enum.3）
- [ ] `nullptr` 而非 `0`/`NULL`（ES.47）
- [ ] 无窄化转换（ES.46）
- [ ] 无 C 风格强制转换（ES.48）
- [ ] 单参数构造函数为 `explicit`（C.46）
- [ ] 零法则或五法则已应用（C.20, C.21）
- [ ] 基类析构函数正确（C.35）
- [ ] 模板已用 Concepts 约束（T.10, C++20）
- [ ] 头文件无全局作用域 `using namespace`（SF.7）
- [ ] 头文件有包含保护且自包含（SF.8, SF.11）
- [ ] 锁使用 RAII（`scoped_lock`/`lock_guard`）（CP.20）
- [ ] 异常为自定义类型，按值抛出，按引用捕获（E.14, E.15）
- [ ] 无魔法数字（ES.45）
- [ ] 函数简洁（< 50 行），文件聚焦（< 800 行）
- [ ] 无深层嵌套（> 4 层）
- [ ] 正确进行错误处理，无静默吞掉错误
- [ ] 无硬编码路径（应从环境变量或配置获取）

---

## 构建与测试

### 构建命令

```bash
# 使用本地构建脚本
build_local.bat

# 手动构建
call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set VCPKG_ROOT=<vcpkg路径>
cmake --preset x64-debug
cmake --build build/x64-debug
```

### 代码分析

```bash
# 静态分析
clang-tidy -p build/x64-debug src/core/scanner/*.cpp

# Cppcheck
cppcheck --enable=all --suppress=missingInclude src/
```

---

## 项目目录结构

```
iceClean/
├── src/
│   ├── core/                # 核心业务逻辑（不依赖 wxWidgets）
│   │   ├── scanner/         # 扫描器（12类，继承 ScannerBase）
│   │   ├── cleaner/         # 清理器（5类，继承 CleanerBase）
│   │   ├── migrator/        # 迁移器（6类，继承 MigratorBase）
│   │   ├── optimizer/       # 启动优化 + 服务优化
│   │   ├── analyzer/        # 进程分析器
│   │   └── safety/          # 安全保护（还原点/白名单/操作日志）
│   ├── gui/                 # GUI界面
│   │   ├── controls/        # 自绘控件
│   │   ├── panels/          # 功能面板（7个）
│   │   └── dialogs/        # 对话框
│   ├── models/              # 数据模型（纯数据，无逻辑）
│   ├── utils/               # 工具类（Win32/文件/注册表）
│   ├── App.cpp/h            # 应用程序入口
│   └── main.cpp             # WinMain 入口
├── vcpkg/                   # vcpkg 包管理
├── build_local.bat          # 本地编译脚本
├── build_release.bat        # 发布编译脚本
└── .github/workflows/      # CI 配置
```

---

## 关键依赖

- **编译器**: MSVC 14.44 (VS Build Tools 2022)
- **构建系统**: CMake 3.25+
- **包管理**: vcpkg（x64-windows-static triplet）
- **GUI框架**: wxWidgets 3.3.1
- **JSON库**: nlohmann_json
- **日志库**: spdlog
- **图片解码**: WebP (libwebp)
- **测试框架**: GoogleTest + GoogleMock
- **调试工具**: GDB / LLDB（如需）
- **静态分析**: Clang Static Analyzer / Cppcheck

---

## 代码示例参考

### RAII 句柄包装器示例

```cpp
// Windows 注册表键 RAII 包装
class RegistryKey {
public:
    RegistryKey() : m_key(nullptr) {}
    ~RegistryKey() { if (m_key) RegCloseKey(m_key); }

    RegistryKey(const RegistryKey&) = delete;
    RegistryKey& operator=(const RegistryKey&) = delete;
    RegistryKey(RegistryKey&& other) noexcept : m_key(other.m_key) {
        other.m_key = nullptr;
    }

    LSTATUS open(HKEY root, const std::wstring& subkey, REGSAM access = KEY_READ);
    operator HKEY() const { return m_key; }

private:
    HKEY m_key;
};
```

### 安全操作模式示例

```cpp
// 清理文件前的安全检查
bool SafeCleanFile(const std::wstring& path) {
    // 1. 白名单检查
    if (SafetyManager::instance().isWhitelisted(path)) {
        spdlog::warn(L"跳过白名单路径: {}", path);
        return false;
    }

    // 2. 安全级别验证
    if (getSafetyLevel(path) == SafetyLevel::Dangerous) {
        spdlog::warn(L"跳过危险路径: {}", path);
        return false;
    }

    // 3. 执行操作并记录日志
    bool success = FileUtil::deleteFile(path);
    OperationLog::instance().record(OperationType::Clean, path, success);

    return success;
}
```

### 扫描器继承示例

```cpp
// 自定义扫描器应继承 ScannerBase
class MyScanner : public ScannerBase {
public:
    MyScanner() : ScannerBase(ScanCategory::Custom, L"我的扫描器") {}

    ScanResult scan() override {
        ScanResult result;
        // 实现扫描逻辑...
        result.addItem({path, size, SafetyLevel::Safe, L"描述"});
        return result;
    }
};
```

---

## 操作规范

1. **修改代码前**: 必须理解模块在整体架构中的位置和依赖关系
2. **修改 core/ 时**: 确保不引入 wxWidgets 依赖，保持纯 C++ 逻辑
3. **修改 gui/ 时**: 确保通过 core/ 层的接口操作数据，不直接调用 Win32 API
4. **添加新扫描器/清理器**: 必须继承对应的 Base 类，实现虚函数接口
5. **涉及文件删除/注册表修改**: 必须添加白名单检查和操作日志记录
6. **涉及权限提升**: 需要在操作前请求管理员权限，并记录日志
7. **编码格式**: 源文件使用 UTF-8 编码（已在 CMake 中配置 `/utf-8`）
8. **新功能开发**: 遵循 TDD 流程（先写测试 → 实现 → 重构）
9. **代码完成后**: 使用 code-reviewer 代理审查代码
10. **构建失败时**: 使用 build-error-resolver 代理分析错误

---

## 禁止事项

- 禁止在 `core/` 中引入任何 wxWidgets 头文件
- 禁止忽略 Windows API 调用的返回值
- 禁止跳过白名单检查直接删除文件
- 禁止在非 UI 线程中直接操作 wxWidgets 控件
- 禁止硬编码文件路径（应从环境变量或配置中获取）
- 禁止使用 C 风格字符串操作函数（strcpy/sprintf/strcat）
- 禁止使用裸指针管理动态内存（使用智能指针）
- 禁止使用 `new`/`delete` 显式管理内存（R.11）
- 禁止使用 `malloc()`/`free()`（R.10）
- 禁止在头文件全局作用域使用 `using namespace`（SF.7）
- 禁止使用 C 风格强制转换（ES.48）
- 禁止使用普通 `enum`，使用 `enum class`（Enum.3）
- 禁止静默吞掉错误
- 禁止使用 `volatile` 进行同步（CP.8）
- 禁止使用 `0`/`NULL` 表示空指针，使用 `nullptr`（ES.47）

---

## 注意事项

- 这是 Windows 平台专用工具，不需要考虑跨平台兼容
- 本工具需要管理员权限运行，涉及系统还原点创建、服务管理等操作
- 使用 MSVC 编译器，不保证 GCC/Clang 兼容
- vcpkg 使用 static triplet 链接方式，注意依赖的 ABI 兼容性
- 所有面向用户的字符串应支持中文（已配置 `/utf-8` 编译选项）

---

## 来源说明

本文件整合了以下来源的规范：

| 来源 | 描述 |
|------|------|
| **IceClean 项目规范** | 原有项目架构、安全、构建规范 |
| **ECC coding-style** | 通用编码风格（KISS/DRY/YAGNI/不可变性） |
| **ECC git-workflow** | Git 提交与 PR 工作流 |
| **ECC testing** | TDD、AAA 模式、测试命名规范 |
| **ECC security** | 安全检查和密钥管理 |
| **ECC cpp-coding-standards** | C++ Core Guidelines (P.*, F.*, C.*, R.*, ES.*, E.*, Con.*, CP.*, T.*, SL.*, Enum.*, SF.*, NL.*, Per.*) |
| **ECC cpp-testing** | GoogleTest/CTest/sanitizers/coverage 规范 |
