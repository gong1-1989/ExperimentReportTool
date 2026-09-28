# 实验报告记录工具 (ExperimentReportTool)

基于 **Qt 6 + C++** 开发的桌面端实验报告记录工具，支持实验项目管理、模板化报告创建、结构化富文本编辑、实验数据表格与图表、多格式导出、元数据检索、打印、版本管理、公式编辑、数据导入、标签管理、附件管理、多用户协作等完整功能。

## 当前版本

**v9.15** — 深度优化版（测试框架、发布脚本、代码收口、DB 诊断）

---

## 迭代历史

### v9.x 多用户与优化阶段

| 版本 | 主要变更 |
|------|----------|
| v9.0 | 多用户基础：User 模型、用户表、登录验证、首次登录强制改密码、项目按用户二级分类 |
| v9.1 | 权限拦截（普通用户只能改自己的）、用户管理界面（仅管理员）、数据库路径可配置、报告列表显示创建者 |
| v9.1.1~9.1.5 | 数据库路径浏览按钮、选择文件夹模式、设置窗口颜色实时预览、字体配置修复 |
| v9.2 | 字数统计修复、新建报告/项目自动填充创建者、编辑器状态栏重叠修复 |
| v9.2.1~9.2.7 | 创建者显示修复、状态栏重复修复、界面重叠修复、toPlainText 优化 |
| v9.3 | 字数字段缓存方案（数据库 word_count 列，保存时统计，主界面直接读取） |
| v9.3.1 | 修复 hasColumn 编译错误 |
| v9.4 | "作者"改为"创建者"，去掉作者列，创建者自动填充且只读 |
| v9.4.1 | 保存时不强制覆盖创建者，从输入框读取 |
| v9.4.2 | 新建报告时创建者自动设置为当前用户（MainWindow 新建逻辑修复） |
| v9.4.3 | 恢复标签管理菜单（重构中被意外移除） |
| v9.4.4 | 报告列表按项目过滤（必须选择项目才显示对应报告） |
| v9.4.5 | 标签颜色保存修复（颜色组合框 data 值为空导致保存后变红色） |
| v9.5 | 增加用户修改密码功能、修复新建模板崩溃（QDialogButtonBox::Save 空指针） |
| v9.5.1 | 修复模板取消键和版本历史关闭键无反应（信号未连接） |
| v9.5.2 | 改为符合命名规则的槽函数（on_<objectName>_<signalName>），uic 自动连接 |
| v9.6 | 标签选择集成到报告编辑器（下拉单选、默认为无标签）、标签状态在主界面列表/属性面板体现 |
| v9.7 | 导出 PDF/Word 直接引用打印预览渲染路径，修复表格/图表/图片不显示 |
| v9.8 | 附件管理、版本历史等对话框槽函数命名规范化 |
| v9.9 | 搜索范围调整（仅名称/作者/标签/状态）、搜索结果界面布局优化 |
| v9.10 | 插件体系精简：只保留单一抽象基类 PluginInterface + PluginManager + 示例插件 |
| v9.11 | 插件 IID 升级 /2.0（修复旧 .dll ABI 残留导致 SIGSEGV） |
| v9.12 | 全面代码检查（40 项扫描）与 P0 修复 |
| v9.13 | 插件骨架（方案 B）：独立 .dll 示例插件 plugins/demo + plugin_template 脚手架 |
| v9.13.1 | 插件 IID /2.0 打包 |
| v9.13.2 | 插件骨架回退重打包（PluginSkeleton） |
| v9.14 | 全面优化：①新建 UiHelper 统一 UI 提示（125 处收口）②Service 层补日志 ③SettingsDialog connect 收口 on_ 槽 ④search/export/print 的 Repository 直调收口 Service（UI 层零直调）⑤TagRepository 删除统计死代码（count/usageCount，738→649 行）⑥AppTheme 新增 Chart 色板/Heading 磅值档位，颜色字体硬编码收口 ⑦主菜单 action 补 standardIcon ⑧ChangePasswordDialog/UserManagerDialog/ReportListWidget 补 .ui 文件（纯代码 UI → .ui 定义，connect 全部自动连接） |
| v9.14.1 | ChangePasswordDialog/UserManagerDialog/ReportListWidget 补 .ui 文件（纯代码 UI → .ui 定义，17 处 connect 全部改自动连接，删除 2 个死槽和 8 个 m_ 控件成员） |
| v9.14.2 | AttachmentService/DataTableService 补关键日志（Service 层日志全覆盖）、MainWindow/ReportEditorWindow 菜单 action 补 QStyle 标准图标（21 个）、ProjectTreeWidget 补 QStyle include |
| v9.14.3 | 主菜单 action 图标全量收口（26/27，仅 AboutQt 无图标）、修复 tail 误判导致的 Restore 图标遗漏 |
| v9.14.4 | 修复编译错误：①UiHelper 增加带按钮参数的 warning 重载（closeEvent 三态确认框误转修复）②SP_DialogLockButton（Qt6 已移除）改为 SP_DialogApplyButton ③全面扫描确认全部 SP_ 枚举为 Qt6 标准、QMessageBox 直用清零 |
| v9.14.5 | UI 全面审查：19 个 .ui 中文化核对（50+ 按钮/17 窗口标题全部中文）、按钮-槽映射核对（全部连接完备）、对话框 margin 统一规范（10→12、15→16，共 7 个对话框）、登录提示补 test/123456 账号 |
| v9.14.6 | 修复标准对话框按钮英文：①main.cpp 加载 Qt 中文翻译（qtbase_zh_CN.qm，多路径查找）使 QInputDialog/QFileDialog/QColorDialog 按钮中文化 ②UiHelper 全部方法改为自定义中文按钮（确定/取消/保存/放弃/是/否），不依赖翻译文件部署 ③导出项目 getItem 补 &ok 参数准确区分确定/取消 |
| v9.15 | P3 颜色收口（#ddd/#666/#999/#eee → AppTheme，4 处+补 include）；P4① 测试框架（tests/CMakeLists + DataTable/Report 模型单测 12 用例，-DBUILD_TESTS=ON 启用）；P4② 发布脚本 deploy.ps1（CMake 构建+windeployqt+翻译+插件+zip）；P4③ arg 链收口 59 处（.arg 多参数化）；P4④ Service 入口 Q_ASSERT 前置断言 7 处+QtGlobal include；P4⑤ DB 写失败统一日志（事务/commit/rollback 日志、25 处失败分支补 SQL 诊断、3 处事务+3 处 commit 返回值检查、AttachmentService 补日志） |
| v9.15.1 | 修复 v9.15 arg 链合并编译错误：Qt6 QString::arg 多参数版仅支持纯字符串参数，混合 int/QString 会编译失败——已回退 29 处混合类型合并为链式（含 Logger.h 4 处宏、DataTable/ExportManager/ReportEditor/ProjectRepository/HtmlGenerator 等），保留纯字符串多参（AppTheme 常量等合法用法） |
| v9.15.2 | 修复 v9.15.1 遗漏：AppTheme::Spacing/FontSize/Radius 为 int 常量（非字符串），含这些常量的多参 .arg 仍编译失败或语义错误（int+int 匹配 fieldWidth 重载）——已把合并文件中全部纯简单参数多参 .arg 拆回链式（56 处，涉及 10 文件），剩余 40 处多参全部为合法纯字符串/浮点格式化重载；P4③ arg 链合并整体放弃，恢复链式写法 |
| v9.15.3 | 运行期日志检查修复 4 项：①Logger.cpp 恢复 arg(levelStr, -8) 的 fieldWidth 语义（v9.15.2 拆回误伤，导致日志消息被 -8 顶掉、Argument missing 警告）②中文检测正则 \\u4e00 双反斜杠改单反斜杠（PCRE2 不识别 \\u 转义，正则无效——此 bug 正是此前'字数统计为 0'的根源，Report.cpp/ReportEditor.cpp 两处）③主工具栏补 setObjectName（消除 saveState '????????' 警告）④DatabaseManager::close() 改块作用域，确保 removeDatabase 前无活跃句柄（消除 connection still in use 警告） |
| v9.15.4 | 修复 UiHelper 提示弹窗标题/内容位置颠倒：UiHelper 签名定义为 (parent, message, title) 但全项目 134 处调用均按 (parent, title, message) 传参（标题在前），导致弹窗标题栏显示长正文、正文区只显示标题——已将 UiHelper（error/info/warning/五参 warning/confirm）与 BaseDialog（showInfo/showWarning/confirm）签名统一为 title 在前，内部实现与全部调用处自动匹配，零调用处改动 |
| v9.15.5 | MainWindow 优化（P4⑥ 落地，不拆文件）：①新建 PropertyPanelHelper（ui/widgets/）静态工具类，集中报告属性/项目属性/占位提示/标签 HTML 生成（reportHtml/projectHtml/emptyHtml/tagsHtml），内部解析状态（复用 Report::statusDisplayName）、项目名、创建者、标签 ②MainWindow::updatePropertyPanel 从 120 行精简到约 20 行（1570→约1470 行）③SearchService 状态 switch 复用 Report::statusDisplayName（消除重复） |
| v9.15.6 | 菜单栏迁移到 .ui：①MainWindow.ui 新增 menuBar（文件/编辑/视图/工具/帮助 5 菜单）+ 27 个 QAction 定义（text/shortcut/checkable/checked/enabled/statusTip 全部迁入 .ui）②删除 MainWindow.cpp createMenus()（约60行动态代码）③createActions() 改为从 ui->m_actionXxx 取指针，仅保留 icon 设置（QStyle::standardIcon 无法在 .ui 表达）④createToolBar() 改用 ui->mainToolBar（原 addToolBar 会与 .ui 工具栏重复创建） |
| v9.15.7 | 修复 v9.15.6 AutoUic 失败：MainWindow.ui 的 QMenuBar 内 5 个 QMenu 缺少 <addaction name="menuX"/> 引用（Qt 官方 uic 严格要求 QMenu 必须被 menuBar 引用，PyQt6 uic 宽松放过导致漏检）——已补 5 个 addaction，并统一 property 为多行标准格式（与项目其他 .ui 一致），PyQt6 uic 编译验证通过 |
| v9.15.8 | 继续排查 AutoUic 失败：将 menuBar/actions 的 <string> 内容全部改为单行无空白（与 Qt Designer 输出字节级一致，消除 uic 对 string 空白处理的可能歧义）；如仍失败请运行 uic.exe 获取具体错误行 |
| v9.15.9 | **修复 AutoUic 根因**：uic.exe 报 'line 343 Unexpected element actions'——<actions> 节必须位于 <widget>（QMainWindow）内部（</widget> 之前），此前误放在顶层。已把 27 个 <action> 移入 widget 内（statusbar 之后、</widget> 之前），与 Qt Designer 输出完全一致 |
| v9.16.0 | **全面 .ui 化改造**：①MainWindow 中央三栏布局（m_mainSplitter/项目树/报告列表/属性面板）全部定义到 .ui（含 ProjectTreeWidget/ReportListWidget 提升、customwidgets），setupUi() 由 80 行动态创建改为 5 行成员引用；②MainWindow 状态栏 3 个标签+弹簧与工具栏全局搜索框迁入 .ui；③ReportEditorWindow 全面迁 .ui：8 个成员 QAction + 22 个菜单动作 + 文件/编辑/插入/格式/视图 5 个菜单 + 工具栏（含标题下拉）+ 状态栏 4 个标签，删除动态 createMenus/createToolBar，动作连接集中到 createActions |
| v9.16.1 | **修复 v9.16.0 编译错误**：m_reportList 提升为 ReportListWidget 后残留的 4 个 QTableWidget 专属属性（editTriggers/selectionBehavior/selectionMode/alternatingRowColors）导致 uic 生成无效 setter 调用，已删除（ReportListWidget 继承 QWidget，属性由组件内部管理） |
| v9.16.2 | **修复主界面三栏变垂直堆叠**：删除 m_centerSplitter 时残留的 `<property name=orientation>Vertical</property>` 误留在 m_mainSplitter 内，覆盖了 Horizontal 方向导致项目树/报告列表/属性面板上下排列；已删除残留，uic 现仅生成一次 Horizontal |
| v9.16.3 | **修复状态栏控件重叠**：QStatusBar 的子 widget 写在 .ui 中时 uic 不会生成 addWidget 调用，控件全部堆叠重叠；已恢复 cpp 中 addWidget/addPermanentWidget 显式添加（MainWindow 3 标签+弹簧、ReportEditorWindow 4 标签）；另修复 11 处 UiHelper::confirm 参数顺序颠倒（message/title 传反导致弹窗标题=长正文）与 BaseDialog::showError 传反 |
| v9.16.3 | **修复弹窗标题/正文颠倒（文字重叠视觉）**：11 处 `UiHelper::confirm(this, 消息, 标题)` 参数传反（删除项目/报告/附件/标签/用户、恢复设置/版本、内置模板复制等确认框），标题被长消息占用；已全部改为 `(this, 标题, 消息)`。同时修正 BaseDialog::showError 封装（message/title 顺序与 UiHelper 一致） |

