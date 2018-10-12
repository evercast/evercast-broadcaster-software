hr() {
  echo "───────────────────────────────────────────────────"
  echo $1
  echo "───────────────────────────────────────────────────"
}

# Exit if something fails
set -e

# Echo all commands before executing
set -v

# set CEF version for use later
# export CEF_BUILD_VERSION=3.3282.1726.gc8368c8

# needed for enabling "ibtool" which is used later during packaging
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer

# doesn't work in a repo
# git fetch --unshallow

# Leave obs-studio folder
cd ../

# Install Packages app so we can build a package later
# http://s.sudre.free.fr/Software/Packages/about.html
hr "Downloading Packages app"
wget --quiet --retry-connrefused --waitretry=1 https://s3-us-west-2.amazonaws.com/obs-nightly/Packages.pkg
sudo installer -pkg ./Packages.pkg -target /

brew update

#Base OBS Deps and ccache
# skip installing jack as it causes libcrypto conflict in obs-outputs
# skip installing ccache, enable if you want
brew install speexdsp swig #ccache #jack
# install qt@5.10.1
brew install https://raw.githubusercontent.com/Homebrew/homebrew-core/8d4d48f0bb552b7b107119aeef59f141ce1f72c3/Formula/qt.rb

# export PATH=/usr/local/opt/ccache/libexec:$PATH
# ccache -s || echo "CCache is not available."

# Fetch and untar prebuilt OBS deps that are compatible with older versions of OSX
hr "Downloading OBS deps"
wget --retry-connrefused --waitretry=1 https://s3-us-west-2.amazonaws.com/obs-nightly/osx-deps.tar.gz
tar -xf ./osx-deps.tar.gz -C /tmp

# Fetch vlc codebase
hr "Downloading VLC repo"
wget --retry-connrefused --waitretry=1 -O vlc-master.zip https://github.com/videolan/vlc/archive/master.zip
unzip -q ./vlc-master.zip

# Get sparkle
hr "Downloading Sparkle framework"
wget --retry-connrefused --waitretry=1 -O sparkle.tar.bz2 https://github.com/sparkle-project/Sparkle/releases/download/1.16.0/Sparkle-1.16.0.tar.bz2
mkdir ./sparkle
tar -xf ./sparkle.tar.bz2 -C ./sparkle
sudo cp -R ./sparkle/Sparkle.framework /Library/Frameworks/Sparkle.framework

# CEF Stuff
hr "Downloading CEF"
# wget --quiet --retry-connrefused --waitretry=1 https://obs-nightly.s3-us-west-2.amazonaws.com/cef_binary_${CEF_BUILD_VERSION}_macosx64.tar.bz2
# tar -xf ./cef_binary_${CEF_BUILD_VERSION}_macosx64.tar.bz2
# cd ./cef_binary_${CEF_BUILD_VERSION}_macosx64
# # remove a broken test
# sed -i '.orig' '/add_subdirectory(tests\/ceftests)/d' ./CMakeLists.txt
# mkdir build
# cd ./build
# cmake -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-std=c++11 -stdlib=libc++" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9 ..
# make -j4
# mkdir libcef_dll
# cd ../../
