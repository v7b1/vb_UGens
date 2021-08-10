cmake_minimum_required(VERSION 3.14)
set(lib_name "vbUGens")
project("${lib_name}")


# set installation directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${lib_name}" CACHE PATH "Install prefix" FORCE)
else()
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${lib_name}")
endif()



message(STATUS "Install directory set to: ${CMAKE_INSTALL_PREFIX}")


# add_subdirectories manually
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/vbChebyFilt)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/vbFourses)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/vbJonVerb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/vbPVoc)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/vbUtils)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/RCD)



# Install sc classes and help files
install(DIRECTORY "sc/classes" "sc/HelpSource" DESTINATION "${CMAKE_INSTALL_PREFIX}")