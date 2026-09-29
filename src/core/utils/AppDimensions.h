/**
 * @file AppDimensions.h
 * @brief 应用尺寸常量
 *
 * 集中管理窗口尺寸、控件尺寸、定时器延迟等数字常量，避免魔法数字。
 * 所有常量为 constexpr，编译期求值，零运行时开销。
 *
 * 与 AppTheme 的区别：
 * - AppTheme：视觉样式（颜色、字体、间距、圆角）
 * - AppDimensions：几何尺寸和时间参数（窗口大小、控件大小、延迟毫秒数）
 */

#ifndef APP_DIMENSIONS_H
#define APP_DIMENSIONS_H

namespace AppDimensions {

// ===========================================================================
// 窗口默认尺寸
// ===========================================================================
namespace Window {
    // 主窗口
    constexpr int MainWidth  = 1400;
    constexpr int MainHeight = 900;

    // 报告编辑窗口
    constexpr int EditorWidth  = 1200;
    constexpr int EditorHeight = 800;

    // 对话框尺寸分级
    constexpr int DialogSmallWidth   = 600;
    constexpr int DialogSmallHeight  = 500;
    constexpr int DialogMediumWidth  = 800;
    constexpr int DialogMediumHeight = 600;
    constexpr int DialogLargeWidth   = 1000;
    constexpr int DialogLargeHeight  = 700;
    constexpr int DialogXLargeWidth  = 1200;
    constexpr int DialogXLargeHeight = 800;

    // 打印预览
    constexpr int PrintPreviewWidth  = 1000;
    constexpr int PrintPreviewHeight = 700;
}

// ===========================================================================
// 控件尺寸
// ===========================================================================
namespace Widget {
    // 面板
    constexpr int PropertyPanelMinWidth = 220;  ///< 属性面板最小宽度
    constexpr int ProjectPanelMinWidth  = 200;  ///< 项目面板最小宽度

    // 搜索框
    constexpr int GlobalSearchMaxWidth = 280;  ///< 主界面全局搜索框最大宽度
    constexpr int ReportSearchMaxWidth = 250;  ///< 报告列表搜索框最大宽度

    // 编辑器块最小高度
    constexpr int BlockMinHeight     = 32;   ///< 文本块最小高度
    constexpr int TableMinHeight     = 150;  ///< 表格块最小高度
    constexpr int ImageMinHeight     = 200;  ///< 图片块最小高度
    constexpr int ChartMinHeight     = 350;  ///< 图表块最小高度
    constexpr int FormulaMinHeight   = 60;   ///< 公式块最小高度
    constexpr int PlaceholderHeight  = 200;  ///< 空占位高度

    // 按钮
    constexpr int ButtonIconSize = 18;  ///< 工具栏图标尺寸
}

// ===========================================================================
// 定时器延迟（毫秒）
// ===========================================================================
namespace Delay {
    constexpr int Immediate     = 0;     ///< 下一事件循环立即执行（确保布局完成）
    constexpr int Fast          = 100;   ///< 快速响应（公式预览防抖）
    constexpr int Normal        = 300;   ///< 普通延迟（搜索防抖）
    constexpr int Medium        = 500;   ///< 中等延迟（图表渲染等待）
    constexpr int AutoSave      = 2000;  ///< 自动保存触发延迟
    constexpr int StatusMessage = 3000;  ///< 状态栏消息显示时长
    constexpr int ButtonReset   = 1500;  ///< 按钮文字恢复（如"复制"→"已复制"）
    constexpr int ErrorClear    = 3000;  ///< 错误提示自动清除
}

// ===========================================================================
// 自动保存
// ===========================================================================
namespace AutoSave {
    constexpr int IntervalMs  = 30000;  ///< 自动保存间隔（30秒）
    constexpr int MaxVersions = 50;     ///< 最大版本数
}

// ===========================================================================
// 数据库
// ===========================================================================
namespace Database {
    constexpr int Version = 2;  ///< 数据库版本号
}

// ===========================================================================
// 图表渲染
// ===========================================================================
namespace Chart {
    constexpr int RenderWaitMs = 500;  ///< 图表渲染等待时间（确保折线绘制完成）
    constexpr int DefaultWidth  = 800;  ///< 默认渲染宽度
    constexpr int DefaultHeight = 500;  ///< 默认渲染高度
}

// ===========================================================================
// 导出
// ===========================================================================
namespace Export {
    constexpr int PdfDpi = 300;  ///< PDF 导出 DPI
}

} // namespace AppDimensions

#endif // APP_DIMENSIONS_H
