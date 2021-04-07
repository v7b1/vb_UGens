cmake_minimum_required (VERSION 3.14)

set(project_name "vbChebyFilt")
project (${project_name})

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(${project_name} MODULE ${project_name}.cpp)

if (APPLE)
	target_link_libraries(${project_name} "-framework accelerate")
endif()

install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})