### v8.x 全面优化阶段

- **QSS 全局样式表**：app.qss + resources.qrc，统一外观管理
- **UiUtils/HtmlListWidget** 通用组件
- **OtherBlockEditors 拆分**：Table/Image/Code/Chart 四个独立文件
- **Service 层**：ReportService/ProjectService/TagService，解耦 UI 与业务逻辑
- **魔法数字优化**：AppTheme（颜色/字体/间距）+ AppDimensions（尺寸）+ AppConfig（.ini 配置）
- **设置窗口**：外观、数据、插件配置可视化，颜色实时预览

### v4.x ~ v7.x 插件架构重构

- **18 个独立 .dll 插件**：工具插件（搜索/标签/附件/版本/模板/插件管理）、导入插件（CSV）、导出插件（PDF/HTML/Word/Text）、编辑器块插件（文本/表格/图片/代码/公式/图表）
- **core_lib SHARED**：核心框架动态库
- **每个插件独立 CMakeLists.txt**
- **PluginInterface 纯虚类**，支持运行时动态加载
- **ExportManager 调用插件渲染**：blockToHtml 优先调用插件 renderToHtml()

### v1.x ~ v3.x 基础功能阶段

- P0 基础功能：项目管理、报告编辑、数据表、图表、导出、搜索、打印、版本
- P1 高级功能：公式编辑器、数据导入、标签管理、附件管理
- 五阶段重构：14+ .ui 文件、修复 20+ 编译错误
- 对话框信号槽统一：on_<objectName>_<signalName> 命名约定
- 标签功能集成：编辑器单选下拉列表、主界面列表和属性面板显示
- 搜索功能重写：只搜元数据（标题/作者/标签/状态）
- PDF 导出复用 PrintManager 渲染

