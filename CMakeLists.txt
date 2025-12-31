cmake_minimum_required(VERSION 3.19)
set(lib_name "vbUGens")
project("${lib_name}")

if (APPLE)
    if (NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
    endif()
endif()

if (NOT MSVC)
  # specify the C++ standard
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" CACHE STRING "c++ version" FORCE)
  if(CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()


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
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/SCM)
if (NOT MSVC) 
  # Npleth currently doesn't compile on windows
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/NPleth)
endif()




# Install sc classes and help files
install(DIRECTORY "sc/classes" "sc/HelpSource" DESTINATION "${CMAKE_INSTALL_PREFIX}")