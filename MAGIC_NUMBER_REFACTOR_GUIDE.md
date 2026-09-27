# 魔法数字和魔法文字改造指南

## 概述

本项目已引入三层常量管理体系，消除代码中的魔法数字和魔法文字：

| 层级 | 文件 | 用途 | 修改方式 |
|------|------|------|---------|
| L1 | `AppTheme.h` | 视觉样式常量（颜色、字体、间距、圆角） | 改代码，重新编译 |
| L2 | `AppDimensions.h` | 几何尺寸和时间参数（窗口大小、延迟毫秒数） | 改代码，重新编译 |
| L3 | `AppConfig.h/.cpp` + `config.ini` | 用户可配置项（主题色、字体、自动保存间隔） | 改 .ini，运行时生效 |

## 已完成改造的文件

| 文件 | 改造内容 |
|------|---------|
| `MainWindow.cpp` | 面板样式、状态栏样式、搜索框宽度、属性面板宽度 |
| `ReportEditorWindow.cpp` | 窗口尺寸、状态栏样式、保存状态颜色 |
| `ReportListWidget.cpp` | 状态颜色（改用 AppTheme::statusColor）、搜索框宽度、按钮样式 |
| `VersionHistoryDialog.cpp` | 窗口尺寸、状态栏消息延迟 |
| `TemplateEditorDialog.cpp` | 窗口尺寸 |
| `TagManagerDialog.cpp` | 窗口尺寸 |
| `SearchResultDialog.cpp` | 窗口尺寸 |
| `FormulaEditorDialog.cpp` | 窗口尺寸、公式预览延迟 |
| `DataImportDialog.cpp` | 窗口尺寸 |
| `AttachmentManagerDialog.cpp` | 窗口尺寸 |
| `PrintManager.cpp` | 打印预览窗口尺寸 |
| `OtherBlockEditors.cpp` | 表格/图片/代码/图表块最小高度、复制按钮恢复延迟 |
| `BlockEditor.cpp` | 文本块最小高度 |
| `FormulaBlockEditor.cpp` | 公式块最小高度 |

## 待改造文件（优先级从高到低）

### P0：样式集中的文件

1. **`ReportEditor.cpp`** - 保存状态标签颜色、编辑器样式
2. **`TextBlockEditor.cpp`** - 文本编辑器样式、字体大小
3. **`SettingsDialog.cpp/.ui`** - 改用 AppConfig 读写配置

### P1：含定时器延迟的文件

4. **`AutoSaveManager.cpp`** - 自动保存间隔（2000ms）
5. **`LoginDialog.cpp`** - 错误提示清除延迟（3000ms）

### P2：含硬编码颜色的文件

6. **`SearchResultDialog.cpp`** - 搜索结果项的 HTML 样式（大量内联颜色和字体大小）
7. **`VersionHistoryDialog.cpp`** - 版本项的 HTML 样式
8. **`TagManagerDialog.cpp`** - 标签颜色显示

### P3：其他

9. **`ChartRenderer.cpp`** - 图表渲染尺寸、等待时间
10. **`DataTableEditorDialog.cpp`** - 窗口尺寸
11. **`ChartConfigDialog.cpp`** - 窗口尺寸

## 改造示例

### 示例1：替换颜色

**改造前：**
```cpp
label->setStyleSheet("color: #666; font-size: 12px; padding: 0 8px;");
```

**改造后：**
```cpp
#include "core/utils/AppTheme.h"

label->setStyleSheet(
    QString("color: %1; font-size: %2px; padding: 0 %3px;")
        .arg(AppTheme::Color::Gray666)
        .arg(AppTheme::FontSize::Small)
        .arg(AppTheme::Spacing::Normal));
```

### 示例2：替换状态颜色

**改造前：**
```cpp
QColor color;
switch (report->status()) {
case ReportStatus::Draft:     color = QColor("#888888"); break;
case ReportStatus::Submitted: color = QColor("#E6A23C"); break;
case ReportStatus::Reviewed:  color = QColor("#67C23A"); break;
}
item->setForeground(color);
```

