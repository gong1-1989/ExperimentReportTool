# ---------------------------------------------------------------------------
# 单元测试（Qt Test）
# 启用方式：CMake 配置时加 -DBUILD_TESTS=ON
# 运行方式：构建后执行 ctest，或直接运行各测试可执行文件
# ---------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Test)

enable_testing()

# 模型层单元测试（纯逻辑，不依赖数据库与 UI）
add_executable(tst_datatable tst_datatable.cpp)
target_link_libraries(tst_datatable PRIVATE core_lib Qt6::Test)
add_test(NAME tst_datatable COMMAND tst_datatable)

add_executable(tst_report tst_report.cpp)
target_link_libraries(tst_report PRIVATE core_lib Qt6::Test)
add_test(NAME tst_report COMMAND tst_report)

add_executable(tst_versiondiffer tst_versiondiffer.cpp)
target_link_libraries(tst_versiondiffer PRIVATE core_lib Qt6::Test)
add_test(NAME tst_versiondiffer COMMAND tst_versiondiffer)

# 权限矩阵（临时 SQLite 库，查库逻辑层）
add_executable(tst_permission tst_permission.cpp)
target_link_libraries(tst_permission PRIVATE core_lib Qt6::Test)
add_test(NAME tst_permission COMMAND tst_permission)

# 工作流状态机（临时 SQLite 库，含提交/退回/撤回/归档）
add_executable(tst_workflow tst_workflow.cpp)
target_link_libraries(tst_workflow PRIVATE core_lib Qt6::Test)
add_test(NAME tst_workflow COMMAND tst_workflow)
