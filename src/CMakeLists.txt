# CMake build script
cmake_minimum_required(VERSION 3.0)

# project name & version
project(mprt)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif() 


if (MSVC)
	set(GEN_COMPILE_FLAGS "-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
	set(GEN_COMPILE_FLAGS "${GEN_COMPILE_FLAGS} /std:c++17 -D_WIN32_WINNT=0x0A00 -D_ENABLE_EXTENDED_ALIGNED_STORAGE")

	message(STATUS ${MSVC_COMPILE_FLAGS})
else()
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	set(THREAD_LIB "Threads::Threads")
	set(GEN_COMPILE_FLAGS "-DBOOST_LOG_DYN_LINK -std=c++17")	
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GEN_COMPILE_FLAGS}")

#add_definitions(${GEN_COMPILE_FLAGS})



# common settings (Boost libraries)
#if (MSVC)
  # Enable the static libraries on Windows
#  foreach (flag_var 
#    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
#    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
#    CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
#    CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
#    # update to the static version of the run time library
#    string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#  endforeach()

#  set(CMAKE_C_STANDARD_LIBRARIES "")
#  set(CMAKE_CXX_STANDARD_LIBRARIES "")
#endif()

# enable boost static flag
# unset(Boost_LIBRARIES)

#set(Boost_DEBUG ON)

#find_package(Sqlite3 REQUIRED)
find_package(Boost REQUIRED system filesystem log)

#find_package(boost-filesystem REQUIRED)

find_path(FLAC_INCLUDE_DIR FLAC/stream_decoder.h)
find_library(FLAC_LIB FLAC)

find_library(TAG_LIB tag)

find_path(FFMPEG_LIBAV_CODEC_DIR libavcodec/avcodec.h)
find_library(FFMPEG_AV_CODEC_LIB avcodec)
find_path(FFMPEG_LIBAV_UTIL_DIR libavutil/avutil.h)
find_library(FFMPEG_AV_UTIL_LIB avutil)
find_path(FFMPEG_LIBAV_FORMAT_DIR libavformat/avformat.h)
find_library(FFMPEG_AV_FORMAT_LIB avformat)

#find_path(URIPARSER_INCLUDE_DIR uriparser/Uri.h)
#find_library(URIPARSER_LIB uriparser)
#set(Boost_USE_STATIC OFF)
#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)

# boost components
#find_package(Boost REQUIRED system filesystem)

message(STATUS ${Boost_SYSTEM_LIBRARY_DEBUG})

include_directories(
	"${PROJECT_SOURCE_DIR}"
	"${Boost_INCLUDE_DIRS}"
	"${FLAC_INCLUDE_DIR}"
	"${FFMPEG_LIBAV_CODEC_DIR}"
	"${FFMPEG_LIBAV_UTIL_DIR}"
	"${FFMPEG_LIBAV_FORMAT_DIR}"
	)

link_directories(${Boost_LIBRARY_DIRS} ${VCPKG_LIB_DIR})

set(mprt_dbg_libs
	"${Boost_SYSTEM_LIBRARY_DEBUG}"
	"${Boost_FILESYSTEM_LIBRARY_DEBUG}"
	"${Boost_LOG_LIBRARY_DEBUG}"
	)

set(mprt_opt_libs
	"${Boost_SYSTEM_LIBRARY_RELEASE}"
	"${Boost_FILESYSTEM_LIBRARY_RELEASE}"
	"${Boost_LOG_LIBRARY_RELEASE}"
	)

add_executable(mprt
	"${PROJECT_SOURCE_DIR}/core/plugins_loader.h"
	"${PROJECT_SOURCE_DIR}/core/plugins_loader.cpp"
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.h"
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.cpp"
	"${PROJECT_SOURCE_DIR}/core/config.h"
	"${PROJECT_SOURCE_DIR}/core/config.cpp"
	"${PROJECT_SOURCE_DIR}/main.cpp"
	)

add_library(playlist_management_plugin SHARED
	"${PROJECT_SOURCE_DIR}/core/config.h"
	"${PROJECT_SOURCE_DIR}/core/config.cpp"
	"${PROJECT_SOURCE_DIR}/common/refcounting_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/plugin_types.h"
	"${PROJECT_SOURCE_DIR}/common/async_tasker.h"
	"${PROJECT_SOURCE_DIR}/common/type_defs.h"
	"${PROJECT_SOURCE_DIR}/core/config.cpp"
	"${PROJECT_SOURCE_DIR}/core/config.h"
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.h"
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/tag_parser_abstract.h"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/tag_lib_parser.h"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/tag_lib_parser.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/playlist_item.h"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/playlist.h"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/playlist.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/playlist_management_plugin.h"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/playlist_management_plugin_imp.h"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/playlist_management_plugin_imp.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/album_art.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/core_plugins/playlist_management_plugin/album_art.h"
	)
	
