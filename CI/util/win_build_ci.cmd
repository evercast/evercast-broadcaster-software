rem @ECHO OFF

SET PATH="%PATH%;C:\Program Files\CMake\bin"
SET PATH="%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64"

ECHO %PATH%

"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

CD ..\..\build

..\CI\configure-script-win.cmd
nmake