---

## 功能使用指南

### 1. 登录与用户

- 启动后显示登录界面，输入用户名和密码
- 默认账号：
  - 管理员：`admin` / `admin123`
  - 普通用户：`test` / `123456`（首次登录必须修改密码）
- 工具 → 修改密码：修改当前用户密码
- 工具 → 用户管理（仅管理员可见）：增删改查用户、重置密码

### 2. 多用户协作

- 数据库可放在共享文件夹，多人同时使用
- 普通用户可查看所有人数据，但只能修改自己创建的
- 项目树按用户名二级分类，每个用户名下显示其创建的项目
- 未分配创建者的旧数据，所有人都可修改（兼容）

### 3. 项目管理

- 左侧项目树：右键新建项目/子项目、重命名、删除
- 支持树状项目结构（项目 → 子项目 → 报告）
- 必须选择具体项目，右侧才显示该项目下的报告

### 4. 报告创建与编辑

- 选择项目后 → 文件 → 新建报告（或工具栏按钮）
- 选择模板 → 输入标题 → 自动打开编辑窗口
- 创建者自动填充为当前用户显示名，不可修改
- 支持 14 种内容块：标题(H1-H3)、段落、无序列表、有序列表、引用、表格、图片、代码块、分割线、图表、公式、数据引用
- 标签选择：编辑器顶部下拉列表，单选，默认为"无标签"
- 快捷键：Enter 新建块、Backspace 删除空块、Alt+Up/Down 移动块
- 自动保存：3 秒防抖

