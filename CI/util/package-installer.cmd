@ECHO OFF

SET WIX=%USERPROFILE%\project\CI\tools
ECHO %WIX%
SET PATH=%PATH%;%WIX%
SET PATH=%PATH%;C:\Program Files\CMake\bin
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64

CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cpack.exe" -G WIX
"%WIX%\candle.exe" "..\CI\install\win\Install EBS.wxs" -ext WixBalExtension -ext WixUtilExtension
"%WIX%\light.exe" "Install EBS.wixobj" -ext WixBalExtension -ext WixUtilExtension
MOVE "Install EBS.exe" "Install EBS %EBS_VERSION%.exe"

rem store installer as artifact
MKDIR artifacts
COPY "Install EBS %EBS_VERSION%.exe" artifacts
DIR artifacts
