set(VERSION_CMAKE_ROOT_PATH
    "${CMAKE_CURRENT_LIST_DIR}"
    CACHE INTERNAL "The path to the version.cmake"
)

function(target_add_version_header AVH_TARGET_NAME)
  include(GNUInstallDirs)

  cmake_parse_arguments(
    AVH
    ""
    "VERSION_HEADER;INSTALL"
    ""
    ${ARGN}
  )

  string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
  string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
  set(AVH_INCLUDE_DIR ${PROJECT_BINARY_DIR}/avh__/${PROJECT_NAME_LOWERCASE})

  if(NOT DEFINED AVH_VERSION_HEADER)
    set(AVH_VERSION_HEADER version.h)
  endif()

  configure_file(
    ${VERSION_CMAKE_ROOT_PATH}/version.h.in ${AVH_INCLUDE_DIR}/${AVH_VERSION_HEADER} @ONLY
  )

  target_include_directories(${AVH_TARGET_NAME} INTERFACE "$<BUILD_INTERFACE:${AVH_INCLUDE_DIR}>")

  if(DEFINED AVH_INSTALL)
    install(DIRECTORY ${AVH_INCLUDE_DIR}/ DESTINATION ${AVH_INSTALL})
  endif()

endfunction()
