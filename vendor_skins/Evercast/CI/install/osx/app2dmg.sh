hdiutil create /tmp/tmp.dmg -ov -volname "EBS_""$EBS_VERSION""_Install" -fs HFS+ -srcfolder "./EBS.app" 
hdiutil convert /tmp/tmp.dmg -format UDZO -o ./EBS_"$EBS_VERSION"_Install.dmg
