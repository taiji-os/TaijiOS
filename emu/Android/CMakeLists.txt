# CMakeLists.txt for TaijiOS Android port
# Used by Android Studio's Gradle build system

cmake_minimum_required(VERSION 3.18.1)
project(taijos_emu)

# Set C standard
set(CMAKE_C_STANDARD 11)

# Define Android-specific macros
add_definitions(-DANDROID -D__ANDROID__)

# Include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../port
	${CMAKE_CURRENT_SOURCE_DIR}/../../include
	${CMAKE_CURRENT_SOURCE_DIR}/../../../include
	${CMAKE_CURRENT_SOURCE_DIR}/../../../libinterp
)

# Source files for the emu library
set(EMU_SOURCES
	os.c
	segflush-arm64.c
	../port/kproc-pthreads.c
)

# Build the emu library as a static library
add_library(emu-android STATIC ${EMU_SOURCES})

# Link libraries
target_link_libraries(emu-android
	log
	android
	EGL
	GLESv3
	m
	pthread
)

# Set output directory
set_target_properties(emu-android PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)
