#!/bin/bash
set -e

# ==================================================
# AudioCut.framework 集成到 AudioConverter iOS 工程
# ==================================================

PROJECT_DIR="/Users/wxj/workspace/App/AudioConverter/iOS"
FRAMEWORK_SRC="/Users/wxj/workspace/HiVideo/MediaKit/refactor_build/ios/AudioCut/.build/Framework/AudioCut.framework"
FRAMEWORK_DST="${PROJECT_DIR}/Frameworks/AudioCut.framework"

echo "📦 1. 复制 AudioCut.framework..."
mkdir -p "${PROJECT_DIR}/Frameworks"
rm -rf "${FRAMEWORK_DST}"
cp -R "${FRAMEWORK_SRC}" "${FRAMEWORK_DST}"

echo "✅ Framework 已复制到: ${FRAMEWORK_DST}"

echo ""
echo "📋 ====================================================="
echo "📋 还需要在 Xcode 中手动配置以下步骤："
echo "📋 ====================================================="
echo ""
echo "📌 步骤 1: 打开 Xcode 项目"
echo "   open ${PROJECT_DIR}/AudioConverter.xcodeproj"
echo ""
echo "📌 步骤 2: 添加 Framework Search Paths"
echo "   在项目 TARGET > Build Settings 中搜索 \"Framework Search Paths\""
echo "   添加: \$(PROJECT_DIR)/Frameworks"
echo ""
echo "📌 步骤 3: 添加 Link Binary With Libraries"
echo "   在 TARGET > Build Phases > Link Binary With Libraries"
echo "   点击 + 号 > Add Other... > 选择 ${FRAMEWORK_DST}"
echo ""
echo "📌 步骤 4: 添加 Other Linker Flags"
echo "   Build Settings > 搜索 Other Linker Flags"
echo "   添加: -framework AudioCut"
echo ""
echo "📌 步骤 5: 创建 Bridging Header（如果直接使用 C++ API）"
echo "   由于 AudioCut.framework 是 C++ 库，Swift 中需要使用"
echo "   Objective-C 包装类或创建 Bridging Header"
echo ""
echo "===================================================="
