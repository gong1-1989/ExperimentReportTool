# 实验报告记录工具 (ExperimentReportTool)

基于 **Qt 6 + C++** 开发的桌面端实验报告记录工具，支持实验项目管理、模板化报告创建、结构化富文本编辑、实验数据表格与图表、多格式导出、元数据检索、打印、版本管理、公式编辑、数据导入、标签管理、附件管理、多用户协作等完整功能。

## 当前版本

**v9.19.3** — C1 阶段2 块3：MainWindow 对话框组拆分 MainWindowDialogs

---

## 迭代历史

### v9.x 多用户与优化阶段

| 版本 | 主要变更 |
|------|----------|
| **v9.19.3** | C1 阶段2 块3：MainWindow 1413 行对话框组拆分——新建 MainWindowDialogs（17 个对话框槽/649 行：新建项目/报告、导入、项目导出、编辑/删除项目与报告、模板/标签/密码/备份/恢复/设置、关于/检查更新/用户管理），Hooks 结构体依赖注入窗口能力（当前选中/刷新/状态栏/关闭编辑器窗口）；权限检查（canModifyReport/Project）按方案保留窗口槽；onOpenReport/onLogout/onFind/onPluginManager 因依赖窗口状态留窗口；MainWindow 933 行；清理 20 个无引用 include；验收补丁：Hooks 补成员时误删 refreshProjectTree 导致聚合初始化错位（编译错 refreshProjectTree 无成员/too many initializers），已补回并与 MainWindow.cpp 初始化顺序逐项对齐（11 成员 ↔ 11 lambda）；补丁2：报告编辑窗口「创建者」强制只读（加载后 setReadOnly(true)，保存不再从编辑框回写 author，创建者固定为创建时用户名）；补丁3：「创建者」输入框直接改为 QLabel 纯展示（ReportEditor.ui QLineEdit→QLabel，删除 setReadOnly 调用，视觉上不可编辑） |
| **v9.19.2** | C1 阶段2 块2：ReportEditorWindow 1098 行三组拆分——格式操作下沉 ReportEditor（setBold/setItalic/setUnderline/setFontSize/setTextColor/setList/setQuote + applyCharFormat/clearBlockHeading，窗口槽 1-3 行转发）；新建 ObjectInsertionController（表格/图片/图表/公式/分隔线插入 + 对象编辑 onObjectEdit 107 行）；新建 ReportExportController（导出/打印/打印预览/页面设置本体，保存前置保留窗口槽）；窗口保留保存组（saveReport/另存为/版本历史）、UI 组装、信号槽骨架 828 行；迁移后清理 6 个无引用 include |
| **v9.19.1** | C1 阶段2 块1：ReportEditor 4 个对象预览渲染（表格/图表/图片/公式，~180 行）提取为 ObjectPreviewRenderer 静态纯渲染类（入参仅 ContentBlock，不依赖编辑器实例）；refreshObjectPreview 收敛为一行调用；CMake GLOB_RECURSE 自动收录新源文件 |
| **v9.19.0** | C6 Repository 基类：新增 BaseRepository（非模板工具基类），db() 统一 7 仓库 69 处数据库连接获取；execChecked() 统一 SQL exec 失败检测与错误日志（22 处接入）；inPlaceholders/dedupeIds/chunkIds 统一批量 IN 查询占位符/去重/500 分块（3 处重复逻辑收敛）；rollback 事务、文件操作日志等特殊错误处理保留原样；子类保持静态业务方法（静态成员可被派生类访问）；验收补丁：main.cpp '数据库初始化成功' info 降为 debug（与 DatabaseManager 内部 DEBUG 记录同义），删除 MainWindow 与 Repository 层重复的项目/报告创建 LOG_INFO（各保留 Repository 层权威记录） |
| **v9.18.5** | N+1 定点清理 + 搜索防抖：元数据/全文搜索循环内每结果 getById 改直接复用查询行 mapToReport（一次搜索 N+1 次 SQL → 0）；匹配描述标签改批量 findReportTagsBatch（TagRepository 新增，返回完整标签含颜色）；结果项目名改批量 findNamesBatch（ProjectRepository 新增）；SearchResultDialog 结果列表标签批量一次 SQL；PropertyPanelHelper 创建者/修改者 2 次单查合并批量 1 次；搜索框输入防抖（QTimer 300ms，避免连续击键触发 SQL）；验收补丁：mapToReport 为 private，新增公开薄包装 ReportRepository::fromQueryRow 供搜索复用查询行 |
| **v9.18.4** | C4 导入异步：DataImportDialog 解析改 QtConcurrent 后台线程（QFutureWatcher 回主线程更新预览，setFuture 替换天然只响应最新任务，大 CSV 不卡 UI），CMake 增加 Concurrent 模块；C5/P7 列表局部刷新+虚拟化：报告列表改为 QTableView + 新建 ReportListModel（QAbstractTableModel，视图按需渲染可见行；id 序列一致时仅 dataChanged 局部通知；创建者/标签批量一次 SQL 消除 N+1；表头点击排序由模型 sort() 实现；删 QTableWidget 全量 item 重建与死代码；验收补丁：QTableView 自身无 selectionChanged 信号，选中变化改手动连接 QItemSelectionModel（消除 connectSlotsByName 警告）） |
| **v9.18.3** | 代码/性能优化速赢批：删除 MainWindow::refreshAll 死代码（无调用）；collectDocument 锚点扫描改单次正则收集（O(n×m) → O(n+m)，保存路径优化）；UserService 新增 batchDisplayNames 批量查询，报告列表创建者显示改为一次 SQL（消除跨刷新 N+1）；补：修复 ObjectRenderer.h 引用不存在的 ContentBlock.h（ContentBlock 实为 Report.h 内定义）、qHash 匿名命名空间遮蔽全局重载（字段哈希改 ::qHash 全局限定）、UserService 误调非静态 DatabaseManager::database（改 instance().database()） |
| **v9.18.2** | README 全面精简梳理（641 → 254 行）：迭代历史压缩为 9 行倒序（最近 3 版详细 + 阶段合并）；目录结构精简为 20 行；功能指南 18 节约减 50%；编译/存储/配置/开发约定压缩为要点式；去除旧块编辑器/18 插件/事件总线/插件多接口等已删残留；数据库迁移更新至 v6 |
| **v9.18.1** | 代码/性能优化第二批：双击信号改名 objectClicked→objectDoubleClicked；FTS5 可用状态补 Debug 日志；图表渲染 LRU 缓存（键含数据表id/更新时间/配置/宽高，数据或配置变化自动失效）；评估确认 Q_UNUSED 为 Qt 惯例、撤销栈默认 100 已限、魔法数值已用枚举 |
| **v9.18.0** | 代码/性能优化第一批：补 report_tags.tag_id 索引（按标签反向查报告提速）；图片预览缩略解码（QImageReader，超大图不再整张载入）；补 5 处裸写操作 exec 检查与错误日志；对象渲染合并为共享 ObjectRenderer（导出/打印共用一套逻辑，参数化差异，消除约 300 行重复） |
| **v9.17.54** | 日志分级治理：LOG_DEBUG 在 Release 编译为空零开销；运行时 --log-level= / 环境变量 ERT_LOG_LEVEL 调级；启动细节降 Debug，Info 仅留用户操作结果；密码明文不再写日志 |
| **v9.17.44~9.17.53** | 格式工具栏收尾：按钮 checked 高亮样式；行高范围/回显/Word 式应用修复（clearSelection→blockFormat 取选区终点块→下拉数值匹配 setCurrentIndex）；模板 v3 显式 20pt 标题、v4 零 h 标签（版本号检测重建）；对象对齐跟随编辑窗；清理调试日志 |
| **v9.17.0~9.17.43** | **连续文档+对象锚点重构（大版本）**：块模式→Word 式连续文档，表格/图表/图片/公式以对象锚点内嵌；旧块编辑器全删；锚点双保险显示（setResourceProvider+addResource）；模板带对象序列化；数据表保存/外键/命名/查找、标签下拉、保存标记、只读权限、打印间距/实际大小、标题统一、段落样式与代码块移除、字号/加粗/对齐回显链路修复等 40+ 项迭代 |
| **v9.16.x** | 全面 .ui 化（主窗口三栏/菜单/工具栏/状态栏）；撤销栈节流合并；字号下拉回显+可编辑；历史版本崩溃修复（ChartRenderer 双重释放）；Word 导出 base64 内嵌（不再生成 images 文件夹）；搜索 N+1 修复；FTS5 全文搜索启用；数据库迁移包事务防半迁移 |
| **v9.15.x** | 颜色收口 AppTheme；测试框架（12 用例）；deploy.ps1 发布脚本；arg 链合并尝试与回退（Qt6 多参限制）；UiHelper 参数顺序统一；菜单栏迁 .ui + AutoUic 修复 |
| **v9.0~9.14** | 多用户基础（登录/权限/用户管理/改密）；创建者字段；字数字段缓存；标签管理；PDF/Word 导出；插件体系精简与 IID 升级；UiHelper 统一提示（125 处收口）；Service 层日志与 Repository 直调收口；.ui 中文化与标准图标收口 |

