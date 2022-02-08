FILE_NAME=$1
FULL_NAME=./EBS.app/Contents/PlugIns/$FILE_NAME

echo "Fixing Plugin $FILE_NAME: $FULL_NAME"

#install_name_tool -change $EBS_DEPS_QT_PATH/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui $FULL_NAME
#install_name_tool -change $EBS_DEPS_QT_PATH/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore $FULL_NAME
#install_name_tool -change $EBS_DEPS_QT_PATH/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets $FULL_NAME
#install_name_tool -change $EBS_DEPS_QT_PATH/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras $FULL_NAME
#install_name_tool -change $EBS_DEPS_QT_PATH/lib/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg $FULL_NAME

python3 ../CI/install/osx/libpack.py -f $FULL_NAME -d ./tmp-libs/ -p @executable_path/../Frameworks

mv -f ./tmp-libs/$FILE_NAME $FULL_NAME
mv -f ./tmp-libs/* ./EBS.app/Contents/Frameworks/