### 5. 数据表

- 插入表格块 → 双击打开数据表编辑器
- 动态增删行列、设置列属性（名称/类型/单位/必填/数值范围）
- 数据校验、CSV 导入导出

### 6. 图表

- 插入图表块 → 点击「配置图表」选择数据表和图表类型
- 支持 5 种图表：折线图、柱状图、饼图、散点图、面积图
- 可配置标题、轴标题、图例、网格、数据点、主题
- 打印预览时等待 500ms 确保图表渲染完成

### 7. 公式

- 插入公式块 → 双击打开公式编辑器
- 输入 LaTeX 公式，右侧实时预览（MathJax）
- 20 个常用公式模板：分数、根号、求和、积分、矩阵、方程组等

### 8. 导出

- 文件 → 导出 → 选择格式（PDF/HTML/Word/纯文本）
- PDF：复用打印预览渲染，表格图表正常显示
- HTML：完整 CSS 样式，图片内嵌
- Word：Word 兼容格式（.doc）
- 纯文本：Markdown 风格

### 9. 搜索

- 主窗口顶部搜索框 → 输入关键词 → 回车
- 只搜索元数据：报告名称、作者、标签、状态
- 支持状态筛选下拉框

### 10. 打印

- 文件 → 打印预览 / 打印 / 页面设置
- 支持 A4/Letter 等纸张、横竖屏、页边距

