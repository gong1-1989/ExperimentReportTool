# ============================================================================
# SearchPlugin - 全文搜索插件
# 独立动态库插件，由主程序通过 QPluginLoader 动态加载
# ============================================================================

add_library(SearchPlugin SHARED
    SearchPlugin.h
    SearchPlugin.cpp
)

# 链接核心动态库（包含所有核心功能和 Qt 库）
target_link_libraries(SearchPlugin PRIVATE core_lib)

# 输出到 bin/plugins/ 目录
set_target_properties(SearchPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Windows 下 Debug/Release 都输出到同一目录
if(WIN32)
    set_target_properties(SearchPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/plugins
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/plugins
    )
endif()
