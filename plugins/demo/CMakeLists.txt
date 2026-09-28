# ============================================================================
# 示例插件工程（独立动态库）
#
# 演示独立 .dll 插件的构建方式：
# 1. 本目录由主工程 CMakeLists.txt 通过 add_subdirectory(plugins/demo) 引入
# 2. 编译产物 demo.dll 自动输出到 bin/plugins/（exe 同级 plugins/ 目录）
# 3. 程序启动时 PluginManager 扫描 plugins/ 目录并加载该插件
#
# 插件开发模板请参考 ../../plugin_template/ 目录
# ============================================================================

# 插件库：SHARED 动态库
add_library(demo SHARED
    DemoPlugin.cpp
    DemoPlugin.h
)

# 链接框架库 core_lib（提供 PluginInterface 等导出符号）
target_link_libraries(demo PRIVATE core_lib)

# 输出到 exe 同级 plugins/ 目录（exe 输出在 ${CMAKE_BINARY_DIR}/bin）
set_target_properties(demo PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins"
)