target_link_libraries(playlist_management_plugin
	debug "${mprt_dbg_libs}"
	optimized "${mprt_opt_libs}"
	${TAG_LIB})
	
add_library(input_plugin_file SHARED
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.h"
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.cpp"
	"${PROJECT_SOURCE_DIR}/core/config.h"
	"${PROJECT_SOURCE_DIR}/core/config.cpp"
	"${PROJECT_SOURCE_DIR}/common/refcounting_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/plugin_types.h"
	"${PROJECT_SOURCE_DIR}/common/input_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/async_tasker.h"
	"${PROJECT_SOURCE_DIR}/common/type_defs.h"
	"${PROJECT_SOURCE_DIR}/common/cache_buffer.h"
	"${PROJECT_SOURCE_DIR}/plugins/input_plugins/input_plugin_file.h"
	"${PROJECT_SOURCE_DIR}/plugins/input_plugins/input_plugin_file.cpp"	
	)
	
target_link_libraries(input_plugin_file
	debug "${mprt_dbg_libs}"
	optimized "${mprt_opt_libs}"
	)
	
add_library(decoder_plugin_flac SHARED
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.h"
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.cpp"
	"${PROJECT_SOURCE_DIR}/core/config.h"
	"${PROJECT_SOURCE_DIR}/core/config.cpp"
	"${PROJECT_SOURCE_DIR}/common/refcounting_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/plugin_types.h"
	"${PROJECT_SOURCE_DIR}/common/decoder_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/async_tasker.h"
	"${PROJECT_SOURCE_DIR}/common/cache_buffer.h"
	"${PROJECT_SOURCE_DIR}/common/type_defs.h"
	"${PROJECT_SOURCE_DIR}/common/cache_manage.h"
	"${PROJECT_SOURCE_DIR}/common/sound_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/plugins/decoder_plugins/decoder_plugin_flac.h"
	"${PROJECT_SOURCE_DIR}/plugins/decoder_plugins/decoder_plugin_flac.cpp"
	)
target_link_libraries(decoder_plugin_flac
	debug "${mprt_dbg_libs}"
	optimized "${mprt_opt_libs}"
	${SOUND_LIB} ${THREAD_LIB} ${CMAKE_DL_LIBS} ${FLAC_LIB}
	)

	add_library(decoder_plugin_ffmpeg SHARED
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.h"
	"${PROJECT_SOURCE_DIR}/core/decoder_plugins_manager.cpp"
	"${PROJECT_SOURCE_DIR}/core/config.h"
	"${PROJECT_SOURCE_DIR}/core/config.cpp"
	"${PROJECT_SOURCE_DIR}/common/refcounting_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/plugin_types.h"
	"${PROJECT_SOURCE_DIR}/common/decoder_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/async_tasker.h"
	"${PROJECT_SOURCE_DIR}/common/cache_buffer.h"
	"${PROJECT_SOURCE_DIR}/common/type_defs.h"
	"${PROJECT_SOURCE_DIR}/common/cache_manage.h"
	"${PROJECT_SOURCE_DIR}/common/sound_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/plugins/decoder_plugins/decoder_plugin_ffmpeg.h"
	"${PROJECT_SOURCE_DIR}/plugins/decoder_plugins/decoder_plugin_ffmpeg.cpp"
	)	
#target_link_libraries(decoder_plugin_ffmpeg ${FFMPEG_AV_CODEC_LIB} ${FFMPEG_AV_FORMAT_LIB})

target_link_libraries(decoder_plugin_ffmpeg
	debug "${mprt_dbg_libs}"
	optimized "${mprt_opt_libs}"
	${SOUND_LIB} ${THREAD_LIB} ${CMAKE_DL_LIBS} ${FFMPEG_AV_CODEC_LIB} ${FFMPEG_AV_UTIL_LIB} ${FFMPEG_AV_FORMAT_LIB})
	
