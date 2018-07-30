# Make sure ccache is found
# export PATH=/usr/local/opt/ccache/libexec:$PATH

# set CEF version for use later
# export CEF_BUILD_VERSION="3.3282.1726.gc8368c8"

sudo rm -rf build
mkdir build
cd build
cmake \
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.10 \
-DDepsPath=/tmp/obsdeps \
-DVLCPath=$PWD/../../vlc-master \
-DCMAKE_INSTALL_PREFIX=/opt/obs \
-DCMAKE_BUILD_TYPE=RelWithDebInfo ..