### 11. 版本管理

- 报告编辑窗口 → 工具 → 版本历史
- 保存当前版本（可命名）、恢复历史版本、删除版本
- 版本内容预览

### 12. 标签管理

- 工具 → 标签管理：新建/编辑/删除标签，设置颜色（12 种预设颜色）
- 报告编辑器顶部下拉列表选择标签（单选）
- 主界面报告列表和属性面板显示标签名称和颜色

### 13. 附件管理

- 报告编辑窗口 → 工具 → 附件管理
- 上传文件（支持批量，进度条）、下载、打开、删除
- 文件名显示原始文件名

### 14. 数据导入

- 数据表编辑器 → 导入 CSV
- 自动检测分隔符和编码
- 数据预览（前 20 行）
- 三种导入模式：追加到现有数据、替换现有数据、创建新数据表

### 15. 模板管理

- 工具 → 模板管理器
- 新建/编辑模板，设置名称、分类、描述、内容块
- 新建报告时选择模板

### 16. 插件管理

- 帮助 → 插件管理
- 查看已加载插件列表、插件元信息
- 18 个独立 .dll 插件，可扩展

### 17. 设置

- 工具 → 设置
- 外观：主题颜色、字体、字号（修改后需重启生效）
- 数据：数据库路径（选择文件夹，程序自动创建数据库文件）
- 颜色配置实时预览