if (MSVC)
	add_library(output_plugin_dsound SHARED
		"${PROJECT_SOURCE_DIR}/common/refcounting_plugin_api.h"
		"${PROJECT_SOURCE_DIR}/common/plugin_types.h"
		"${PROJECT_SOURCE_DIR}/common/output_plugin_api.h"
		"${PROJECT_SOURCE_DIR}/common/async_tasker.h"
		"${PROJECT_SOURCE_DIR}/common/cache_buffer.h"
		"${PROJECT_SOURCE_DIR}/common/cache_manage.h"
		"${PROJECT_SOURCE_DIR}/common/sound_plugin_api.h"
		"${PROJECT_SOURCE_DIR}/core/config.cpp"
		"${PROJECT_SOURCE_DIR}/core/config.h"
		"${PROJECT_SOURCE_DIR}/plugins/output_plugins/output_plugin_dsound.h"
		"${PROJECT_SOURCE_DIR}/plugins/output_plugins/output_plugin_dsound.cpp"	
		)
		
	target_link_libraries(output_plugin_dsound
	debug "${mprt_dbg_libs}"
	optimized "${mprt_opt_libs}"
	)
else()
		
	find_package(ALSA)
	if (ALSA_FOUND)
		set(GCC_COMPILE_FLAGS "${GCC_COMPILE_FLAGS} -D_ALSA_FOUND")
		add_library(output_plugin_alsa SHARED
			"${PROJECT_SOURCE_DIR}/core/config.cpp"
			"${PROJECT_SOURCE_DIR}/core/config.h"
			"${PROJECT_SOURCE_DIR}/common/refcounting_plugin_api.h"
			"${PROJECT_SOURCE_DIR}/common/plugin_types.h"
			"${PROJECT_SOURCE_DIR}/common/output_plugin_api.h"
			"${PROJECT_SOURCE_DIR}/common/async_tasker.h"
			"${PROJECT_SOURCE_DIR}/common/cache_buffer.h"
			"${PROJECT_SOURCE_DIR}/common/cache_manage.h"
			"${PROJECT_SOURCE_DIR}/common/sound_plugin_api.h"
			"${PROJECT_SOURCE_DIR}/plugins/output_plugins/output_plugin_alsa.h"
			"${PROJECT_SOURCE_DIR}/plugins/output_plugins/output_plugin_alsa.cpp"
			)
		target_link_libraries(output_plugin_alsa 
		debug "${mprt_dbg_libs}"
		optimized "${mprt_opt_libs}"
		${ALSA_LIBRARY})
	endif ()

	add_definitions(${GCC_COMPILE_FLAGS})	
endif()


# Find the QtWidgets library
find_package(Qt5 COMPONENTS Core Widgets Svg REQUIRED)

#set(CMAKE_AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/forms/")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

set(ui_plugin_qt_SRCS
	"${PROJECT_SOURCE_DIR}/core/config.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/ui_plugin_qt.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/mainwindow.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/centralwidget.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_item_delegate.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_view.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_tabwidget.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_view_item_model.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_dock_widget.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/song_progress_widget.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/proxy_style.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/dock_widget.cpp"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/tab_bar.cpp"
)

set(ui_plugin_qt_HDRS
	"${PROJECT_SOURCE_DIR}/core/config.h"
	"${PROJECT_SOURCE_DIR}/common/refcounting_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/plugin_types.h"
	"${PROJECT_SOURCE_DIR}/common/async_tasker.h"
	"${PROJECT_SOURCE_DIR}/common/cache_buffer.h"
	"${PROJECT_SOURCE_DIR}/common/type_defs.h"
	"${PROJECT_SOURCE_DIR}/common/cache_manage.h"
	"${PROJECT_SOURCE_DIR}/common/sound_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/common/ui_plugin_api.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/ui_plugin_qt.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/mainwindow.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/centralwidget.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_item_delegate.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_view.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_tabwidget.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_view_item_model.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/playlist_dock_widget.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/song_progress_widget.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/proxy_style.h"
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/tab_bar.h"
	)

#set(ui_plugin_qt_UIS
#	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/forms/mainwindow.ui"
#)
#QT5_WRAP_UI(ui_plugin_qt_UIS_H ${ui_plugin_qt_UIS})

add_library(ui_plugin_qt SHARED
	"${PROJECT_SOURCE_DIR}/plugins/ui_plugins/ui_plugin_qt/resources/resources.qrc"
	${ui_plugin_qt_SRCS}
	${ui_plugin_qt_HDRS}
#	${ui_plugin_qt_UIS_H} 
)
	
target_link_libraries(ui_plugin_qt
	debug "${mprt_dbg_libs}"
	optimized "${mprt_opt_libs}"
	Qt5::Widgets Qt5::Svg
)

target_link_libraries(mprt
	debug "${mprt_dbg_libs}"
	optimized "${mprt_opt_libs}"
	${SOUND_LIB} ${THREAD_LIB} ${CMAKE_DL_LIBS} ${TAG_LIB}
	)
