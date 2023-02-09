@ECHO OFF

SET PATH=%PATH%;C:\Program Files\CMake\bin
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64

CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
CD ..\..\build
CALL ..\CI\configure-script-win.cmd
nmake

rem copy required dependencies
COPY "%DepsPath64%\bin\libaom.dll" .\rundir\%build_config%\bin\64bit\
COPY "%DepsPath64%\bin\libmbedcrypto.dll" .\rundir\%build_config%\bin\64bit\
COPY "%DepsPath64%\bin\libSvtAv1Enc.dll" .\rundir\%build_config%\bin\64bit\
COPY "%DepsPath64%\bin\librist.dll" .\rundir\%build_config%\bin\64bit\

