#!/usr/bin/env bash

export QTDIR=/usr/local/Cellar/qt/5.11.1
./CI/before-script-osx.sh
cd build && make -j 8 && cd ..
./CI/before-deploy-osx.sh
echo "Package created in ./nigthly"