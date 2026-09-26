# Qt 可视化界面重构说明

## 概述

本项目正在进行 UI 界面重构，将原来通过代码手动创建的界面改为使用 Qt Designer (.ui 文件) 可视化设计。

## 重构进度

### 已完成
- ✅ ProjectDialog — 项目编辑对话框（完整示例）

### 待完成
- ⏳ MainWindow — 主窗口
- ⏳ ReportEditorWindow — 报告编辑窗口
- ⏳ ReportEditor — 报告编辑器组件
- ⏳ DataTableEditorDialog — 数据表编辑器对话框
- ⏳ ChartConfigDialog — 图表配置对话框
- ⏳ DataImportDialog — 数据导入对话框
- ⏳ FormulaEditorDialog — 公式编辑对话框
- ⏳ SearchResultDialog — 搜索结果对话框
- ⏳ VersionHistoryDialog — 版本历史对话框
- ⏳ TagManagerDialog — 标签管理对话框
- ⏳ ReportTagDialog — 报告标签选择对话框
- ⏳ AttachmentManagerDialog — 附件管理对话框
- ⏳ TemplateEditorDialog — 模板编辑器对话框
- ⏳ ReportListWidget — 报告列表组件

## 重构标准模式

### 1. 创建 .ui 文件

使用 Qt Designer 或手动编写 XML 格式的 .ui 文件，放在对应源文件同目录下。

示例：`src/ui/dialogs/ProjectDialog.ui`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>ProjectDialog</class>
 <widget class="QDialog" name="ProjectDialog">
  <!-- 界面布局和控件定义 -->
 </widget>
 <resources/>
 <connections/>
</ui>
```

### 2. 修改头文件 (.h)

```cpp
// 前向声明 UI 类
namespace Ui {
class ProjectDialog;
}

class ProjectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectDialog(QWidget* parent = nullptr);
    ~ProjectDialog() override;  // 需要添加析构函数

private:
    Ui::ProjectDialog* ui;  // UI 界面对象
    // 其他成员变量...
};
```

### 3. 修改实现文件 (.cpp)

```cpp
#include "ProjectDialog.h"
#include "ui_ProjectDialog.h"  // 由 uic 自动生成

ProjectDialog::ProjectDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ProjectDialog)  // 创建 UI 对象
{
    ui->setupUi(this);  // 从 .ui 文件加载界面

    // 后续初始化代码...
    // 注意：原来的手动创建控件代码全部删除
    // 控件通过 ui->控件名 访问
}

ProjectDialog::~ProjectDialog()
{
    delete ui;  // 释放 UI 对象
}
```

### 4. CMake 配置

确保 CMakeLists.txt 中已启用 AUTOUIC：

```cmake
set(CMAKE_AUTOUIC ON)

# 自动收集 .ui 文件
file(GLOB_RECURSE UI_FILES
    "${CMAKE_SOURCE_DIR}/src/*.ui"
)

# 添加到可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${UI_FILES}
)
```

## 控件命名规范

.ui 文件中的控件命名与原代码中的成员变量名保持一致，例如：
- `m_nameEdit` — 项目名称输入框
- `m_typeEdit` — 项目类型输入框
- `m_statusCombo` — 状态下拉框
- `m_buttonBox` — 按钮组

这样可以最小化代码改动，只需要将 `m_nameEdit` 改为 `ui->m_nameEdit`。

## 注意事项

1. **析构函数**：使用 .ui 文件后必须添加析构函数来释放 ui 对象
2. **控件访问**：所有控件通过 `ui->控件名` 访问
3. **信号连接**：.ui 文件中可以定义信号连接，也可以在代码中手动连接
4. **样式设置**：简单的样式可以在 .ui 文件中设置，复杂的样式在代码中设置
5. **动态内容**：下拉框的 itemData 等动态数据需要在代码中设置

## 编译验证

重构后需要验证：
1. 代码能够正常编译
2. 界面显示与原来一致
3. 所有功能正常工作
4. 没有内存泄漏（ui 对象正确释放）
