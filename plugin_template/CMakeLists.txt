# ============================================================================
# 插件脚手架模板（新建插件时复制本目录）
#
# 使用步骤（对应 Qt Creator"新建动态库"向导的产物）：
#   1. 复制本目录到 <主工程>/plugins/<你的插件名>/   （或直接用 Qt Creator
#      新建 Qt 库工程后，把 MyPlugin.h/cpp 与下方 CMake 配置搬入）
#   2. 修改 add_library 的库名（即 <你的插件名>）
#   3. 修改 MyPlugin 类名、iid()、name() 等元信息
#   4. 在主工程 CMakeLists.txt 的"插件"小节添加：add_subdirectory(plugins/<你的插件名>)
#   5. 编译：插件 .dll 自动输出到 bin/plugins/，程序启动时 PluginManager 自动加载
#
# 独立构建（Qt Creator 单独打开插件工程）时：
#   - target_link_libraries 需要能找到 core_lib：
#     * include 目录指向主工程 src/（core/plugin/PluginInterface.h 所在）
#     * 链接主工程构建产物中的 core_lib 库（Windows: core_lib.dll + 导入库）
#   - 最简单的方式仍是放在主工程内 add_subdirectory，由主工程统一构建。
# ============================================================================

# 插件库：SHARED 动态库（把 "myplugin" 改为你的插件名）
add_library(myplugin SHARED
    MyPlugin.cpp
    MyPlugin.h
)

# 链接框架库 core_lib（提供 PluginInterface 等导出符号）
# 如需使用 Qt 模块，追加 Qt6::Widgets 等
target_link_libraries(myplugin PRIVATE core_lib)

# 输出到 exe 同级 plugins/ 目录（exe 输出在 ${CMAKE_BINARY_DIR}/bin）
set_target_properties(myplugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
)

# 插件需要 QObject 元对象编译（Q_PLUGIN_METADATA/Q_INTERFACES）
set_target_properties(myplugin PROPERTIES
    AUTOMOC ON
)
