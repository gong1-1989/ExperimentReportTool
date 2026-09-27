# ============================================================================
# TextExportPlugin - 纯文本导出插件
# 独立动态库插件，由主程序通过 QPluginLoader 动态加载
# ============================================================================

add_library(TextExportPlugin SHARED
    TextExportPlugin.h
    TextExportPlugin.cpp
)

# 链接核心动态库（包含所有核心功能和 Qt 库）
target_link_libraries(TextExportPlugin PRIVATE core_lib)

# 输出到 bin/plugins/ 目录
set_target_properties(TextExportPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/plugins
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Windows 下 Debug/Release 都输出到同一目录
if(WIN32)
    set_target_properties(TextExportPlugin PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/plugins
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/plugins
    )
endif()
