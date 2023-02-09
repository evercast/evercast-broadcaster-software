project(vlc-video)

if(DISABLE_VLC)
	message(STATUS "VLC video plugin disabled")
	return()
endif()

if(NOT VLCPath)
	set(VLCPath ${CMAKE_CURRENT_SOURCE_DIR}/vlc)
endif()

find_package(LibVLC QUIET)

if(NOT LIBVLC_INCLUDES_FOUND AND ENABLE_VLC)
	message(FATAL_ERROR "LibVLC includes not found but set as enabled")
elseif(NOT LIBVLC_INCLUDES_FOUND)
	message(STATUS "LibVLC includes not found, VLC video plugin disabled")
	return()
endif()

include_directories(${LIBVLC_INCLUDE_DIRS})
add_definitions(${LIBVLC_DEFINITIONS})

if(MSVC)
	set(vlc-video_PLATFORM_DEPS
		w32-pthreads)
endif()

set(vlc-video_HEADERS
	vlc-video-plugin.h
	)

set(vlc-video_SOURCES
	vlc-video-plugin.c
	vlc-video-source.c
	)

add_library(vlc-video MODULE
	${vlc-video_SOURCES}
	${vlc-video_HEADERS})
target_link_libraries(vlc-video
	libobs
	${vlc-video_PLATFORM_DEPS})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BITS 64)
else()
	set(BITS 32)
endif()

add_custom_command(TARGET obs-ffmpeg POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	"$ENV{DepsPath${BITS}}/bin/libsrt.dll"
	"${CMAKE_BINARY_DIR}/rundir/$<CONFIGURATION>/bin/${BITS}bit/"
)


install_obs_plugin_with_data(vlc-video data)
