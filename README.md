# 实验报告记录工具 (ExperimentReportTool)

基于 **Qt + C++** 开发的桌面端实验报告记录工具，支持实验项目管理、模板化报告创建、结构化富文本编辑、实验数据表格与图表、多格式导出、全文检索、打印、版本管理、公式编辑、数据导入、标签管理、附件管理等完整功能。

## 项目状态

当前版本：**v1.0.0（完整功能版）**

### 已完成功能

#### P0 基础功能
- ✅ **P0-1 基础框架** — 项目骨架、CMake 构建系统、核心实体模型、SQLite 数据库层、仓储层、主窗口三栏布局、项目树、报告列表、日志系统
- ✅ **P0-2 模板与富文本编辑器** — 块级编辑器体系（14 种块类型）、文本块（标题/段落/列表/引用/富文本格式）、表格块、图片块、代码块、分割线、图表块、自动保存（3 秒防抖）、模板编辑器
- ✅ **P0-3 数据表格与图表** — 数据表编辑器（动态增删行列、列属性面板、数据校验）、Qt Charts 图表渲染（折线/柱状/饼图/散点/面积）、图表配置对话框
- ✅ **P0-4 导出与全文检索** — PDF/HTML/Word/纯文本 4 种格式导出、FTS5 全文索引搜索、搜索结果对话框（高亮预览/项目筛选/搜索历史）
- ✅ **P0-5 打印与版本管理** — 打印预览、页面设置、直接打印、版本历史（保存/恢复/删除/预览）

#### P1 高级功能
- ✅ **P1-1 公式编辑器** — LaTeX 公式输入、MathJax 实时预览、20 个常用公式模板、行内/块级模式
- ✅ **P1-2 数据导入** — CSV 文件导入、自动检测分隔符（逗号/分号/制表符/竖线）、自动检测编码（UTF-8/GBK）、数据预览、三种导入模式（追加/替换/新表）
- ✅ **P1-3 标签管理** — 标签 CRUD、12 种预设颜色、报告-标签多对多关联、按名称自动创建标签、使用次数统计
- ✅ **P1-4 附件管理** — 文件上传（批量/进度条）、下载、用系统默认程序打开、删除、文件类型识别（图片/PDF/文档/压缩包/音视频）、右键菜单

## 技术栈

| 类别 | 技术 | 版本要求 |
|------|------|----------|
| 框架 | Qt | 5.15+ / 6.2+ |
| 语言 | C++ | C++17 |
| 构建 | CMake | 3.16+ |
| 数据库 | SQLite | 3.40+（FTS5 全文索引） |
| 图表 | Qt Charts | 随 Qt |
| 打印 | Qt PrintSupport | 随 Qt |
| 公式渲染 | MathJax 3 | CDN（需网络） |

## 目录结构

