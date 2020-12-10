# Exit if something fails
set -e

# Echo all commands before executing
set -v

#git fetch --unshallow

# Leave obs-studio folder
cd ../

#Base OBS Deps and ccache
brew install jack speexdsp ccache mbedtls clang-format freetype fdk-aac
#Install qt5.14
wget https://gist.githubusercontent.com/DDRBoxman/9c7a2b08933166f4b61ed9a44b242609/raw/ef4de6c587c6bd7f50210eccd5bd51ff08e6de13/qt.rb
brew install -s ./qt.rb
#Install swig
wget https://gist.githubusercontent.com/DDRBoxman/4cada55c51803a2f963fa40ce55c9d3e/raw/572c67e908bfbc1bcb8c476ea77ea3935133f5b5/swig.rb
brew install -s ./swig.rb
#cleanup custom rb files
rm ./qt.rb && rm ./swig.rb

# Install Packages app so we can build a package later
# http://s.sudre.free.fr/Software/Packages/about.html
wget --retry-connrefused --waitretry=1 https://s3-us-west-2.amazonaws.com/obs-nightly/Packages.pkg
sudo installer -pkg ./Packages.pkg -target /

#Base OBS Deps and ccache

export PATH=/usr/local/opt/ccache/libexec:$PATH
ccache -s || echo "CCache is not available."

# Fetch and untar prebuilt OBS deps that are compatible with older versions of OSX
wget --retry-connrefused --waitretry=1 https://s3-us-west-2.amazonaws.com/obs-nightly/osx-deps.tar.gz
tar -xzf ./osx-deps.tar.gz -C ~

# if you have your own libwebrtc already installed, comment the following paragraph out.
# Fetch libwebrtc 73 Community Edition
# wget --retry-connrefused --waitretry=1 https://libwebrtc-community-builds.s3.amazonaws.com/libWebRTC-73-mac.tar.gz
# tar -xf ./libWebRTC-73-mac.tar.gz -C /tmp

# Fetch vlc codebase
curl -L -O https://downloads.videolan.org/vlc/3.0.4/vlc-3.0.4.tar.xz
tar -xf vlc-3.0.4.tar.xz -C ~