### v8.x 全面优化阶段

- QSS 全局样式表统一外观；UiUtils/HtmlListWidget 通用组件；Service 层解耦 UI 与业务
- AppTheme/AppDimensions 魔法数字收口；设置窗口可视化（外观/数据/插件）

### v4.x ~ v7.x 插件架构重构（历史）

- 当时采用 18 个独立 .dll 插件（工具/导入/导出/编辑器块）+ core_lib 动态库
- 后续版本已精简为单一 PluginInterface + 示例插件（见 v9.10/v9.11 迭代）

### v1.x ~ v3.x 基础功能阶段（历史）

- P0 基础功能（项目管理/报告编辑/数据表/图表/导出/搜索/打印/版本）+ P1 高级功能（公式/导入/标签/附件）
- 五阶段重构、.ui 化、信号槽命名约定（on_<objectName>_<signalName>）、PDF 导出复用 PrintManager

---

## 功能使用指南

### 1. 登录与用户
- 默认账号：管理员 `admin/admin123`；普通用户 `test/123456`（首次登录强制改密）
- 工具 → 修改密码；用户管理（仅管理员）：增删改查、重置密码

### 2. 多用户协作
- 数据库放共享文件夹即可多人使用；普通用户可查看所有数据，只能修改自己创建的
- 项目树按用户名二级分类；未分配创建者的旧数据所有人可改（兼容）

