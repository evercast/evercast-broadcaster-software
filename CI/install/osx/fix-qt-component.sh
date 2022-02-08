NAME=$1

cp -R $EBS_DEPS_QT_PATH/lib/$NAME.framework ./EBS.app/Contents/Frameworks
python3 ../CI/install/osx/libpack.py -f ./EBS.app/Contents/Frameworks/$NAME.framework/Versions/5/$NAME -p @executable_path/../Frameworks
