#!/usr/bin/env bash

export QTDIR=/usr/local/Cellar/qt/5.11.1
./CI/before-script-osx.sh
cd build && make -j 8 && sudo make install && cd ..
echo "OBS installed in /opt/obs"