### 3. 项目管理
- 左侧项目树：右键新建项目/子项目、重命名、删除；选择项目后右侧显示其报告

### 4. 报告创建与编辑
- 新建报告 → 选模板 → 自动打开编辑窗口；创建者自动填充不可改，修改者自动记录
- **连续文档模式（Word 式）**：正文直接输入；表格/图表/图片/公式/分割线/数据引用以对象锚点内嵌，**双击**打开编辑
- 格式工具栏：字号、加粗/斜体/下划线、颜色、对齐、行高（1.0~3.0）；无选中时作用于整段
- 标签下拉单选（默认"无标签"）；自动保存：3 秒防抖 + Ctrl+S

### 5. 数据表
- 插入表格对象 → 双击打开编辑器：动态增删行列、列属性（名称/类型/单位/必填/范围）、数据校验、CSV 导入导出

### 6. 图表
- 插入图表对象 → 双击配置：5 种图表（折线/柱状/饼图/散点/面积），可配标题、轴、图例、网格、主题

### 7. 公式
- 插入公式对象 → 双击打开：LaTeX 输入 + MathJax 实时预览，20 个常用公式模板

### 8. 导出
- 文件 → 导出：PDF（复用打印渲染）/ HTML（CSS+图片内嵌）/ Word / 纯文本（Markdown 风格）

### 9. 搜索
- 顶部搜索框回车：FTS5 全文检索（标题/内容/标签，不可用自动降级 LIKE）+ 状态筛选

### 10. 打印
- 文件 → 打印预览/打印/页面设置：A4/Letter、横竖屏、页边距

### 11. 版本管理
- 报告编辑窗口 → 工具 → 版本历史：保存命名版本、恢复、删除、内容预览

