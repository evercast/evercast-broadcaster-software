CD ..\..\build
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cpack.exe" -G WIX

PRINT C:/Users/circleci/project/build/_CPack_Packages/win64/WIX/PreinstallOutput.log

"%WIX%\bin\candle.exe" "..\CI\install\win\Install EBS.wxs" -ext WixBalExtension -ext WixUtilExtension
"%WIX%\bin\light.exe" "Install EBS.wixobj" -ext WixBalExtension -ext WixUtilExtension
MOVE "Install EBS.exe" "Install EBS %EBS_VERSION%.exe"
