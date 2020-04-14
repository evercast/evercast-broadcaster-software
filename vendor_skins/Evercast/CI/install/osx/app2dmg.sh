hdiutil create /tmp/tmp.dmg -ov -volname "EBS_2.5_Install" -fs HFS+ -srcfolder "./EBS.app" 
hdiutil convert /tmp/tmp.dmg -format UDZO -o ./EBS_2.5_Install.dmg
