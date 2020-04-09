# Make sure ccache is found
export PATH=/usr/local/opt/ccache/libexec:$PATH
mkdir build
cd build
cmake \
-DENABLE_SCRIPTING=OFF \
-DDepsPath=~/obsdeps \
-DCMAKE_BUILD_TYPE=RELEASE \
-DCMAKE_INSTALL_PREFIX=/opt/ebs \
-DVLCPath=~/vlc-3.0.4 \
-DQTDIR=/usr/local/Cellar/qt/5.14.1 \
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
-DOPENSSL_ROOT_DIR=/usr/local/opt/openssl@1.1 \
-Dlibwebrtc_DIR=~/libwebrtc/cmake \
-DBUILD_BROWSER=false \
-DOBS_WEBRTC_VENDOR_NAME=Evercast \
-DOBS_VERSION_OVERRIDE=2.5.0 \
..
