cmake_minimum_required (VERSION 2.6)
project (elf)
set (elf_VERSION_MAJOR 1)
set (elf_VERSION_MINOR 0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC  -Wno-write-strings")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC  -Wno-write-strings")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/bin)

if(NOT CMAKE_BUILD_TYPE)
	message(WARNING "cmake was execute without specify build type, run cmake -DCMAKE_BUILD_TYPE=Release for release build")
	set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -DDEBUG")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -DDEBUG")
endif()

if(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

include_directories($ENV{XSTL_PATH}/Include)
include_directories(Include)

add_definitions(-DXSTL_UNICODE)
add_definitions(-D_UNICODE)

list(APPEND ELF_LIB_FILES
	Source/elf/cElfFile.cpp
	Source/elf/cElfRelocationSection.cpp
	Source/elf/cElfStringTable.cpp
	Source/elf/cElfMemoryMapper.cpp
	Source/elf/cElfSection.cpp
	Source/elf/cElfSymbol.cpp
	Source/elf/Dwarf/cDwarf2Manager.cpp
	Source/elf/Dwarf/cDwarfAttribute.cpp
	Source/elf/Dwarf/cDwarfDie.cpp
)

add_library(elf_static STATIC ${ELF_LIB_FILES})
add_library(elf SHARED ${ELF_LIB_FILES})

# Try to find CMake single library
	find_library(LIBXSTLCM xStlcm HINTS $ENV{XSTL_PATH}/out/lib)
	target_link_libraries(elf ${LIBXSTLCM})
if(NOT LIBXSTLCM)
# Compatibility mode with autotools (Only linux) isn't fully tested.
	find_library(LIBXSTL xstl HINTS $ENV{XSTL_PATH}/out/lib)
	find_library(LIBXSTL_DATA xstl_data HINTS $ENV{XSTL_PATH}/out/lib)
	find_library(LIBXSTL_EXCEPT xstl_except HINTS $ENV{XSTL_PATH}/out/lib)
	find_library(LIBXSTL_OS xstl_os HINTS $ENV{XSTL_PATH}/out/lib)
	find_library(LIBXSTL_PARSER xstl_parser HINTS $ENV{XSTL_PATH}/out/lib)
	find_library(LIBXSTL_STREAM xstl_stream HINTS $ENV{XSTL_PATH}/out/lib)
	find_library(LIBXSTL_UNIX xstl_unix HINTS $ENV{XSTL_PATH}/out/lib)

	target_link_libraries(elf ${LIBXSTL} ${LIBXSTL_DATA} ${LIBXSTL_EXCEPT}
	                              ${LIBXSTL_OS} ${LIBXSTL_PARSER} ${LIBXSTL_STREAM}
	                              ${LIBXSTL_UNIX})
endif()
if (UNIX)
	set(CMAKE_MACOSX_RPATH 1)
	add_definitions(-DLINUX)
endif()
if (WIN32)
	add_definitions(-DWIN32)
endif()
