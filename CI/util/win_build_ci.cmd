@ECHO OFF
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

set PATH=%PATH%;"C:\Program Files\CMake\bin"
set PATH=%PATH%;"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64"

echo %PATH%

..\CI\configure-script-win.cmd
nmake