---

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
┌─────────────────────────────────────────┐
│  UI Layer (Qt Widgets)                   │  主窗口、编辑器、对话框、组件
├─────────────────────────────────────────┤
│  Service Layer                            │  ReportService/ProjectService/TagService
├─────────────────────────────────────────┤
│  Editor Layer                             │  块编辑器、报告编辑器、自动保存
├─────────────────────────────────────────┤
│  Plugin Layer                             │  18 个独立 .dll 插件
├─────────────────────────────────────────┤
│  Core Layer                               │  实体模型、插件接口、事件总线、工具类
├─────────────────────────────────────────┤
│  Data Layer                               │  数据库、仓储层
└─────────────────────────────────────────┘
```

### 插件架构

- **core_lib**：核心框架动态库，包含所有基础功能
- **18 个独立插件**（.dll）：
  - 工具插件：搜索、标签管理、附件管理、版本历史、模板管理、插件管理
  - 导入插件：CSV 导入
  - 导出插件：PDF、HTML、Word、纯文本
  - 编辑器块插件：文本、表格、图片、代码、公式、图表
- 每个插件独立 CMakeLists.txt，运行时动态加载
- PluginInterface 纯虚类，支持 qobject_cast 类型转换

### 核心设计模式

- **仓储模式 (Repository Pattern)**：数据访问封装在 Repository 类中
- **单例模式 (Singleton)**：DatabaseManager、Logger、UserSession、AppConfig
- **工厂模式 (Factory Pattern)**：BlockEditorFactory 根据块类型创建编辑器
- **插件模式 (Plugin Pattern)**：基于 Qt Plugin 系统的动态加载
- **观察者模式 (Observer Pattern)**：Qt 信号槽 + 事件总线

---

## 目录结构

```
ExperimentReportTool/
├── CMakeLists.txt                    # 根 CMake 配置
├── README.md                         # 项目说明（唯一文档）
├── src/
│   ├── main.cpp                      # 程序入口
│   ├── app.qrc                       # 资源文件（QSS 样式表）
│   │
│   ├── core/                         # 核心层
│   │   ├── models/                   # 实体模型
│   │   │   ├── Project.h/cpp             # 实验项目
│   │   │   ├── Report.h/cpp              # 实验报告（ContentBlock + JSON）
│   │   │   ├── Template.h/cpp            # 报告模板
│   │   │   ├── DataTable.h/cpp           # 实验数据表
│   │   │   ├── Tag.h/cpp                 # 标签
│   │   │   ├── Attachment.h/cpp          # 附件
│   │   │   └── User.h/cpp                # 用户
│   │   ├── plugin/                   # 插件框架
│   │   │   ├── PluginInterface.h         # 插件基类接口
│   │   │   ├── ToolPluginInterface.h     # 工具插件接口
│   │   │   ├── ImportPluginInterface.h   # 导入插件接口
│   │   │   ├── ExportPluginInterface.h   # 导出插件接口
│   │   │   ├── EditorBlockPluginInterface.h  # 编辑器块插件接口
│   │   │   ├── PluginManager.h/cpp       # 插件管理器
│   │   │   └── CoreServiceImpl.h/cpp     # 核心服务实现
│   │   ├── eventbus/                 # 事件总线
│   │   │   └── EventBus.h/cpp
│   │   └── utils/                    # 工具类
│   │       ├── AppConstants.h            # 全局常量
│   │       ├── AppConfig.h/cpp           # 配置管理（.ini）
│   │       ├── AppTheme.h                # 主题常量（颜色/字体/间距）
│   │       ├── AppDimensions.h           # 尺寸常量
│   │       ├── Logger.h/cpp              # 线程安全日志
│   │       ├── UserSession.h/cpp         # 当前用户会话
│   │       └── CsvParser.h/cpp           # CSV 解析器
│   │
│   ├── data/                         # 数据层
│   │   ├── database/
│   │   │   └── DatabaseManager.h/cpp     # 数据库管理器（建表/迁移）
│   │   └── repositories/             # 仓储层
│   │       ├── ProjectRepository.h/cpp
│   │       ├── ReportRepository.h/cpp
│   │       ├── TemplateRepository.h/cpp
│   │       ├── DataTableRepository.h/cpp
│   │       ├── TagRepository.h/cpp
│   │       ├── AttachmentRepository.h/cpp
│   │       └── UserRepository.h/cpp
│   │
│   ├── service/                      # 服务层
│   │   ├── ReportService.h/cpp
│   │   ├── ProjectService.h/cpp
│   │   └── TagService.h/cpp
│   │
│   ├── editor/                       # 编辑器层
│   │   ├── BlockEditor.h/cpp            # 块编辑器基类
│   │   ├── TextBlockEditor.h/cpp        # 文本块
│   │   ├── TableBlockEditor.h/cpp       # 表格块
│   │   ├── ImageBlockEditor.h/cpp       # 图片块
│   │   ├── CodeBlockEditor.h/cpp        # 代码块
│   │   ├── ChartBlockEditor.h/cpp       # 图表块
│   │   ├── FormulaBlockEditor.h/cpp     # 公式块
│   │   ├── ReportEditor.h/cpp            # 报告编辑器主组件
│   │   ├── AutoSaveManager.h/cpp         # 自动保存
│   │   └── ReportEditor.ui
│   │
│   ├── chart/                        # 图表模块
│   │   ├── ChartRenderer.h/cpp
│   │   └── ChartConfigDialog.h/cpp
│   │
│   ├── export/                       # 导出模块
│   │   └── ExportManager.h/cpp
│   │
│   ├── print/                        # 打印模块
│   │   └── PrintManager.h/cpp
│   │
│   ├── ui/                           # 界面层
│   │   ├── MainWindow.h/cpp             # 主窗口
│   │   ├── MainWindow.ui
│   │   ├── ReportEditorWindow.h/cpp     # 报告编辑独立窗口
│   │   ├── ReportEditorWindow.ui
│   │   ├── widgets/
│   │   │   ├── ProjectTreeWidget.h/cpp  # 项目树（按用户二级分类）
│   │   │   └── ReportListWidget.h/cpp   # 报告列表
│   │   └── dialogs/
│   │       ├── LoginDialog.h/cpp         # 登录对话框
│   │       ├── ChangePasswordDialog.h/cpp # 修改密码对话框
│   │       ├── UserManagerDialog.h/cpp   # 用户管理对话框
│   │       ├── ProjectDialog.h/cpp       # 项目编辑
│   │       ├── TagManagerDialog.h/cpp    # 标签管理
│   │       ├── AttachmentManagerDialog.h/cpp  # 附件管理
│   │       ├── VersionHistoryDialog.h/cpp # 版本历史
│   │       ├── TemplateEditorDialog.h/cpp # 模板编辑器
│   │       ├── SettingsDialog.h/cpp       # 设置对话框
│   │       ├── PluginManagerDialog.h/cpp  # 插件管理
│   │       ├── SearchResultDialog.h/cpp   # 搜索结果
│   │       └── FormulaEditorDialog.h/cpp  # 公式编辑器
│   │
│   └── plugins/                      # 插件（18 个独立 .dll）
│       ├── tools/                    # 工具插件
│       │   ├── search/
│       │   ├── tagmanager/
│       │   ├── attachmentmanager/
│       │   ├── versionmanager/
│       │   ├── templatemanager/
│       │   └── pluginmanager/
│       ├── import/                   # 导入插件
│       │   └── csvimport/
│       ├── export/                   # 导出插件
│       │   ├── pdfexport/
│       │   ├── htmlexport/
│       │   ├── wordexport/
│       │   └── textexport/
│       └── editor/                   # 编辑器块插件
│           ├── textblock/
│           ├── tableblock/
│           ├── imageblock/
│           ├── codeblock/
│           ├── formulablock/
│           └── chartblock/
│
├── resources/                        # 资源文件
│   └── styles/
│       └── app.qss                   # 全局样式表
│
└── tests/                            # 单元测试（预留）
```

---

## 编译方法

### 前置要求

- **Qt** 6.2+（必须包含：Core、Gui、Widgets、Sql、Charts、PrintSupport）
- **CMake** 3.16+
- **C++17** 兼容编译器：GCC 9+、Clang 10+、MSVC 2019+、MinGW 11+

### Windows (MinGW)

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\mingw_64
mingw32-make -j
```

