cmake_minimum_required (VERSION 3.14)
set(project_name "vbPVoc")
project (${project_name})

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)


# here we choose who provides us with the FFT lib



set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(vbPVoc MODULE 
			vbPVoc.cpp
			MyFFTInterfaceTable.cpp
			)



find_library(FFTW fftw3f)


if(APPLE)
	message("using vDSP")
	add_definitions("-DSC_FFT_VDSP")
	target_link_libraries(vbPVoc "-framework accelerate")

elseif(FFTW)
	message(found fftw3!)
	add_definitions("-DSC_FFT_FFTW")
	target_link_libraries(vbPVoc ${FFTW})

else()
	message(FATAL_ERROR "no vDSP and no fftw3 found!!")

endif()


# installing
# if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
#   message(WARNING "No install prefix provided, defaulting to $BUILD_DIR")
#   set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Install prefix" FORCE)
# endif()

# message(STATUS "Install directory set to: ${CMAKE_INSTALL_PREFIX}")

install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})