### 12. 标签管理
- 工具 → 标签管理：增删改 + 12 种颜色；报告顶部下拉单选；列表/属性面板显示

### 13. 附件管理
- 报告编辑窗口 → 工具 → 附件管理：批量上传（进度条）、下载、打开、删除

### 14. 数据导入
- 数据表编辑器 → 导入 CSV：自动检测分隔符/编码、预览前 20 行、追加/替换/新建三种模式

### 15. 模板管理
- 工具 → 模板管理器：新建/编辑（名称/分类/描述/内容）；新建报告时选用

### 16. 插件管理
- 帮助 → 插件管理：查看已加载插件；PluginManager 动态加载 .dll（示例 DemoPlugin）

### 17. 设置
- 工具 → 设置：外观（主题/字体/字号，重启生效）、数据（数据库路径）、颜色实时预览

### 18. 日志级别控制
- 默认：Release=`info`（Debug 日志编译期屏蔽，零开销）；Debug 构建=`debug` 全量输出
- 现场排查临时开全量（免重编译）：`--log-level=debug` 命令行参数，或环境变量 `ERT_LOG_LEVEL=debug`（优先级：命令行 > 环境变量）
- 可选级别：`debug/info/warning/error`；日志位于程序目录 `logs/`，按大小滚动

## 技术栈

| 类别 | 技术 | 版本要求 |
|------|------|----------|
| 框架 | Qt | 6.10+（开发环境）/ 6.2+（最低兼容） |
| 语言 | C++ | C++17 |
| 构建 | CMake | 3.16+ |
| 数据库 | SQLite | 3.40+ |
| 图表 | Qt Charts | 随 Qt |
| 打印 | Qt PrintSupport | 随 Qt |
| 公式渲染 | MathJax 3 | CDN（需网络） |

---

## 架构设计

### 分层架构

```
┌───────────────────────────────────────────┐
│  UI Layer (Qt Widgets)                     │  主窗口、报告编辑窗口、对话框、组件
├───────────────────────────────────────────┤
│  Service Layer                             │  Report/Project/Tag/DataTable/User Service
├───────────────────────────────────────────┤
│  Editor / Export / Print / Chart           │  报告编辑器、对象渲染（ObjectRenderer）、图表渲染
├───────────────────────────────────────────┤
│  Core Layer                                │  实体模型、插件接口、工具类（Logger/AppConfig）
├───────────────────────────────────────────┤
│  Data Layer                                │  DatabaseManager + Repository
└───────────────────────────────────────────┘
```

### 插件架构

- **core_lib**：核心框架动态库，包含全部基础功能
- **单一 PluginInterface 抽象基类** + PluginManager 动态加载；示例插件 plugins/demo（独立 CMakeLists.txt 编译为 .dll）
- 早期版本曾用 18 个独立插件，v9.10 起精简为单一基类

### 核心设计模式

- **仓储模式 (Repository Pattern)**：数据访问封装在 Repository 类
- **单例模式 (Singleton)**：DatabaseManager、Logger、UserSession、AppConfig
- **策略模式 (Strategy)**：对象渲染差异（导出/打印）通过 ObjectRenderer::Options 参数化
- **观察者模式 (Observer Pattern)**：Qt 信号槽驱动 UI 与数据联动

---

## 目录结构