```
ExperimentReportTool/
├── CMakeLists.txt                    # CMake 构建配置（支持 Qt5/Qt6 自动检测）
├── README.md                         # 项目说明文档
├── src/
│   ├── main.cpp                      # 程序入口
│   │
│   ├── core/                         # 核心层
│   │   ├── models/                   # 实体模型（6 个）
│   │   │   ├── Project.h/cpp             # 实验项目（树状结构）
│   │   │   ├── Report.h/cpp              # 实验报告（含 ContentBlock 块结构 + JSON 序列化）
│   │   │   ├── Template.h/cpp            # 报告模板
│   │   │   ├── DataTable.h/cpp           # 实验数据表（列定义 + 数据校验）
│   │   │   ├── Tag.h/cpp                 # 标签
│   │   │   └── Attachment.h/cpp          # 附件
│   │   └── utils/                    # 工具类
│   │       ├── AppConstants.h            # 全局常量
│   │       ├── Logger.h/cpp              # 线程安全日志（文件滚动）
│   │       └── CsvParser.h/cpp           # CSV 解析器（自动检测分隔符/编码）
│   │
│   ├── data/                         # 数据层
│   │   ├── database/
│   │   │   └── DatabaseManager.h/cpp     # 数据库管理器（单例/WAL/建表/迁移/FTS5）
│   │   └── repositories/             # 仓储层（6 个）
│   │       ├── ProjectRepository.h/cpp
│   │       ├── ReportRepository.h/cpp      # 含全文搜索 + 版本管理
│   │       ├── TemplateRepository.h/cpp
│   │       ├── DataTableRepository.h/cpp
│   │       ├── TagRepository.h/cpp
│   │       └── AttachmentRepository.h/cpp
│   │
│   ├── editor/                       # 编辑器层
│   │   ├── BlockEditor.h/cpp            # 块编辑器基类（操作手柄/类型转换/键盘导航）
│   │   ├── TextBlockEditor.h/cpp        # 文本块（标题/段落/列表/引用/富文本格式）
│   │   ├── OtherBlockEditors.h/cpp      # 其他块（表格/图片/代码/分割线/图表 + 工厂）
│   │   ├── ReportEditor.h/cpp            # 报告编辑器主组件（块管理/焦点导航/状态栏）
│   │   ├── AutoSaveManager.h/cpp         # 自动保存管理器（3 秒防抖/失败重试）
│   │   ├── DataTableEditorDialog.h/cpp   # 数据表编辑器对话框
│   │   └── DataImportDialog.h/cpp        # 数据导入对话框（CSV）
│   │
│   ├── chart/                        # 图表模块
│   │   ├── ChartRenderer.h/cpp          # 图表渲染器（Qt Charts，5 种图表类型）
│   │   └── ChartConfigDialog.h/cpp      # 图表配置对话框
│   │
│   ├── export/                       # 导出模块
│   │   └── ExportManager.h/cpp          # 导出管理器（PDF/HTML/Word/纯文本）
│   │
│   ├── search/                       # 搜索模块
│   │   ├── SearchService.h/cpp          # 搜索服务（FTS5 + LIKE 降级/搜索历史）
│   │   └── SearchResultDialog.h/cpp     # 搜索结果对话框
│   │
│   ├── print/                        # 打印模块
│   │   └── PrintManager.h/cpp           # 打印管理器（预览/页面设置/打印）
│   │
│   ├── formula/                      # 公式模块
│   │   ├── FormulaBlockEditor.h/cpp     # 公式块编辑器
│   │   └── FormulaEditorDialog.h/cpp    # 公式编辑对话框（LaTeX + MathJax）
│   │
│   ├── version/                      # 版本管理模块
│   │   └── VersionHistoryDialog.h/cpp   # 版本历史对话框
│   │
│   ├── template/                     # 模板模块
│   │   └── TemplateEditorDialog.h/cpp  # 模板编辑器对话框
│   │
│   └── ui/                           # 界面层
│       ├── MainWindow.h/cpp             # 主窗口（三栏布局/菜单/工具栏/状态栏）
│       ├── ReportEditorWindow.h/cpp     # 报告编辑独立窗口
│       ├── widgets/
│       │   ├── ProjectTreeWidget.h/cpp  # 项目树组件
│       │   └── ReportListWidget.h/cpp   # 报告列表组件
│       └── dialogs/
│           ├── ProjectDialog.h/cpp       # 项目编辑对话框
│           ├── TagManagerDialog.h/cpp    # 标签管理对话框
│           ├── ReportTagDialog.h/cpp     # 报告标签选择对话框
│           └── AttachmentManagerDialog.h/cpp  # 附件管理对话框
│
├── tests/                          # 单元测试（预留）
├── docs/                           # 文档（预留）
└── packaging/                      # 安装包脚本（预留）
```

## 编译方法

### 前置要求

- **Qt** 5.15+ 或 6.2+（必须包含以下模块：Core、Gui、Widgets、Sql、Charts、PrintSupport）
- **CMake** 3.16+
- **C++17** 兼容编译器：GCC 9+、Clang 10+、MSVC 2019+

### Linux / macOS

```bash
# 1. 创建构建目录
mkdir build && cd build

# 2. 配置项目（如果 Qt 不在默认路径，需指定 CMAKE_PREFIX_PATH）
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.0/gcc_64

# 3. 编译（使用所有 CPU 核心）
make -j$(nproc)

# 4. 运行程序
./bin/ExperimentReportTool
```

### Windows (MSVC)

```powershell
# 1. 创建构建目录
mkdir build
cd build

# 2. 配置项目（使用 Visual Studio 生成器，指定 Qt 路径）
cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64

# 3. 编译（Release 模式）
cmake --build . --config Release

# 4. 运行程序
.\bin\Release\ExperimentReportTool.exe
```

