cmake_minimum_required(VERSION 3.8)

project(core)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}/../")
set(CMAKE_CXX_STANDARD 17)

include_directories(
	${PROJECT_SOURCE_DIR}/../include
	${PROJECT_SOURCE_DIR}/../source
	${PROJECT_SOURCE_DIR}/../thirdparty/include
	${PROJECT_SOURCE_DIR}/../thirdparty/include/freetype
	${PROJECT_SOURCE_DIR}/../thirdparty/include/imgui
)

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset configurations" FORCE)
endif()

file(GLOB_RECURSE SOURCE_CPP_FILES ${PROJECT_SOURCE_DIR}/../source/*.cpp)
file(GLOB_RECURSE SOURCE_HPP_FILES ${PROJECT_SOURCE_DIR}/../source/*.hpp)
file(GLOB_RECURSE HEADER_HPP_FILES ${PROJECT_SOURCE_DIR}/../include/*.hpp)

file(GLOB_RECURSE SOURCE_IMGUI_CPP_FILES ${PROJECT_SOURCE_DIR}/../thirdparty/source/imgui/*.cpp)
file(GLOB_RECURSE SOURCE_IMGUI_H_FILES ${PROJECT_SOURCE_DIR}/../thirdparty/include/imgui/*.h)

source_group("Source Files\\imgui" FILES ${SOURCE_IMGUI_CPP_FILES})
source_group("Header Files\\imgui" FILES ${SOURCE_IMGUI_H_FILES})

add_library(core STATIC 
	${SOURCE_CPP_FILES} 
	${SOURCE_HPP_FILES}
	${HEADER_HPP_FILES}
	${SOURCE_IMGUI_H_FILES}
	${SOURCE_IMGUI_CPP_FILES}
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT core)

add_definitions(-DGLEW_STATIC)

set_target_properties(core PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${ROOT_DIR}/lib")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

if(${WIN32})
	set(DEBUG_LINK_LIBRARIES
		debug ${ROOT_DIR}/thirdparty/lib/debug/freetype.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/glew32sd.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/libogg.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/libpng16.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/libvorbis.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/libvorbisfile.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/zlib.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/IrrXML.lib
		debug ${ROOT_DIR}/thirdparty/lib/debug/assimp.lib
		debug opengl32.lib
		debug glu32.lib
		debug ws2_32.lib
	)
	set(RELEASE_LINK_LIBRARIES
		optimized ${ROOT_DIR}/thirdparty/lib/release/freetype.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/glew32s.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/libogg.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/libpng16.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/libvorbis.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/libvorbisfile.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/zlib.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/IrrXML.lib
		optimized ${ROOT_DIR}/thirdparty/lib/release/assimp.lib
		optimized opengl32.lib
		optimized glu32.lib
		optimized ws2_32.lib
	)
	set(ALL_LINK_LIBRARIES ${DEBUG_LINK_LIBRARIES} ${RELEASE_LINK_LIBRARIES})
	target_link_libraries(core ${ALL_LINK_LIBRARIES})
endif()
