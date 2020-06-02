mkdir /tmp/installebs
cp -r "./EBS.app" /tmp/installebs
cp $NDI_RUNTIME /tmp/installebs
hdiutil create /tmp/tmp.dmg -ov -volname "EBS_""$EBS_VERSION""_Install" -fs HFS+ -srcfolder /tmp/installebs # "./EBS.app"
hdiutil convert /tmp/tmp.dmg -format UDZO -o ./EBS_"$EBS_VERSION"_Install.dmg
