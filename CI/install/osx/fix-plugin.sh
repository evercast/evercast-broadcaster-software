FILE_NAME=$1
FULL_NAME=./EBS.app/Contents/PlugIns/$FILE_NAME

echo "################################################"
echo "## FIXING PLUGIN: $FILE_NAME: $FULL_NAME"
echo " "

python3 ../CI/install/osx/libpack.py -f $FULL_NAME -d ./tmp-libs/ -p @executable_path/../Frameworks

mv -f ./tmp-libs/$FILE_NAME $FULL_NAME
mv -f ./tmp-libs/* ./EBS.app/Contents/Frameworks/

echo " "
echo "################################################"
echo " "