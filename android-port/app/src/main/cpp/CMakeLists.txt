# CMakeLists.txt for TaijiOS Android app
# This file is used by Android Studio's Gradle build system

cmake_minimum_required(VERSION 3.18.1)
project("taijos-android")

# Set C standard
set(CMAKE_C_STANDARD 11)

# Define Android-specific macros
add_definitions(-DANDROID -D__ANDROID__)

# Find paths to parent directories
set(TAIJIOS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(EMU_PORT ${TAIJIOS_ROOT}/emu/port)
set(EMU_ANDROID ${TAIJIOS_ROOT}/emu/Android)

# Include directories
include_directories(
	${EMU_PORT}
	${TAIJIOS_ROOT}/include
	${TAIJIOS_ROOT}/libinterp
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Source files - port files (shared with other platforms)
file(GLOB_RECURSE EMU_PORT_SOURCES
	"${EMU_PORT}/*.c"
)

# Exclude files not needed for Android
list(FILTER EMU_PORT_SOURCES EXCLUDE REGEX ".*win-x11.*")
list(FILTER EMU_PORT_SOURCES EXCLUDE REGEX ".*devpipe-9.*")
list(FILTER EMU_PORT_SOURCES EXCLUDE REGEX ".*audio-oss.*")

# Android-specific source files
set(EMU_ANDROID_SOURCES
	${EMU_ANDROID}/os.c
	${EMU_ANDROID}/win.c
	${EMU_ANDROID}/deveia.c
	${EMU_ANDROID}/devaudio.c
	${EMU_ANDROID}/devip.c
	${EMU_ANDROID}/devfs.c
	${EMU_ANDROID}/android_main.c
	${EMU_ANDROID}/segflush-arm64.c
	${EMU_ANDROID}/asm-arm64.S
)

# Limbo/Dis VM source files
file(GLOB_RECURSE LIBINTERP_SOURCES
	"${TAIJIOS_ROOT}/libinterp/*.c"
)

# Draw library source files
file(GLOB_RECURSE LIBDRAW_SOURCES
	"${TAIJIOS_ROOT}/libdraw/*.c"
)

# Combine all sources
set(TAIJIOS_SOURCES
	${EMU_PORT_SOURCES}
	${EMU_ANDROID_SOURCES}
)

# Build the shared library
add_library(taijos SHARED ${TAIJIOS_SOURCES})

# Link libraries
target_link_libraries(taijos
	log
	android
	EGL
	GLESv3
	OpenSLES
	m
	pthread
	dl
	z
)

# Set output name
set_target_properties(taijos PROPERTIES
	OUTPUT_NAME "taijos"
)
