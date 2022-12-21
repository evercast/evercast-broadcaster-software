project(mac-capture)

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)
find_library(COREFOUNDATION CoreFoundation)
find_library(IOSURF IOSurface)
find_library(COCOA Cocoa)
find_library(COREVIDEO CoreVideo)
find_library(COREMEDIA CoreMedia)
find_library(SCREENCAPTUREKIT ScreenCaptureKit)

include_directories(${COREAUDIO}
                    ${AUDIOUNIT}
                    ${COREFOUNDATION}
		    ${IOSURF}
		    ${COCOA})

set(mac-capture_HEADERS
	audio-device-enum.h
	window-utils.h)

set(mac-capture_SOURCES
	plugin-main.c
	audio-device-enum.c
	mac-audio.c
	mac-display-capture.m
	mac-screen-capture.m
	mac-window-capture.m
	window-utils.m)
	
set_source_files_properties(mac-display-capture.m
			    mac-window-capture.m
			    window-utils.m
	PROPERTIES LANGUAGE C)

add_library(mac-capture MODULE
	${mac-capture_SOURCES}
	${mac-capture_HEADERS})
target_link_libraries(mac-capture
	libobs
	${COREAUDIO}
	${AUDIOUNIT}
	${COREFOUNDATION}
	${IOSURF}
	${COCOA})

if(SCREENCAPTUREKIT)
  target_link_libraries(mac-capture libobs ${COREVIDEO}
                                            ${COREMEDIA})

  target_link_options(mac-capture PRIVATE SHELL:-weak_framework
                      ScreenCaptureKit)
  target_link_options(libobs PRIVATE SHELL:-weak_framework ScreenCaptureKit)
endif()

install_obs_plugin_with_data(mac-capture data)
