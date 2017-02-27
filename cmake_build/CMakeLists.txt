cmake_minimum_required(VERSION 2.8)
project(yate_datacard CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(Yate REQUIRED)

INCLUDE_DIRECTORIES(${YATE_INCLUDE_DIR})

#SET(GCC_COVERAGE_COMPILE_FLAGS "-O3 -Wno-overloaded-virtual -fno-exceptions -fPIC -DHAVE_GCC_FORMAT_CHECK -DHAVE_BLOCK_RETURN")

add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

#SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )




#set version from git (as old autotools script)
#FIXME
find_package(Git)
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --always --dirty
            OUTPUT_VARIABLE GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

add_definitions(-DDTC_VER="${GIT_HASH}")
ADD_LIBRARY(datacard MODULE
		     at_io.cpp
		     at_parse.cpp
		     at_response.cpp
		     char_conv.cpp
		     datacard.cpp
		     datacarddevice.cpp
		     pdu.cpp
		     )
TARGET_LINK_LIBRARIES(datacard ${YATE_LIBRARIES})
SET_TARGET_PROPERTIES(datacard PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(datacard PROPERTIES SUFFIX .yate)

INSTALL(TARGETS datacard
		DESTINATION ${YATE_MODULES_DIR}
		RENAME datacard.yate
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

INSTALL(FILES datacard.conf.sample
		DESTINATION ${YATE_CONFIG_DIR}
		RENAME datacard.conf)
