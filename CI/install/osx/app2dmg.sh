echo "EBS_VERSION=$EBS_VERSION"
echo "NDI_RUNTIME=$NDI_RUNTIME"
echo "NDI_M1_RUNTIME=$NDI_M1_RUNTIME"
echo "NOTARIZE_PASS=$NOTARIZE_PASS"

xattr -rc ./EBS.app
echo "\n 1 \n"
rm -rf ./EBS.app/Contents/Frameworks/Qt*.framework/Headers
echo "\n 2 \n"
codesign --deep --options runtime -vfs 'Developer ID Application: Evercast LLC' './EBS.app' --keychain 'login.keychain' --entitlements ./EBS.app/Contents/entitlements.plist
echo "\n 3 \n"
mkdir /tmp/installebs
rm -rf /tmp/installebs/EBS.app
cp -R "./EBS.app" /tmp/installebs
productsign --sign 'Developer ID Installer: Evercast LLC' $NDI_RUNTIME_unsigned.pkg $NDI_RUNTIME
cp $NDI_RUNTIME /tmp/installebs
productsign --sign 'Developer ID Installer: Evercast LLC' $NDI_M1_RUNTIME_unsigned.pkg $NDI_M1_RUNTIME
cp $NDI_M1_RUNTIME /tmp/installebs
hdiutil create /tmp/tmp.dmg -ov -volname "EBS_""$EBS_VERSION""_Install" -fs HFS+ -srcfolder /tmp/installebs # "./EBS.app"
cp /tmp/tmp.dmg ./EBS_"$EBS_VERSION"_Install.dmg
codesign -s "Developer ID Application: Evercast LLC" "EBS_""$EBS_VERSION""_Install.dmg" --options runtime
# productsign --sign 'Developer ID Installer: Evercast LLC' --keychain 'login.keychain' ./EBS_"$EBS_VERSION"_Install_unsigned.dmg ./EBS_"$EBS_VERSION"_Install.dmg
xcrun altool --notarize-app --primary-bundle-id "ci.cosmosoftware.obs-webrtc" --username brad@evercast.co --password $NOTARIZE_PASS --file ./EBS_"$EBS_VERSION"_Install.dmg