**改造后：**
```cpp
#include "core/utils/AppTheme.h"

item->setForeground(AppTheme::statusColor(report->status()));
```

### 示例3：替换窗口尺寸

**改造前：**
```cpp
resize(900, 600);
```

**改造后：**
```cpp
#include "core/utils/AppDimensions.h"

resize(AppDimensions::Window::DialogLargeWidth,
       AppDimensions::Window::DialogLargeHeight);
```

### 示例4：替换定时器延迟

**改造前：**
```cpp
QTimer::singleShot(3000, this, &MyClass::clearMessage);
```

**改造后：**
```cpp
#include "core/utils/AppDimensions.h"

QTimer::singleShot(AppDimensions::Delay::StatusMessage,
                   this, &MyClass::clearMessage);
```

### 示例5：使用用户配置（可被 config.ini 覆盖）

**改造前：**
```cpp
resize(1400, 900);
QTimer::singleShot(30000, this, &AutoSaveManager::saveNow);
```

**改造后：**
```cpp
#include "core/utils/AppConfig.h"

AppConfig& config = AppConfig::instance();
resize(config.mainWindowWidth(), config.mainWindowHeight());
QTimer::singleShot(config.autoSaveInterval(), this, &AutoSaveManager::saveNow);
```

## 常量命名规范

### 颜色（AppTheme::Color）
- 语义命名：`Primary`、`Success`、`Warning`、`Danger`、`Info`
- 文本色：`TextPrimary`、`TextRegular`、`TextSecondary`、`TextPlaceholder`
- 灰色：`Gray333`、`Gray666`、`Gray999`、`GrayCCC`、`GrayDDD`、`GrayEEE`

### 字体大小（AppTheme::FontSize）
- 相对命名：`ExtraSmall(11)`、`Small(12)`、`Medium(13)`、`Normal(14)`、`Large(16)`、`ExtraLarge(18)`、`Huge(24)`、`Massive(48)`

### 间距（AppTheme::Spacing）
- 4px 基准阶梯：`Tiny(2)`、`Small(4)`、`Medium(6)`、`Normal(8)`、`Large(12)`、`ExtraLarge(16)`、`Huge(24)`、`Massive(32)`

### 定时器延迟（AppDimensions::Delay）
- 语义命名：`Immediate(0)`、`Fast(100)`、`Normal(300)`、`Medium(500)`、`AutoSave(2000)`、`StatusMessage(3000)`、`ButtonReset(1500)`

## config.ini 配置项说明

```ini
[Theme]
primaryColor=#4A90D9      ; 主色调
successColor=#67C23A      ; 成功色
warningColor=#E6A23C      ; 警告色
dangerColor=#F56C6C       ; 危险色
fontFamily=Microsoft YaHei ; 字体族
baseFontSize=12           ; 基础字体大小

[Window]
mainWidth=1400            ; 主窗口宽度
mainHeight=900            ; 主窗口高度
rememberWindowSize=true   ; 记住窗口大小

[Editor]
fontSize=14               ; 编辑器字体大小
autoSaveInterval=30000    ; 自动保存间隔（毫秒）
maxVersions=50            ; 最大版本数

[UI]
statusMessageDuration=3000 ; 状态栏消息显示时长
showStatusBar=true        ; 显示状态栏

[Export]
defaultFormat=pdf        ; 默认导出格式
defaultPath=             ; 默认导出路径（空=用户主目录）
```

## 后续优化方向

1. **全局样式表（.qss）**：将分散的 setStyleSheet 迁移到统一的 .qss 文件，在 main.cpp 中加载
2. **主题切换**：基于 AppConfig 实现亮色/暗色主题一键切换
3. **SettingsDialog 改造**：用 AppConfig 替代直接读写 QSettings
4. **窗口位置记忆**：利用 AppConfig 的 rememberWindowSize 配置项
