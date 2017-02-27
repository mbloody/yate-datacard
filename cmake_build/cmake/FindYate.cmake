# rst:
# FindYate
# --------
#
# Find YATE include file and libraries
#
# Result variables
#
# YATE_FOUND - system has yate installed
# YATE_INCLUDE_DIR - the YATE include directory
# YATE_LIBRARIES - 
# YATE_VERSION_STRING -
# YATE_MODULES_DIR - 
# YATE_CONFIG_DIR - 
# 
#
################################################################################


# find yate-config
#
find_program(YATE_CONFIG_EXECUTABLE NAMES yate-config DOC "yate-config executable")

if(YATE_CONFIG_EXECUTABLE)
	execute_process(COMMAND ${YATE_CONFIG_EXECUTABLE} --version
			OUTPUT_VARIABLE YATE_VERSION_STRING
			OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(COMMAND ${YATE_CONFIG_EXECUTABLE} --modules
			OUTPUT_VARIABLE YATE_MODULES_DIR
			OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(COMMAND ${YATE_CONFIG_EXECUTABLE} --config
			OUTPUT_VARIABLE YATE_CONFIG_DIR
			OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

find_path(YATE_INCLUDE_DIR yatengine.h
	PATH_SUFFIXES yate)

message(${YATE_INCLUDE_DIR})

find_library(YATE_LIBRARY NAMES yate)

set(YATE_LIBRARIES ${YATE_LIBRARY})

mark_as_advanced(YATE_CONFIG_DIR YATE_MODULES_DIR YATE_LIBRARY_DIR YATE_INCLUDE_DIR)
