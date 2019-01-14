cmake_minimum_required(VERSION 3.8)

project(server)

set(ROOT_DIR "${PROJECT_SOURCE_DIR}/../")
set(CMAKE_CXX_STANDARD 17)

include_directories(
	${PROJECT_SOURCE_DIR}/../include
	${PROJECT_SOURCE_DIR}/../../core/thirdparty/include
	${PROJECT_SOURCE_DIR}/../../core/include
	${PROJECT_SOURCE_DIR}/../../game/include
)

if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Debug Release)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset configurations" FORCE)
endif()

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/../source/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/../source/*.hpp)

add_executable(server WIN32 ${SOURCE_FILES} ${HEADER_FILES})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT server)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

if(${WIN32})
	set(DEBUG_LINK_LIBRARIES
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/freetype.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/glew32sd.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/libogg.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/libpng16.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/libvorbis.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/libvorbisfile.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/libvorbisfile.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/zlib.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/IrrXML.lib
		debug ${ROOT_DIR}/../core/thirdparty/lib/debug/assimp.lib
		debug opengl32.lib
		debug glu32.lib
		debug ws2_32.lib
		debug ${ROOT_DIR}/../core/lib/debug/core.lib
		debug ${ROOT_DIR}/../game/lib/debug/game.lib
	)
	set(RELEASE_LINK_LIBRARIES
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/freetype.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/glew32s.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/libogg.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/libpng16.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/libvorbis.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/libvorbisfile.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/libvorbisfile.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/zlib.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/IrrXML.lib
		optimized ${ROOT_DIR}/../core/thirdparty/lib/release/assimp.lib
		optimized opengl32.lib
		optimized glu32.lib
		optimized ws2_32.lib
		optimized ${ROOT_DIR}/../core/lib/release/core.lib
		optimized ${ROOT_DIR}/../game/lib/release/game.lib
	)
	set(ALL_LINK_LIBRARIES ${DEBUG_LINK_LIBRARIES} ${RELEASE_LINK_LIBRARIES})
	target_link_libraries(server ${ALL_LINK_LIBRARIES})
endif()
