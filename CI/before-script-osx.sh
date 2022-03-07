# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH
mkdir build
cd build
cmake \
-DENABLE_SCRIPTING=OFF \
-DDepsPath=$EBS_DEPS_PATH \
-DCMAKE_BUILD_TYPE=RELEASE \
-DCMAKE_INSTALL_PREFIX=/opt/ebs \
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.12 \
-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 \
-Dlibwebrtc_DIR=$EBS_LIBWEBRTC_PATH \
-DBUILD_BROWSER=false \
-DOBS_WEBRTC_VENDOR_NAME=Evercast \
-DOBS_VERSION_OVERRIDE=$EBS_VERSION \
-DOBS_BASE_VERSION=23.2.0 \
-DWEBRTC_VERSION=87.0.0 \
-DENABLE_VLC=ON \
-DQt5Core_DIR=$EBS_DEPS_QT_PATH/lib/cmake/Qt5Core \
-DQt5Widgets_DIR=$EBS_DEPS_QT_PATH/lib/cmake/Qt5Widgets \
-DQTDIR=$EBS_DEPS_QT_PATH \
..