### Windows (MSVC)

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2019_64
cmake --build . --config Release
```

### Linux / macOS

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.0/gcc_64
make -j$(nproc)
```

### 注意事项

- 新增 .cpp 文件无需手动修改 CMakeLists.txt（使用 file(GLOB_RECURSE) 自动收集）
- 新增 Q_OBJECT 类或修改 .ui 文件后，需删除 build 目录重新运行 CMake
- 插件编译为独立 .dll，运行时从 plugins/ 目录加载

---

## 数据库设计

### 数据表清单（11 张）

| 表名 | 说明 |
|------|------|
| `users` | 用户表（用户名、密码哈希、显示名、角色、首次改密码标记） |
| `projects` | 实验项目（树状结构，parent_id 自关联，created_by 创建者） |
| `reports` | 实验报告（内容 JSON + created_by + version + word_count） |
| `report_versions` | 报告版本快照 |
| `templates` | 报告模板 |
| `data_tables` | 实验数据表 |
| `tags` | 标签（名称、颜色） |
| `report_tags` | 报告-标签多对多关联 |
| `attachments` | 附件 |
| `app_meta` | 应用元信息（数据库版本等） |

### 数据库版本迁移

- 当前版本：**4**
- v1→v2：tags 表添加 description 和 created_at
- v2→v3：添加 users 表，projects/reports 表加 created_by 和 version
- v3→v4：reports 表加 word_count（字数统计缓存）
- 首次运行自动迁移，无需手动操作

