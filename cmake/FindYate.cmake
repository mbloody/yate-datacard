
find_package(PkgConfig)

pkg_check_modules(PC_YATE REQUIRED yate)
set(YATE_DEFINITIONS ${PC_YATE_CFLAGS_OTHER})

find_path(YATE_INCLUDE_DIR yatengine.h
          HINTS ${PC_YATE_INCLUDE_DIR} ${PC_YATE_INCLUDE_DIRS}
          PATH_SUFFIXES yate )

find_library(YATE_LIBRARY NAMES yate
             HINTS ${PC_YATE_LIBDIR} ${PC_YATE_LIBRARY_DIRS} )

set(YATE_LIBRARIES ${YATE_LIBRARY} )
set(YATE_INCLUDE_DIRS ${YATE_INCLUDE_DIR} )

set(YATE_MODULES_DIR ${PC_YATE_LIBDIR}/yate)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set YATE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Yate  DEFAULT_MSG
                                  YATE_LIBRARY YATE_INCLUDE_DIR)

mark_as_advanced(YATE_INCLUDE_DIR YATE_LIBRARY )
