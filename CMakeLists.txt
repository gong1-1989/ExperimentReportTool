# ============================================================================
# 实验报告记录工具 - CMake 构建配置
# 支持 Qt5 和 Qt6，优先查找 Qt6，回退到 Qt5
# ============================================================================

cmake_minimum_required(VERSION 3.16)

# 项目名称、版本、语言
project(ExperimentReportTool
    VERSION 1.0.0
    DESCRIPTION "基于 Qt+C++ 的实验报告记录工具"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# 全局编译设置
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)              # 使用 C++17 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # 强制要求 C++17
set(CMAKE_CXX_EXTENSIONS OFF)            # 不使用编译器扩展
set(CMAKE_AUTOMOC ON)                    # 自动运行 MOC（元对象编译器）
set(CMAKE_AUTORCC ON)                    # 自动处理资源文件 (.qrc)
set(CMAKE_AUTOUIC ON)                    # 自动处理 UI 文件 (.ui)

# 默认构建类型为 Release（如果未指定）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "构建类型" FORCE)
endif()

# 输出目录统一设置
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# 查找 Qt（优先 Qt6，回退 Qt5）
# ---------------------------------------------------------------------------
find_package(Qt6 QUIET COMPONENTS
    Core Gui Widgets Sql Charts PrintSupport
)

if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "找到 Qt6，版本: ${Qt6_VERSION}")
else()
    find_package(Qt5 REQUIRED COMPONENTS
        Core Gui Widgets Sql Charts PrintSupport
    )
    set(QT_VERSION_MAJOR 5)
    message(STATUS "找到 Qt5，版本: ${Qt5_VERSION}")
endif()

# ---------------------------------------------------------------------------
# 收集源文件
# 使用 GLOB_RECURSE 自动收集，但建议新增文件后重新运行 CMake
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)

file(GLOB_RECURSE UI_FILES
    "${CMAKE_SOURCE_DIR}/src/*.ui"
)

file(GLOB_RECURSE QRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.qrc"
)

# ---------------------------------------------------------------------------
# 可执行文件目标
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${UI_FILES}
    ${QRC_FILES}
)

# 头文件包含目录（让 #include 可以使用相对 src 的路径）
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# 链接 Qt 库
# ---------------------------------------------------------------------------
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Charts
        Qt6::PrintSupport
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Sql
        Qt5::Charts
        Qt5::PrintSupport
    )
endif()

# ---------------------------------------------------------------------------
# 平台特定设置
# ---------------------------------------------------------------------------
if(WIN32)
    # Windows：设置应用程序图标、入口子系统
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif(APPLE)
    # macOS：设置为 .app 包
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/Info.plist
    )
endif()

# ---------------------------------------------------------------------------
# 编译警告设置
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter    # 暂未使用的参数不警告（接口预留）
    )
endif()

# ---------------------------------------------------------------------------
# 编译定义
# ---------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CMAKE_APP_NAME="${PROJECT_NAME}"
    CMAKE_APP_VERSION="${PROJECT_VERSION}"
    QT_DEPRECATED_WARNINGS          # 启用 Qt 废弃 API 警告
)

# ---------------------------------------------------------------------------
# 安装规则（可选，用于 make install）
# ---------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# ---------------------------------------------------------------------------
# 单元测试（可选，tests 目录存在时启用）
# ---------------------------------------------------------------------------
option(BUILD_TESTS "构建单元测试" OFF)
if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "==================================================")
message(STATUS "项目: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Qt 版本: Qt${QT_VERSION_MAJOR}")
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "==================================================")
