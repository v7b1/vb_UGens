cmake_minimum_required (VERSION 2.8)
project (vbPVoc)

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
