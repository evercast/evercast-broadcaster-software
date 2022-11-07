rem ---------------------------------------------------------------------
cmake ^
  -G "NMake Makefiles" ^
  -DENABLE_SCRIPTING=OFF ^
  -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true ^
  -DBUILD_CAPTIONS=false ^
  -DCOMPILE_D3D12_HOOK=true ^
  -DBUILD_BROWSER=false ^
  -Dlibwebrtc_DIR=%libwebrtcPath% ^
  -DOPENSSL_ROOT_DIR=%opensslPath% ^
  -DQt5_DIR=%QTDIR64%\lib\cmake\Qt5 ^
  -DCMAKE_BUILD_TYPE=%build_config% ^
  -DOBS_WEBRTC_VENDOR_NAME=Evercast ^
  -DOBS_VERSION_OVERRIDE=%EBS_VERSION% ^
  -DUI_VERSION_MAJOR=%EBS_VERSION_MAJOR% ^
  -DUI_VERSION_MINOR=%EBS_VERSION_MINOR% ^
  -DUI_VERSION_PATCH=%EBS_VERSION_PATCH% ^
  -DFFMPEG_AVCODEC_INCLUDE_DIRS=%ffmpegPath%\include ^
  -DFFMPEG_AVCODEC_LIBRARIES=%ffmpegPath%\lib ^
  -DDepsPath64=%DepsPath64% ^
  -DNDI_PATH=%NDIPath% ^
  -DVLCPath=%VLCPath% ^
  ..

