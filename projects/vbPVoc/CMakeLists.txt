cmake_minimum_required (VERSION 3.14)
set(project_name "vbPVoc")
project (${project_name})

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(vbPVoc MODULE 
			${project_name}.cpp
			)



find_library(FFTW fftw3f)


if(APPLE)
	message("using vDSP")
	add_definitions("-DSC_FFT_VDSP")
	target_link_libraries(${project_name} "-framework accelerate")

elseif(FFTW)
	message(found fftw3!)
	add_definitions("-DSC_FFT_FFTW")
	target_link_libraries(${project_name} ${FFTW})

else()
	message(WARNING "no vDSP and no fftw3 found!")

endif()


install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})