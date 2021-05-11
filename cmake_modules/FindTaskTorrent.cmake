#[=======================================================================[.rst:
FindTaskTorrent
----------

Finds TaskTorrent

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``TaskTorrent::TaskTorrent``
  The TaskTorrent library

Result Variables
^^^^^^^^^^^^^^^^

``TaskTorrent_FOUND``
  Set if TaskTorrent was found

``TaskTorrent_INCLUDE_DIRS``
  Include directories needed to use TaskTorrent

``TaskTorrent_LIBRARIES``
  Libraries needed to link to TaskTorrent

``TaskTorrent_CFLAGS_OTHER``
  Other CFLAGS needed to use TaskTorrent

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``TaskTorrent_INCLUDE_DIR``
  The include directory for TaskTorrent

``TaskTorrent_LIBRARY``
  Path to the library for TaskTorrent

#]=======================================================================]

include(FindPackageHandleStandardArgs)
find_package(PkgConfig)

pkg_check_modules(_TaskTorrent_PC QUIET taskTorrent)
find_path(TaskTorrent_INCLUDE_DIR
          NAMES "tasktorrent.hpp"
          HINTS
            ENV TTOR_ROOT
            ${CMAKE_SOURCE_DIR}/experiments/external/tasktorrent-install
          PATHS
            ${_TaskTorrent_PC_INCLUDE_DIRS}
          PATH_SUFFIXES
            include
)
find_library(TaskTorrent_LIBRARY
             NAMES "TaskTorrent"
             HINTS
               ENV TTOR_ROOT
               ${CMAKE_SOURCE_DIR}/experiments/external/tasktorrent-install
             PATHS
               ${_TaskTorrent_PC_LIBRARY_DIRS}
             PATH_SUFFIXES
               lib
)
set(TaskTorrent_VERSION ${_TaskTorrent_PC_VERSION})

find_package_handle_standard_args(TaskTorrent
  REQUIRED_VARS
    TaskTorrent_INCLUDE_DIR
    TaskTorrent_LIBRARY
  VERSION_VAR TaskTorrent_VERSION
)

if(TaskTorrent_FOUND)
  set(TaskTorrent_INCLUDE_DIRS ${TaskTorrent_INCLUDE_DIR})
  set(TaskTorrent_LIBRARIES ${TaskTorrent_LIBRARY})
  set(TaskTorrent_CFLAGS_OTHER ${_TaskTorrent_PC_CFLAGS_OTHER})

  if(NOT TARGET TaskTorrent::TaskTorrent)
    add_library(TaskTorrent::TaskTorrent UNKNOWN IMPORTED)
    set_target_properties(TaskTorrent::TaskTorrent PROPERTIES
      IMPORTED_LOCATION "${TaskTorrent_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${TaskTorrent_INCLUDE_DIR}"
      INTERFACE_COMPILE_OPTIONS "${_TaskTorrent_PC_CFLAGS_OTHER}"
    )
  endif()

  mark_as_advanced(TaskTorrent_INCLUDE_DIR TaskTorrent_LIBRARY)
endif()
