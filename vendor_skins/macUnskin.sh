rm -rf CI/install/win
rm CI/install/osx/Info.plist
rm CI/install/osx/dylibBundler
rm CI/install/osx/packageApp.sh
git checkout -- CI/*
rm UI/webrtcVersion.h
rm -rf UI/update
git checkout -- UI/*
git checkout -- cmake/*
git checkout -- deps/*
rm libobs/obs-cocoa.m
git checkout -- libobs/*
rm libobs-d3d11/d3d11-texture3d.cpp
git checkout -- libobs-d3d11/*
rm libobs-opengl/gl-texture3d.c
git checkout -- libobs-opengl/*
git checkout -- plugins/obs-outputs
git checkout -- plugins/mac-avcapture/*
git checkout -- plugins/rtmp-services/*
git checkout -- plugins/obs-outputs/*
rm plugins/obs-outputs/EvercastOutputs.h
