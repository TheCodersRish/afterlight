#!/usr/bin/env bash
# Package AFTERLIGHT for itch.io (Mac).
set -euo pipefail

PROJECT="/Users/rishandutia/Projects/buns/Afterlight/Afterlight.uproject"
ENGINE="/Users/Shared/Epic Games/UE_5.8"
UAT="$ENGINE/Engine/Build/BatchFiles/RunUAT.sh"
UBT="$ENGINE/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll"
DOTNET="$ENGINE/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64/dotnet"
DIST="/Users/rishandutia/Projects/buns/Afterlight/Dist/itch"
STAGE="/Users/rishandutia/Projects/buns/Afterlight/Saved/StagedBuilds/Mac"
MAPS="/Game/System/FrontEnd/Maps/L_LyraFrontEnd+/ShooterMaps/Maps/L_Expanse+/ShooterMaps/Maps/L_Expanse_Blockout+/ShooterCore/Maps/L_ShooterGym"

mkdir -p "$DIST"

echo "==> Build game binary"
"$ENGINE/Engine/Build/BatchFiles/Mac/Build.sh" LyraGame Mac Development -Project="$PROJECT" -WaitMutex || true
# If Xcode finalize flakes, regenerate receipt from linked binary
"$DOTNET" "$UBT" LyraGame Mac Development -Project="$PROJECT" -SkipBuild -WaitMutex

echo "==> Cook + stage + pak"
"$UAT" BuildCookRun \
  -project="$PROJECT" \
  -noP4 \
  -platform=Mac \
  -clientconfig=Development \
  -cook -skipbuild -stage -pak \
  -utf8output \
  -map="$MAPS"

echo "==> Assemble itch folder from staged app"
rm -rf "$DIST/Mac"
mkdir -p "$DIST/Mac"
ditto "$STAGE/LyraGame.app" "$DIST/Mac/Afterlight.app"
/usr/libexec/PlistBuddy -c "Set :CFBundleName AFTERLIGHT" "$DIST/Mac/Afterlight.app/Contents/Info.plist" || true
/usr/libexec/PlistBuddy -c "Set :CFBundleDisplayName AFTERLIGHT" "$DIST/Mac/Afterlight.app/Contents/Info.plist" 2>/dev/null \
  || /usr/libexec/PlistBuddy -c "Add :CFBundleDisplayName string AFTERLIGHT" "$DIST/Mac/Afterlight.app/Contents/Info.plist" || true
codesign --force --deep --sign - "$DIST/Mac/Afterlight.app"

cat > "$DIST/.itch.toml" <<'EOF'
[[actions]]
name = "play"
path = "Mac/Afterlight.app"
platform = "osx"
EOF

rm -f "$DIST/Afterlight-Mac.zip"
(
  cd "$DIST"
  ditto -c -k --sequesterRsrc --keepParent Mac "$DIST/Afterlight-Mac.zip"
)

echo "==> Done"
echo "Folder: $DIST/Mac/Afterlight.app"
echo "Zip:    $DIST/Afterlight-Mac.zip"
echo "Upload: butler push \"$DIST/Mac\" youruser/afterlight:osx"
