# ============================================================================
# ExperimentReportTool 一键发布脚本（Windows / MinGW）
# 用法：
#   powershell -ExecutionPolicy Bypass -File deploy.ps1
# 可选参数：
#   -QtBin     Qt 的 bin 目录（默认自动探测 D:\Qt\ 下的最新 mingw 版本）
#   -BuildDir  构建输出目录（默认 build\Release）
# 功能：
#   1. CMake 配置 + 编译 Release
#   2. windeployqt 部署 Qt 运行库
#   3. 拷贝 Qt 中文翻译（qtbase_zh_CN.qm，保证标准对话框按钮为中文）
#   4. 拷贝插件动态库（plugins\*.dll）
#   5. 打包 zip
# ============================================================================

param(
    [string]$QtBin = "",
    [string]$BuildDir = "build\Release"
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot

Write-Host "== 实验报告记录工具 发布脚本 ==" -ForegroundColor Cyan

# ---- 1. 定位 Qt bin ----
if (-not $QtBin) {
    $candidates = Get-ChildItem "D:\Qt" -Directory -ErrorAction SilentlyContinue |
        Where-Object { Test-Path "$($_.FullName)\mingw_64\bin\qmake.exe" } |
        Sort-Object Name -Descending
    if ($candidates) {
        $QtBin = "$($candidates[0].FullName)\mingw_64\bin"
    } else {
        Write-Host "未找到 Qt 安装目录，请用 -QtBin 参数指定（如 D:\Qt\6.10.1\mingw_64\bin）" -ForegroundColor Red
        exit 1
    }
}
$QtRoot = Split-Path $QtBin -Parent
Write-Host "Qt: $QtRoot"

# ---- 2. CMake 配置 + 编译 Release ----
$BuildAbs = Join-Path $Root $BuildDir
Write-Host "构建目录: $BuildAbs"
cmake -S $Root -B $BuildAbs -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { Write-Host "CMake 配置失败" -ForegroundColor Red; exit 1 }
cmake --build $BuildAbs --config Release --parallel
if ($LASTEXITCODE -ne 0) { Write-Host "编译失败" -ForegroundColor Red; exit 1 }

# ---- 3. 收集产物 ----
$ExeDir = $BuildAbs
if (-not (Test-Path "$ExeDir\ExperimentReportTool.exe")) {
    # 多配置生成器（Ninja Multi-Config 等）可能把 exe 放子目录
    $ExeDir = (Get-ChildItem $BuildAbs -Recurse -Filter "ExperimentReportTool.exe" | Select-Object -First 1).DirectoryName
}
Write-Host "exe 目录: $ExeDir"

# ---- 4. windeployqt 部署运行库 ----
& "$QtBin\windeployqt.exe" "$ExeDir\ExperimentReportTool.exe"
if ($LASTEXITCODE -ne 0) { Write-Host "windeployqt 失败" -ForegroundColor Red; exit 1 }

# ---- 5. 拷贝中文翻译（标准对话框按钮中文化关键）----
$TransDir = Join-Path $QtRoot "translations"
if (Test-Path "$TransDir\qtbase_zh_CN.qm") {
    Copy-Item "$TransDir\qtbase_zh_CN.qm" $ExeDir -Force
    Write-Host "已拷贝 qtbase_zh_CN.qm" -ForegroundColor Green
} else {
    Write-Host "警告：未找到 qtbase_zh_CN.qm，标准对话框按钮可能显示英文" -ForegroundColor Yellow
}

# ---- 6. 拷贝插件动态库 ----
$PluginDirs = Get-ChildItem $BuildAbs -Recurse -Filter "*.dll" |
    Where-Object { $_.Name -ne "core_lib.dll" -and $_.Name -ne "ExperimentReportTool.exe" }
foreach ($dll in $PluginDirs) {
    $dest = Join-Path $ExeDir "plugins"
    New-Item -ItemType Directory -Path $dest -Force | Out-Null
    Copy-Item $dll.FullName "$dest\$($dll.Name)" -Force
    Write-Host "已拷贝插件: $($dll.Name)" -ForegroundColor Green
}

# ---- 7. 打包 zip ----
$ZipName = "ExperimentReportTool_Release_$(Get-Date -Format 'yyyyMMdd_HHmm').zip"
$ZipPath = Join-Path $Root $ZipName
Compress-Archive -Path "$ExeDir\*" -DestinationPath $ZipPath -Force
Write-Host "发布完成: $ZipPath" -ForegroundColor Green
Write-Host "提示：正式分发前请在未安装 Qt 的机器上验证运行。" -ForegroundColor Yellow
