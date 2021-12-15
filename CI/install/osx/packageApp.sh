rm -rf ./EBS.app

mkdir EBS.app
mkdir EBS.app/Contents
mkdir EBS.app/Contents/MacOS
mkdir EBS.app/Contents/PlugIns
mkdir EBS.app/Contents/Resources
mkdir EBS.app/Contents/Frameworks

BUILD_CONFIG=RELEASE
QT_VERSION=5.15.2_1
cp -r rundir/$BUILD_CONFIG/bin/ ./EBS.app/Contents/MacOS
cp -r rundir/$BUILD_CONFIG/data ./EBS.app/Contents/Resources
cp ../CI/install/osx/EBS.icns ./EBS.app/Contents/Resources
cp -r rundir/$BUILD_CONFIG/obs-plugins/ ./EBS.app/Contents/PlugIns
cp ../CI/install/osx/Info.plist ./EBS.app/Contents
cp ../CI/install/osx/entitlements.plist ./EBS.app/Contents

#NDI Plugin
cp $NDI_PATH/build/obs-ndi.so ./EBS.app/Contents/PlugIns
mkdir -p ./EBS.app/Contents/Resources/data/obs-plugins/obs-ndi
cp -r $NDI_PATH/data/locale ./EBS.app/Contents/Resources/data/obs-plugins/obs-ndi
install_name_tool -change @rpath/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/obs-ndi.so
install_name_tool -change @rpath/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/obs-ndi.so
install_name_tool -change @rpath/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/obs-ndi.so

rm -rf ./tmp-libs/
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/EBS -d ./tmp-libs/ -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/libobs.0.dylib -d ./tmp-libs/ -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/obs-ffmpeg-mux -d ./tmp-libs/ -p @executable_path/../Frameworks

mv -f ./tmp-libs/EBS ./EBS.app/Contents/MacOS/EBS
mv -f ./tmp-libs/libobs.0.dylib ./EBS.app/Contents/MacOS/libobs.0.dylib
mv -f ./tmp-libs/obs-ffmpeg-mux ./EBS.app/Contents/MacOS/obs-ffmpeg-mux

mv -f ./tmp-libs/* ./EBS.app/Contents/Frameworks/

chmod +x ./EBS.app/Contents/MacOS/EBS

#echo "Deploying QT..."
/usr/local/Cellar/qt@5/$QT_VERSION/bin/macdeployqt ./EBS.app

# put qt network in here becasuse streamdeck uses it
cp -R /usr/local/opt/qt5/lib/QtCore.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtCore.framework/Versions/5/QtCore

cp -R /usr/local/opt/qt5/lib/QtWidgets.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets

cp -R /usr/local/opt/qt5/lib/QtNetwork.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork

cp -R /usr/local/opt/qt5/lib/QtMacExtras.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras

cp -R /usr/local/opt/qt5/lib/QtGui.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Frameworks/QtGui.framework/Versions/5/QtGui

cp -R /usr/local/opt/qt5/lib/QtSvg.framework ./EBS.app/Contents/Frameworks
chmod +w ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg
install_name_tool -change /usr/local/Cellar/qt@5/$QT_VERSION/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg

install_name_tool -change /usr/local/opt/qt5/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt5/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt5/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt5/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./EBS.app/Contents/MacOs/ebs
install_name_tool -change /usr/local/opt/qt5/lib/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg ./EBS.app/Contents/MacOs/ebs

../CI/install/osx/fix-plugin.sh coreaudio-encoder.so
../CI/install/osx/fix-plugin.sh decklink-ouput-ui.so
../CI/install/osx/fix-plugin.sh frontend-tools.so

../CI/install/osx/fix-plugin.sh image-source.so

../CI/install/osx/fix-plugin.sh linux-jack.so
../CI/install/osx/fix-plugin.sh mac-avcapture.so
../CI/install/osx/fix-plugin.sh mac-capture.so
../CI/install/osx/fix-plugin.sh mac-decklink.so
../CI/install/osx/fix-plugin.sh mac-syphon.so
../CI/install/osx/fix-plugin.sh mac-vth264.so
../CI/install/osx/fix-plugin.sh obs-ffmpeg.so
../CI/install/osx/fix-plugin.sh obs-filters.so
../CI/install/osx/fix-plugin.sh obs-ndi.so
../CI/install/osx/fix-plugin.sh obs-outputs.so
../CI/install/osx/fix-plugin.sh obs-transitions.so
../CI/install/osx/fix-plugin.sh obs-vst.so
../CI/install/osx/fix-plugin.sh obs-x264.so

../CI/install/osx/fix-plugin.sh rtmp-services.so
../CI/install/osx/fix-plugin.sh text-freetype2.so
../CI/install/osx/fix-plugin.sh vlc-video.so
../CI/install/osx/fix-plugin.sh websocketclient.dylib
