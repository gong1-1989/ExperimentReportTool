# ============================================================================
# 实验报告记录工具 - CMake 构建配置
#
# 简化架构（单库部署）：
#   core_lib.dll            - 核心动态库（框架 + 内置块编辑器/导入导出）
#   ExperimentReportTool.exe - 主程序（只含 main.cpp）
#
# 插件体系（独立 .dll 插件）：
#   PluginManager 启动时扫描 exe 同级 plugins/ 目录加载插件。
#   - plugins/demo/     独立 .dll 示例插件（演示加载→查询→调用链路）
#   - plugin_template/  插件脚手架模板（新建插件时复制该目录）
#   插件只需继承 core/plugin/PluginInterface 抽象接口即可被加载。
# ============================================================================

cmake_minimum_required(VERSION 3.16)

project(ExperimentReportTool
    VERSION 1.0.0
    DESCRIPTION "基于 Qt+C++ 的实验报告记录工具"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# 全局编译设置
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "构建类型" FORCE)
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# 查找 Qt
# ---------------------------------------------------------------------------
find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Sql Charts PrintSupport Concurrent)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    set(QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Charts Qt6::PrintSupport Qt6::Concurrent)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Sql Charts PrintSupport Concurrent)
    set(QT_VERSION_MAJOR 5)
    set(QT_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Sql Qt5::Charts Qt5::PrintSupport Qt5::Concurrent)
endif()

# ---------------------------------------------------------------------------
# 核心动态库 core_lib
# 包含 src/ 下除 main.cpp 之外的所有代码
# ---------------------------------------------------------------------------
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)
file(GLOB_RECURSE CORE_UI CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.ui")
file(GLOB_RECURSE CORE_QRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.qrc")

# 排除 main.cpp
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/src/main\\.cpp$")

# 排除已删除的旧块编辑器（v9.17.0 重构后文件已移除；
# Windows 工作副本可能残留旧文件，排除后即使残留也不参与编译）
list(FILTER CORE_SOURCES EXCLUDE REGEX
    ".*/src/editor/(BlockEditor|TextBlockEditor|TableBlockEditor|ChartBlockEditor|ImageBlockEditor|CodeBlockEditor|FormulaBlockEditor|OtherBlockEditors)\..*$")

add_library(core_lib SHARED ${CORE_SOURCES} ${CORE_UI} ${CORE_QRC})

target_include_directories(core_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(core_lib PUBLIC ${QT_LIBS})

# Windows 下自动导出所有符号
set_target_properties(core_lib PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    POSITION_INDEPENDENT_CODE ON
)

# ---------------------------------------------------------------------------
# 主程序可执行文件
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE core_lib)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/Info.plist
    )
endif()

# ---------------------------------------------------------------------------
# 编译警告
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(core_lib PRIVATE /W4 /utf-8)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
    target_compile_options(core_lib PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ---------------------------------------------------------------------------
# 编译定义
# ---------------------------------------------------------------------------
target_compile_definitions(core_lib PUBLIC
    CMAKE_APP_NAME="${PROJECT_NAME}"
    CMAKE_APP_VERSION="${PROJECT_VERSION}"
    QT_DEPRECATED_WARNINGS
)

# ---------------------------------------------------------------------------
# 安装规则
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME} core_lib
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

# ---------------------------------------------------------------------------
# 单元测试（tests/ 目录尚未建立，预留开关；建立后放入 tests/CMakeLists.txt）
# ---------------------------------------------------------------------------
option(BUILD_TESTS "构建单元测试" OFF)
if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# 插件（独立动态库）
# ---------------------------------------------------------------------------
# 示例插件：演示独立 .dll 插件的加载链路
add_subdirectory(plugins/demo)

# 以后新增插件时，在此添加：
# add_subdirectory(plugins/<你的插件目录>)

message(STATUS "==================================================")
message(STATUS "项目: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Qt 版本: Qt${QT_VERSION_MAJOR}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "架构: core_lib.dll + 主程序（单库部署）")
message(STATUS "插件: 独立 .dll 示例插件 plugins/demo（输出到 bin/plugins/）")
message(STATUS "插件模板: plugin_template/（新建插件时复制）")
message(STATUS "==================================================")