### Windows (MinGW)

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\mingw_64
mingw32-make -j
```

### 使用 qmake（备选方案）

如果更习惯 qmake，可以创建 `ExperimentReportTool.pro` 文件：

```qmake
QT += core gui widgets sql charts printsupport
CONFIG += c++17
TARGET = ExperimentReportTool
TEMPLATE = app

# 递归包含源文件
SOURCES += $$files(src/*.cpp, true)
HEADERS += $$files(src/*.h, true)

# 包含路径
INCLUDEPATH += src
```

## 功能使用指南

### 1. 项目管理
- 左侧项目树：右键新建项目/子项目、重命名、删除
- 支持树状项目结构（项目 → 子项目 → 报告）

### 2. 报告创建与编辑
- 双击报告打开独立编辑窗口
- 支持 14 种内容块：标题(H1-H3)、段落、无序列表、有序列表、引用、表格、图片、代码块、分割线、图表、公式、数据引用
- 快捷键：Enter 新建块、Backspace 删除空块、Alt+Up/Down 移动块
- 自动保存：3 秒防抖，失败自动重试（最多 3 次）

### 3. 数据表
- 插入表格块 → 双击打开数据表编辑器
- 动态增删行列、设置列属性（名称/类型/单位/必填/数值范围）
- 数据校验、CSV 导入导出

### 4. 图表
- 插入图表块 → 点击「配置图表」选择数据表和图表类型
- 支持 5 种图表：折线图、柱状图、饼图、散点图、面积图
- 可配置标题、轴标题、图例、网格、数据点、主题
- 点击「编辑数据」直接修改数据表，图表自动刷新

### 5. 公式
- 插入公式块 → 双击打开公式编辑器
- 输入 LaTeX 公式，右侧实时预览（MathJax）
- 20 个常用公式模板：分数、根号、求和、积分、矩阵、方程组等
- 支持行内/块级模式

### 6. 导出
- 文件 → 导出 → 选择格式（PDF/HTML/Word/纯文本）
- PDF：高质量打印渲染
- HTML：完整 CSS 样式，图片内嵌
- Word：Word 兼容格式（.doc）
- 纯文本：Markdown 风格

### 7. 全文搜索
- 主窗口顶部搜索框 → 输入关键词 → 回车
- FTS5 全文索引，高亮匹配摘要
- 支持项目筛选、搜索历史

### 8. 打印
- 文件 → 打印预览 / 打印 / 页面设置
- 支持 A4/Letter 等纸张、横竖屏、页边距

### 9. 版本管理
- 文件 → 版本历史
- 保存当前版本（可命名）、恢复历史版本、删除版本
- 版本内容预览

### 10. 标签管理
- 编辑 → 编辑标签：为当前报告选择标签
- 编辑 → 管理标签：新建/编辑/删除标签，设置颜色
- 标签自动创建：输入不存在的标签名自动创建

### 11. 附件管理
- 编辑 → 附件管理
- 上传文件（支持批量，进度条）、下载、打开、删除
- 文件存储在 `~/.ExperimentReportTool/attachments/`

### 12. 数据导入
- 数据表编辑器 → 导入 CSV
- 自动检测分隔符和编码
- 数据预览（前 20 行）
- 三种导入模式：追加到现有数据、替换现有数据、创建新数据表

## 数据库设计

### 数据表清单（10 张）

| 表名 | 说明 |
|------|------|
| `projects` | 实验项目（支持树状结构，parent_id 自关联） |
| `reports` | 实验报告（内容以 JSON 数组存储） |
| `report_versions` | 报告版本快照 |
| `templates` | 报告模板 |
| `data_tables` | 实验数据表（列定义 JSON + 数据 JSON） |
| `tags` | 标签 |
| `report_tags` | 报告-标签多对多关联 |
| `attachments` | 附件（文件元信息 + 存储路径） |
| `app_meta` | 应用元信息（版本号、数据库版本等） |
| `reports_fts` | FTS5 全文索引虚拟表（标题 + 内容 + 标签） |

### 报告内容结构（JSON）

报告内容以 JSON 数组存储，每个元素是一个"内容块"（ContentBlock）：

```json
[
  {
    "id": "uuid-xxx",
    "type": "heading1",
    "data": { "text": "一、实验目的" }
  },
  {
    "id": "uuid-yyy",
    "type": "paragraph",
    "data": { "text": "验证牛顿第二定律 F=ma...", "alignment": "left" }
  },
  {
    "id": "uuid-zzz",
    "type": "bullet_list",
    "data": { "items": ["实验器材", "实验步骤", "数据记录"] }
  },
  {
    "id": "uuid-aaa",
    "type": "table",
    "data": { "tableId": 123 }
  },
  {
    "id": "uuid-bbb",
    "type": "chart",
    "data": { "dataTableId": 123, "chartType": "line", "xColumn": "时间", "yColumns": ["速度"] }
  },
  {
    "id": "uuid-ccc",
    "type": "formula",
    "data": { "latex": "F = ma", "inline": false }
  }
]
```

### 支持的块类型（14 种）

| 类型枚举 | JSON type | 说明 |
|----------|-----------|------|
| `Heading1` | `heading1` | 一级标题 |
| `Heading2` | `heading2` | 二级标题 |
| `Heading3` | `heading3` | 三级标题 |
| `Paragraph` | `paragraph` | 段落 |
| `BulletList` | `bullet_list` | 无序列表 |
| `NumberedList` | `numbered_list` | 有序列表 |
| `Quote` | `quote` | 引用 |
| `Table` | `table` | 表格（引用 data_tables 表） |
| `Image` | `image` | 图片 |
| `CodeBlock` | `code_block` | 代码块 |
| `Chart` | `chart` | 图表 |
| `Divider` | `divider` | 分割线 |
| `Formula` | `formula` | 数学公式（LaTeX） |
| `DataReference` | `data_reference` | 数据引用（预留） |

## 架构设计

采用经典的**分层架构**：

```
┌─────────────────────────────────────────┐
│  UI Layer (Qt Widgets)                   │  主窗口、编辑器、对话框、组件
├─────────────────────────────────────────┤
│  Editor Layer                             │  块编辑器、报告编辑器、自动保存
├─────────────────────────────────────────┤
│  Application Layer                        │  导出、搜索、打印、导入等业务逻辑
├─────────────────────────────────────────┤
│  Domain Layer                             │  实体模型、业务规则、JSON 序列化
├─────────────────────────────────────────┤
│  Infrastructure Layer                     │  数据库、仓储、文件存储、日志
└─────────────────────────────────────────┘
```

### 核心设计模式

- **仓储模式 (Repository Pattern)**：数据访问封装在 Repository 类中，上层不直接操作 SQL
- **单例模式 (Singleton)**：DatabaseManager、Logger 使用单例
- **工厂模式 (Factory Pattern)**：BlockEditorFactory 根据块类型创建对应的编辑器
- **观察者模式 (Observer Pattern)**：Qt 信号槽机制实现组件间通信

## 数据存储位置

| 平台 | 路径 |
|------|------|
| Windows | `%LOCALAPPDATA%\ExperimentReportTool\` |
| macOS | `~/Library/Application Support/ExperimentReportTool/` |
| Linux | `~/.ExperimentReportTool/` |

包含：
- `experiment_reports.db` — SQLite 数据库文件
- `attachments/` — 报告附件存储目录
- `logs/` — 日志文件目录（按日期滚动）

## 开发计划

| 阶段 | 目标 | 状态 |
|------|------|------|
| P0-1 | 基础框架（项目/报告/模板/数据表 + 数据库 + 主窗口） | ✅ 完成 |
| P0-2 | 模板与富文本编辑器（块级编辑器 + 自动保存） | ✅ 完成 |
| P0-3 | 数据表格与图表（数据表编辑器 + Qt Charts） | ✅ 完成 |
| P0-4 | 导出与全文检索（PDF/HTML/Word + FTS5 搜索） | ✅ 完成 |
| P0-5 | 打印与版本管理（打印预览 + 版本历史） | ✅ 完成 |
| P1-1 | 公式编辑器（LaTeX + MathJax） | ✅ 完成 |
| P1-2 | 数据导入（CSV 导入 + 预览） | ✅ 完成 |
| P1-3 | 标签管理（标签 CRUD + 报告标签关联） | ✅ 完成 |
| P1-4 | 附件管理（文件上传/下载/打开/删除） | ✅ 完成 |
| P2 | 协作/云同步/高级图表/统计面板 | ⏳ 待开发 |
| 发布 | 跨平台测试与正式发布 | ⏳ 待开发 |

## 代码统计

- **源文件数**：82 个（.h + .cpp）
- **代码行数**：约 20,000 行
- **注释覆盖率**：所有类、方法、关键代码段均有详细中文注释

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request。
