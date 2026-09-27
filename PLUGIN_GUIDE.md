# 插件开发指南

## 概述

实验报告记录工具采用**框架 + 插件**架构，核心功能与业务功能分离。
新增功能只需开发一个插件，不需要修改核心代码。

## 架构图

```
┌─────────────────────────────────────────────────┐
│                   主程序 (Host)                   │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ MainWindow│  │ CoreService│  │ PluginManager │  │
│  └────┬─────┘  └────┬─────┘  └───────┬───────┘  │
│       │              │                │          │
│       └──────────────┼────────────────┘          │
│                      │                           │
│              ┌───────▼───────┐                   │
│              │  扩展点 (API)  │                   │
│              └───────┬───────┘                   │
└──────────────────────┼───────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
   ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
   │编辑器插件│   │导出插件  │   │工具插件  │
   └─────────┘   └─────────┘   └─────────┘
```

## 插件类型

### 1. 基础插件 (PluginInterface)

所有插件的根接口，提供基本的生命周期管理。

```cpp
class MyPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.example.MyPlugin")
    Q_INTERFACES(PluginInterface)

public:
    QString name() const override { return "My Plugin"; }
    QString iid() const override { return "com.example.MyPlugin"; }
    QString version() const override { return "1.0.0"; }
    QString description() const override { return "示例插件"; }

    bool initialize(CoreService* core) override {
        // 初始化逻辑
        return true;
    }

    void shutdown() override {
        // 清理逻辑
    }
};
```

### 2. 编辑器块插件 (EditorBlockPluginInterface)

用于扩展报告编辑器中的内容块类型（文本、表格、图表、公式等）。

```cpp
class ChartBlockPlugin : public QObject, public EditorBlockPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.example.ChartBlock")
    Q_INTERFACES(PluginInterface EditorBlockPluginInterface)

public:
    QString blockType() const override { return "chart"; }
    QString blockDisplayName() const override { return "图表"; }

    ContentBlock createDefaultBlock() const override {
        ContentBlock block(BlockType::Chart);
        return block;
    }

    BlockEditor* createEditor(const ContentBlock& block, QWidget* parent) override {
        return new ChartBlockEditor(block, parent);
    }

    QString renderToHtml(const ContentBlock& block) const override {
        // 渲染为 HTML
        return "<div>...</div>";
    }
};
```

### 3. 导出插件 (ExportPluginInterface)

用于扩展报告导出格式（PDF、HTML、Word、Markdown 等）。

```cpp
class MarkdownExportPlugin : public QObject, public ExportPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.example.MarkdownExport")
    Q_INTERFACES(PluginInterface ExportPluginInterface)

public:
    QString format() const override { return "md"; }
    QString formatName() const override { return "Markdown"; }
    QString fileExtension() const override { return "md"; }

    bool exportReport(const Report::Ptr& report,
                      const ExportConfig& config,
                      QWidget* parent) override {
        // 导出逻辑
        return true;
    }
};
```

## 核心服务 (CoreService)

插件通过 CoreService 访问主程序提供的核心服务：

```cpp
bool MyPlugin::initialize(CoreService* core) {
    // 数据库
    DatabaseManager* db = core->database();

    // 日志
    core->logger()->info("插件初始化");

    // 设置
    core->settings()->setValue("myplugin/enabled", true);

    // 插件专属设置
    QSettings* ps = core->pluginSettings(iid());
    ps->setValue("key", "value");
    ps->endGroup();

    // 事件总线
    core->eventBus()->publish("myplugin.event", data);

    // 目录
    QString dataDir = core->dataDirectory();
    QString pluginDir = core->pluginDirectory();

    return true;
}
```

## 事件总线 (EventBus)

插件之间通过事件总线解耦通信：

```cpp
// 发布事件
EventBus::instance().publish("report.saved", reportId);

// 订阅事件
EventBus::instance().subscribe("report.saved", this, SLOT(onReportSaved(QString,QVariant)));
```

## 插件部署

### 内置插件

直接在 main.cpp 中注册：

```cpp
pluginManager.registerBuiltinPlugin(new MyPlugin());
```

### 动态库插件

1. 将插件编译为动态库（.dll/.so/.dylib）
2. 放到以下任一目录：
   - 可执行文件同级的 `plugins/` 目录
   - 应用数据目录下的 `plugins/` 目录
3. 程序启动时自动扫描加载

## 插件元数据 JSON

可以通过 JSON 文件提供插件元数据：

```json
{
    "name": "My Plugin",
    "version": "1.0.0",
    "author": "Your Name",
    "description": "插件描述",
    "dependencies": []
}
```

在插件类中引用：

```cpp
Q_PLUGIN_METADATA(IID "com.example.MyPlugin" FILE "myplugin.json")
```

## 开发步骤

1. **创建插件类**：继承 QObject 和相应的插件接口
2. **声明元数据**：使用 Q_PLUGIN_METADATA 和 Q_INTERFACES 宏
3. **实现接口方法**：name()、iid()、initialize() 等
4. **实现业务逻辑**：在 initialize() 中注册扩展点
5. **测试**：作为内置插件注册，或编译为动态库加载

## 注意事项

- 插件 IID 必须全局唯一，建议使用反向域名格式
- 插件的 initialize() 中不要做耗时操作，以免影响启动速度
- 插件的 shutdown() 中必须清理所有资源
- 插件之间通过事件总线通信，不要直接依赖其他插件的类
- 依赖其他插件时，在 dependencies() 中声明，插件管理器会按依赖顺序加载