---

## 数据存储位置

数据文件和配置文件放在**程序所在目录**：

```
程序目录/
├── data/
│   └── experiment_reports.db    # SQLite 数据库
├── logs/                         # 日志文件（按日期滚动）
├── config.ini                    # 配置文件
├── plugins/                      # 插件目录（.dll 文件）
└── attachments/                  # 报告附件存储
```

多用户使用时，将整个程序目录放在共享文件夹，多人同时访问同一数据库文件。

---

## 配置文件说明

`config.ini` 位于程序目录，包含：

```ini
[Appearance]
theme=default
fontFamily=Microsoft YaHei
baseFontSize=13
primaryColor=#4A90D9
successColor=#52c41a
warningColor=#faad14
dangerColor=#f5222d

[Data]
databasePath=              ; 为空则使用程序目录/data/
showStatusBar=true

[Window]
width=1200
height=800
```

可通过"工具 → 设置"可视化修改，颜色配置支持实时预览。

---

## 开发约定

### 槽函数命名

所有按钮/控件的槽函数必须遵循 `on_<objectName>_<signalName>` 命名约定，由 uic 自动连接，禁止手动 connect。

示例：
- `on_m_saveBtn_clicked()`
- `on_m_buttonBox_accepted()`
- `on_m_versionList_itemClicked(QListWidgetItem* item)`

### 插件开发

新增插件需继承对应接口（ToolPluginInterface/ImportPluginInterface/ExportPluginInterface/EditorBlockPluginInterface），使用 `Q_PLUGIN_METADATA` 宏，独立 CMakeLists.txt 编译为 .dll。

### 常量管理

- 魔法数字/文字禁止硬编码
- 外观常量 → AppTheme（颜色/字体/间距）
- 尺寸常量 → AppDimensions
- 可配置项 → AppConfig（.ini 文件）
- 全局常量 → AppConstants

---

## 许可证

MIT License

---

## 贡献

欢迎提交 Issue 和 Pull Request。
