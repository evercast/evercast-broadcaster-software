rm -rf ./EBS.app

mkdir EBS.app
mkdir EBS.app/Contents
mkdir EBS.app/Contents/MacOS
mkdir EBS.app/Contents/PlugIns
mkdir EBS.app/Contents/Resources
mkdir EBS.app/Contents/Frameworks

BUILD_CONFIG=RELEASE
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
#install_name_tool -change @rpath/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/PlugIns/obs-ndi.so
#install_name_tool -change @rpath/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/PlugIns/obs-ndi.so
#install_name_tool -change @rpath/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/PlugIns/obs-ndi.so

rm -rf ./tmp-libs/
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/EBS -d ./tmp-libs/ -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/libobs-frontend-api.dylib -d ./tmp-libs/ -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/libobs-opengl.so -d ./tmp-libs/ -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/libobs.0.dylib -d ./tmp-libs/ -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/libobsglad.0.dylib -d ./tmp-libs/ -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/MacOS/obs-ffmpeg-mux -d ./tmp-libs/ -p @executable_path/../Frameworks

rm ./EBS.app/Contents/MacOS/libobs-frontend-api.dylib
rm ./EBS.app/Contents/MacOS/libobs-opengl.so
rm ./EBS.app/Contents/MacOS/libobs.0.dylib
rm ./EBS.app/Contents/MacOS/libobsglad.0.dylib
rm ./EBS.app/Contents/MacOS/obs-ffmpeg-mux

mv -f ./tmp-libs/EBS ./EBS.app/Contents/MacOS/EBS

mv -f ./tmp-libs/* ./EBS.app/Contents/Frameworks/

cp -r $EBS_DEPS_PATH/lib/ ./EBS.app/Contents/Frameworks/

chmod +x ./EBS.app/Contents/MacOS/EBS

#echo "Deploying QT..."
$EBS_DEPS_QT_PATH/bin/macdeployqt ./EBS.app

python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/PlugIns/platforms/libqcocoa.dylib -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/PlugIns/printsupport/libcocoaprintersupport.dylib -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/PlugIns/iconengines/libqsvgicon.dylib -p @executable_path/../Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/PlugIns/styles/libqmacstyle.dylib -p @executable_path/../Frameworks

../CI/install/osx/fix-qt-component.sh QtCore
../CI/install/osx/fix-qt-component.sh QtWidgets
../CI/install/osx/fix-qt-component.sh QtNetwork
../CI/install/osx/fix-qt-component.sh QtMacExtras
../CI/install/osx/fix-qt-component.sh QtGui
../CI/install/osx/fix-qt-component.sh QtSvg

../CI/install/osx/fix-qt-component.sh QtDBus
../CI/install/osx/fix-qt-component.sh QtPrintSupport

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