```
ExperimentReportTool/
├── CMakeLists.txt                  # 根 CMake（GLOB 自动收集 src 源码）
├── README.md                       # 项目说明（唯一文档）
├── src/
│   ├── main.cpp                    # 程序入口
│   ├── app.qss + resources.qrc     # 全局样式表与资源
│   ├── core/                       # 核心层：模型（Project/Report/Template/DataTable/Tag/Attachment/User）、
│   │                               #   插件框架（PluginManager）、工具类（Logger/AppConfig/AppTheme/UserSession）
│   ├── data/                       # 数据层：DatabaseManager（建表/迁移/FTS5/索引/事务）+ 7 个 Repository
│   ├── service/                    # 业务服务层（Report/Project/Tag/DataTable/User 等）
│   ├── editor/                     # 报告编辑器：DocumentTextEdit（连续文档+对象锚点）、ReportEditor、
│   │                               #   AutoSaveManager、DataTableEditorDialog
│   ├── chart/                      # 图表：ChartRenderer、ChartConfigDialog
│   ├── export/                     # 导出：HtmlGenerator（HTML/Word）、ExportManager、ObjectRenderer（对象渲染共享）
│   ├── print/                      # 打印：PrintManager
│   ├── search/                     # 搜索服务（FTS5 全文检索）
│   ├── utils/                      # CSV 解析/导入工具
│   └── ui/                         # 界面：MainWindow、ReportEditorWindow、widgets（项目树/报告列表/属性面板）、
│                                   #   dialogs（登录/设置/模板/版本/标签/附件/公式/搜索结果等）
├── plugins/demo/                   # 示例插件（插件框架示例）
└── tests/                          # 单元测试（-DBUILD_TESTS=ON 启用）
```

---

## 编译方法

**前置要求**：Qt 6.2+（Core/Gui/Widgets/Sql/Charts/PrintSupport）、CMake 3.16+、C++17 编译器（GCC 9+/Clang 10+/MSVC 2019+/MinGW 11+）

- **Windows (MinGW)**：`cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\mingw_64 && mingw32-make -j`
- **Windows (MSVC)**：`cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2019_64 && cmake --build . --config Release`
- **Linux/macOS**：`cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.0/gcc_64 && make -j$(nproc)`

**注意**：新增 .cpp 无需改 CMakeLists（GLOB 自动收集）；新增 Q_OBJECT 类或改 .ui 后须删除 build 重跑 CMake；插件独立编译为 .dll 运行时从 plugins/ 加载。

## 数据库设计

### 数据表清单（10 张 + 1 张 FTS5 虚拟表）

| 表名 | 说明 |
|------|------|
| `users` | 用户表（用户名、密码哈希、角色、首次改密码标记） |
| `projects` | 实验项目（树状结构，parent_id 自关联） |
| `reports` | 实验报告（document+objects JSON、created_by、modified_by、version） |
| `report_versions` | 报告版本快照 |
| `templates` | 报告模板（结构含 objects） |
| `data_tables` | 实验数据表（columns/rows 存 JSON） |
| `tags` / `report_tags` | 标签 + 报告-标签多对多关联 |
| `attachments` | 附件 |
| `app_meta` | 应用元信息（数据库版本等） |
| `reports_fts` | FTS5 全文索引虚拟表（触发器同步，不可用时自动降级 LIKE） |

### 数据库版本迁移

- 当前版本：**6**，首次运行自动迁移，无需手动操作
- v1→v4：早期演进（tags/reports 扩展字段、users 表、word_count 缓存）
- v4→v5：重建 data_tables 去掉外键（全局数据表可保存）
- v5→v6：reports 表加 modified_by（修改者）

---

## 数据存储位置

```
程序目录/
├── data/experiment_reports.db   # SQLite 数据库
├── logs/                        # 日志（按大小滚动）
├── config.ini                   # 配置
├── plugins/                     # 插件 .dll
└── attachments/                 # 报告附件
```

多用户：将整个程序目录放共享文件夹，多人访问同一数据库文件。

## 配置文件说明

`config.ini` 位于程序目录，三节：`[Appearance]`（主题/字体/颜色）、`[Data]`（数据库路径，空则用程序目录 data/）、`[Window]`（尺寸）。均可通过"工具 → 设置"可视化修改，颜色实时预览。

## 开发约定

### 槽函数命名
- 槽函数必须遵循 `on_<objectName>_<signalName>` 命名约定，由 uic 自动连接，禁止手动 connect（示例：`on_m_saveBtn_clicked()`）

### 插件开发
- 继承 `PluginInterface` 抽象基类，`Q_PLUGIN_METADATA` 宏，独立 CMakeLists.txt 编译为 .dll

### 常量管理
- 禁止魔法数字/文字硬编码：外观 → AppTheme、尺寸 → AppDimensions、可配置项 → AppConfig（.ini）、全局常量 → AppConstants

## 许可证

MIT License

---

## 贡献

欢迎提交 Issue 和 Pull Request。
