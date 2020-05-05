rm -rf ./EBS.app

mkdir EBS.app
mkdir EBS.app/Contents
mkdir EBS.app/Contents/MacOS
mkdir EBS.app/Contents/PlugIns
mkdir EBS.app/Contents/Resources

BUILD_CONFIG=RELEASE
QT_VERSION=5.14.1
cp -r rundir/$BUILD_CONFIG/bin/ ./EBS.app/Contents/MacOS
cp -r rundir/$BUILD_CONFIG/data ./EBS.app/Contents/Resources
cp ../CI/install/osx/EBS.icns ./EBS.app/Contents/Resources
cp -r rundir/$BUILD_CONFIG/obs-plugins/ ./EBS.app/Contents/PlugIns
cp ../CI/install/osx/Info.plist ./EBS.app/Contents

#NDI Plugin
cp $NDI_PATH/build/obs-ndi.so ./EBS.app/Contents/PlugIns
mkdir -p ./EBS.app/Contents/Resources/data/obs-plugins/obs-ndi
cp -r $NDI_PATH/data/locale ./EBS.app/Contents/Resources/data/obs-plugins/obs-ndi
install_name_tool -change @rpath/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/obs-ndi.so
install_name_tool -change @rpath/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/obs-ndi.so
install_name_tool -change @rpath/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/obs-ndi.so
cp $NDI_SDK ./EBS.app/Contents/PlugIns

echo "Bundling app..."
../CI/install/osx/dylibBundler -b -cd -d ./EBS.app/Contents/Frameworks -p @executable_path/../Frameworks/ \
-s ./EBS.app/Contents/MacOS \
-s /usr/local/opt/mbedtls/lib/ \
-x ./EBS.app/Contents/PlugIns/coreaudio-encoder.so \
-x ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so \
-x ./EBS.app/Contents/PlugIns/frontend-tools.so \
-x ./EBS.app/Contents/PlugIns/image-source.so \
-x ./EBS.app/Contents/PlugIns/linux-jack.so \
-x ./EBS.app/Contents/PlugIns/mac-avcapture.so \
-x ./EBS.app/Contents/PlugIns/mac-capture.so \
-x ./EBS.app/Contents/PlugIns/mac-decklink.so \
-x ./EBS.app/Contents/PlugIns/mac-syphon.so \
-x ./EBS.app/Contents/PlugIns/mac-vth264.so \
-x ./EBS.app/Contents/PlugIns/obs-ffmpeg.so \
-x ./EBS.app/Contents/PlugIns/obs-filters.so \
-x ./EBS.app/Contents/PlugIns/obs-ndi.so \
-x ./EBS.app/Contents/PlugIns/obs-transitions.so \
-x ./EBS.app/Contents/PlugIns/obs-vst.so \
-x ./EBS.app/Contents/PlugIns/rtmp-services.so \
-x ./EBS.app/Contents/MacOS/ebs \
-x ./EBS.app/Contents/MacOS/obs-ffmpeg-mux \
-x ./EBS.app/Contents/PlugIns/obs-x264.so \
-x ./EBS.app/Contents/PlugIns/text-freetype2.so \
-x ./EBS.app/Contents/PlugIns/obs-libfdk.so
# -x ./EBS.app/Resources/data/obs-plugins/obs-ndi/bin/obs-ndi.so \

echo "Deploying QT..."
/usr/local/Cellar/qt/$QT_VERSION/bin/macdeployqt ./EBS.app

mv ./EBS.app/Contents/MacOS/libobs-opengl.so ./EBS.app/Contents/Frameworks

# put qt network in here becasuse streamdeck uses it
cp -r /usr/local/opt/qt/lib/QtCore.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtCore.framework/Versions/5/QtCore

cp -r /usr/local/opt/qt/lib/QtWidgets.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets

cp -r /usr/local/opt/qt/lib/QtNetwork.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork

cp -r /usr/local/opt/qt/lib/QtMacExtras.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras

cp -r /usr/local/opt/qt/lib/QtGui.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui

cp -r /usr/local/opt/qt/lib/QtSvg.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg
install_name_tool -change /usr/local/Cellar/qt/$QT_VERSION/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg

install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt/lib/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg ./EBS.app/Contents/MacOs/ebs

# decklink ui qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg ./EBS.app/Contents/PlugIns/decklink-ouput-ui.so

# frontend tools qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg ./EBS.app/Contents/PlugIns/frontend-tools.so

# vst qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./EBS.app/Contents/PlugIns/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg ./EBS.app/Contents/PlugIns/obs-vst.so

#obs ndi plugin
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/obs-ndi.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/obs-ndi.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/obs-ndi.so
