cmake_minimum_required(VERSION 2.8)
project(yate_datacard)

#find yate
find_package(PkgConfig)
pkg_check_modules(PC_YATE QUIET yate)

find_path(YATE_INCLUDE_DIR yatengine.h
          HINTS ${PC_YATE_INCLUDEDIR} ${PC_YATE_INCLUDE_DIRS}
          PATH_SUFFIXES yate)

set(YATE_INCLUDE_DIRS ${YATE_INCLUDE_DIR} )

include_directories(${YATE_INCLUDE_DIRS})

#set version from git (as old autotools script)
#FIXME
find_package(Git)
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --always --dirty
            OUTPUT_VARIABLE GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

SET(GCC_COVERAGE_COMPILE_FLAGS "-O3 -Wno-overloaded-virtual -fno-exceptions -fPIC -DHAVE_GCC_FORMAT_CHECK -DHAVE_BLOCK_RETURN")
SET(GCC_COVERAGE_LINK_FLAGS    "-rdynamic -shared")

add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

add_definitions(-DDTC_VER="${GIT_HASH}")

add_executable(datacard.yate
  at_io.cpp
  at_parse.cpp
  at_response.cpp
  char_conv.cpp
  datacard.cpp
  datacarddevice.cpp
  pdu.cpp
)
TARGET_LINK_LIBRARIES(datacard.yate yate pthread dl)
