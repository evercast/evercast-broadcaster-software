hr() {
  echo "───────────────────────────────────────────────────"
  echo $1
  echo "───────────────────────────────────────────────────"
}

# Exit if something fails
set -e

# Generate file name variables
export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H:%M:%S)
export FILENAME=$FILE_DATE-$GIT_HASH-$TRAVIS_BRANCH-osx.pkg

cd ./build

# Package everything into a nice .app
hr "Packaging .app"
STABLE=false
if [ -n "${TRAVIS_TAG}" ]; then
  STABLE=true
fi

# sudo python ../CI/install/osx/build_app.py --public-key ../CI/install/osx/OBSPublicDSAKey.pem --sparkle-framework ../../sparkle/Sparkle.framework --stable=$STABLE

../CI/install/osx/packageApp.sh

# fix obs outputs
#hr "Fixing OBS outputs"
#cp /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib ./EBS.app/Contents/Frameworks/
#
#cp /usr/local/lib/libcurl.4.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/nghttp2/lib/libnghttp2.14.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/libidn2/lib/libidn2.0.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/rtmpdump/lib/librtmp.1.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/zstd/lib/libzstd.1.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/brotli/lib/libbrotlidec.1.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/brotli/lib/libbrotlicommon.1.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/gettext/lib/libintl.8.dylib ./EBS.app/Contents/Frameworks/
#cp /usr/local/opt/libunistring/lib/libunistring.2.dylib ./EBS.app/Contents/Frameworks/
#
#install_name_tool -change /usr/local/opt/nghttp2/lib/libnghttp2.14.dylib @executable_path/../Frameworks/libnghttp2.14.dylib ./EBS.app/Contents/Frameworks/libcurl.4.dylib
#install_name_tool -change /usr/local/opt/libidn2/lib/libidn2.0.dylib @executable_path/../Frameworks/libidn2.0.dylib ./EBS.app/Contents/Frameworks/libcurl.4.dylib
#install_name_tool -change /usr/local/opt/rtmpdump/lib/librtmp.1.dylib @executable_path/../Frameworks/librtmp.1.dylib ./EBS.app/Contents/Frameworks/libcurl.4.dylib
#install_name_tool -change /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib @executable_path/../Frameworks/libssl.1.1.dylib ./EBS.app/Contents/Frameworks/libcurl.4.dylib
#install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib ./EBS.app/Contents/Frameworks/libcurl.4.dylib
#install_name_tool -change /usr/local/opt/zstd/lib/libzstd.1.dylib @executable_path/../Frameworks/libzstd.1.dylib ./EBS.app/Contents/Frameworks/libcurl.4.dylib
#install_name_tool -change /usr/local/opt/brotli/lib/libbrotlidec.1.dylib @executable_path/../Frameworks/libbrotlidec.1.dylib ./EBS.app/Contents/Frameworks/libcurl.4.dylib
#
#install_name_tool -change /usr/local/opt/gettext/lib/libintl.8.dylib @executable_path/../Frameworks/libintl.8.dylib ./EBS.app/Contents/Frameworks/libidn2.0.dylib
#install_name_tool -change /usr/local/opt/libunistring/lib/libunistring.2.dylib @executable_path/../Frameworks/libunistring.2.dylib ./EBS.app/Contents/Frameworks/libidn2.0.dylib
#
#install_name_tool -change /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib @executable_path/../Frameworks/libssl.1.1.dylib ./EBS.app/Contents/Frameworks/librtmp.1.dylib
#install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib ./EBS.app/Contents/Frameworks/librtmp.1.dylib
#
#install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib @executable_path/../Frameworks/libmbedtls.12.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib @executable_path/../Frameworks/libmbedcrypto.3.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib @executable_path/../Frameworks/libmbedx509.0.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/openssl@1.1/lib/libssl.1.1.dylib @executable_path/../Frameworks/libssl.1.1.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/openssl@1.1/lib/libcrypto.1.1.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/../Frameworks/libcurl.4.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/lib/libcurl.4.dylib @executable_path/../Frameworks/libcurl.4.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change @rpath/libobs.0.dylib @executable_path/../Frameworks/libobs.0.dylib ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/qt5/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/qt5/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/qt5/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/qt5/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./EBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /usr/local/opt/qt5/lib/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg ./EBS.app/Contents/Plugins/obs-outputs.so

cp ../CI/install/osx/OBSPublicDSAKey.pem EBS.app/Contents/Resources

# edit plist
plutil -insert CFBundleVersion -string $EBS_VERSION ./EBS.app/Contents/Info.plist
plutil -insert CFBundleShortVersionString -string $EBS_VERSION ./EBS.app/Contents/Info.plist
plutil -insert SUPublicDSAKeyFile -string OBSPublicDSAKey.pem ./EBS.app/Contents/Info.plist

dmgbuild "EBS" ebs.dmg

# Package app
hr "Generating .pkg"
packagesbuild ../CI/install/osx/CMakeLists.pkgproj

# Move to the folder that travis uses to upload artifacts from
hr "Moving package to nightly folder for distribution"
mkdir -p ./nightly
sudo mv EBS.pkg ./nightly/$FILENAME
