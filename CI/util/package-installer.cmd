SET WIX=%USERPROFILE%\project\CI\tools
ECHO %WIX%
SET PATH=%PATH%;%WIX%
SET PATH=%PATH%;C:\Program Files\CMake\bin
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64
SET PATH=%PATH%;C:\Users\circleci\project\deps-install\obs-deps\win64\include
SET PATH=%PATH%;C:\Users\circleci\project\deps\w32-pthreads

ECHO %PATH%

CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cpack.exe" -G WIX --verbose --debug

ECHO "****************************** Preinstall log ******************************"
TYPE .\_CPack_Packages\win64\WIX\PreinstallOutput.log
ECHO "****************************** Output log ******************************"
TYPE .\CMakeFiles\CMakeOutput.log
ECHO "****************************** Error log ******************************"
TYPE .\CMakeFiles\CMakeError.log

"%WIX%\candle.exe" "..\CI\install\win\Install EBS.wxs" -ext WixBalExtension -ext WixUtilExtension
"%WIX%\light.exe" "Install EBS.wixobj" -ext WixBalExtension -ext WixUtilExtension
MOVE "Install EBS.exe" "Install EBS %EBS_VERSION%.exe"
