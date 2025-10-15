export DeployQt_Path=/Users/lifengdy/Qt/6.8.3/macos/bin/macdeployqt
export Qt_Path=/Users/lifengdy/Qt/6.8.3/macos

function print() {
    local text="$1"
    local width=${2:-40}
    local textLength=${#text}
    local padingLength=$(((width - textLength) / 2))
    local padding=$(printf '%*s' $padingLength '')
    padding=${padding// /=}

    echo "${padding}${text}${padding}"
}

function createDmg()
{
    local creatDmg=${1}
    local sourcePath=${2}
    local dmgPath=${3}
    local iconPath=${4}
    local backgroundPath=${5}

    ${creatDmg} \
    --volname "inspectorTool" \
    --volicon "${iconPath}" \
    --background "${backgroundPath}" \
    --window-pos 200 120 \
    --window-size 800 400 \
    --hide-extension "inspectorTool.app" \
    --icon-size 150 \
    --icon "inspectorTool.app" 200 180 \
    --app-drop-link 600 180 \
    "${dmgPath}" \
    "${sourcePath}"
}

print "[ Initialize variables ]"
CurrentTime=$(date +"%Y%m%d%H%M%S")
RFCTime=$(date -R)
SourceDir=$(dirname "$(dirname "$(realpath "$0")")")
BuildDir="${SourceDir}/build_${CurrentTime}"

if [ -z "${DeployQt_Path}" ] || [ -z "${Qt_Path}" ];
then
  echo "Please set [DeployQt_Path] and [Qt_Path] before build on mac"
  exit 1
fi

Deployqt="${DeployQt_Path}"
QtPath="${Qt_Path}"
QtCMakePath="${QtPath}/lib/cmake"

# 2 Compile
print "[ Compile ]"

# 2.1 Compile inspectorTool-x86_64
print "[ Compile x86_64 ]"
x86_64BuildDir="${BuildDir}/build_x86_64"

cmake -S ${SourceDir} -B ${x86_64BuildDir} -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_PREFIX_PATH="${QtCMakePath}"
if [ $? -ne 0 ];
then
    echo "Build inspectorTool-x86_64 failed"
    exit 1
fi

cmake --build ${x86_64BuildDir} --target inspectorTool -j 8
if [ $? -ne 0 ];
then
    echo "Compile inspectorTool-x86_64 failed"
    exit 1
fi

# 2.2 Compile inspectorTool-arm
print "[ Compile arm64 ]"
arm64BuildDir="${BuildDir}/build_arm64"

cmake -S ${SourceDir} -B ${arm64BuildDir} -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_PREFIX_PATH="${QtCMakePath}"
if [ $? -ne 0 ];
then
    echo "Build inspectorTool-arm64 failed"
    exit 1
fi

cmake --build ${arm64BuildDir} --target inspectorTool -j 8
if [ $? -ne 0 ];
then
    echo "Compile inspectorTool-arm64 failed"
    exit 1
fi

print "[ Merge inspectorTool ]"
AppDir="${BuildDir}/inspectorTool.app"
ExeDir="${AppDir}/Contents/MacOS"
ExePath="${ExeDir}/inspectorTool"
x86_64ExePath="${x86_64BuildDir}/inspectorTool"
arm64ExePath="${arm64BuildDir}/inspectorTool"

mkdir -p ${ExeDir}
lipo -create ${x86_64ExePath} ${arm64ExePath} -output ${ExePath}

print "[ Obtain the dependencies of inspectorTool ]"
${Deployqt} ${AppDir} -qmldir=${SourceDir}/qml
if [ $? -ne 0 ];
then
    echo "Obtain the dependencies of inspectorTool failed"
    exit 1
fi

print "[ Copy resources ]"
ContentsDir="${AppDir}/Contents"
PlistPath=${SourceDir}/package/templates/Info.plist
cp -f ${PlistPath} ${ContentsDir}

ResourcesDir="${AppDir}/Contents/Resources"
cp -f ${SourceDir}/icons/appIcon.icns ${ResourcesDir}
cp -f ${SourceDir}/icons/pagIcon.icns ${ResourcesDir}
if [ -n "${DSAPublicKey}" ] && [ -f "${DSAPublicKey}" ];
then
    cp -f ${DSAPublicKey} ${ResourcesDir}
fi

# 5.2 Generate dmg
print "[ Generate dmg ]"
if [ -d "${BuildDir}/dmg_content" ];
then
    rm -rf "${BuildDir}/dmg_content"
fi
mkdir -p "${BuildDir}/dmg_content"
cp -R -P "${AppDir}" "${BuildDir}/dmg_content"

CreateDmgTool="${SourceDir}/tools/create-dmg/create-dmg"

createDmg "${CreateDmgTool}" "${BuildDir}/dmg_content" "${BuildDir}/inspectorTool.dmg" "${SourceDir}/icons/dmgIcon.icns" "${SourceDir}/icons/dmg-background.png"
if [  $? -eq 0 ];
then
    echo "create dmg success"
else
    echo "create dmg failed"
fi
