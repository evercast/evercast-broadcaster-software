@ECHO OFF

SET PATH=%PATH%;C:\Program Files\CMake\bin
SET PATH=%PATH%;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64

SET EBS_DIR=../../..
SET QTDIR64=C:/Users/circleci/project/deps-install/Qt/5.15.2/msvc2019_64

CALL "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

CD ..\..\deps-install\obs-ndi
MKDIR build
CD build

cmake ^
  -G "NMake Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DLIBOBS_INCLUDE_DIR=%EBS_DIR%/libobs ^
  -DLIBOBS_LIB=%EBS_DIR%/build/libobs/obs.lib ^
  -Dlibobs_ROOT=%EBS_DIR%/build/libobs ^
  -DOBS_FRONTEND_LIB=%EBS_DIR%/build/UI/obs-frontend-api/obs-frontend-api.lib ^
  -DQt5Core_DIR=%QTDIR64%/lib/cmake/Qt5Core ^
  -DQt5Widgets_DIR=%QTDIR64%/lib/cmake/Qt5Widgets ^
  -DQTDIR=%QTDIR64% ^
  -DW32_PTHREADS_LIB=C:/Users/circleci/project/build/deps/w32-pthreads ^
  -Dw32-pthreads_DIR=C:/Users/circleci/project/build/deps/w32-pthreads ^
  ..

nmake

MKDIR plugin\data\obs-plugins\obs-ndi\locale
MKDIR plugin\obs-plugins\64bit
XCOPY ..\data\locale\* plugin\data\obs-plugins\obs-ndi\locale
XCOPY .\obs-ndi.dll plugin\obs-plugins\64bit

SET NDI_DIR=..\obs-plugins
MKDIR %NDI_DIR%
XCOPY /E /I plugin\* %NDI_DIR%
DIR %NDI_DIR%
