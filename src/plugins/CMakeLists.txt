# ============================================================================
# 插件目录总 CMakeLists.txt
# 通过 add_subdirectory 包含每个独立插件
# ============================================================================

# 编辑器块插件（6个）
add_subdirectory(editor/textblock)
add_subdirectory(editor/tableblock)
add_subdirectory(editor/chartblock)
add_subdirectory(editor/formulablock)
add_subdirectory(editor/imageblock)
add_subdirectory(editor/codeblock)

# 导出插件（4个）
add_subdirectory(export/pdfexport)
add_subdirectory(export/htmlexport)
add_subdirectory(export/wordexport)
add_subdirectory(export/textexport)

# 工具插件（6个）
add_subdirectory(tools/search)
add_subdirectory(tools/tagmanager)
add_subdirectory(tools/attachmentmanager)
add_subdirectory(tools/versionmanager)
add_subdirectory(tools/templatemanager)
add_subdirectory(tools/pluginmanager)

# 导入插件（1个）
add_subdirectory(import/csvimport)

# 示例插件（1个）
add_subdirectory(sample/sampleplugin)
