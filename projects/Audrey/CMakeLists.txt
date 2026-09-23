cmake_minimum_required (VERSION 3.14)

set(project_name "Audrey")
project (${project_name})

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)

if(MSVC)
add_definitions(-D_USE_MATH_DEFINES) # defines M_PI with MSVC
endif()

set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()


set(LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sources")
set(DAISY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/DaisySP")

include_directories( 
	"${LIB_PATH}"
	"${DAISY_PATH}"
)

#add_subdirectory(${LIB_PATH})

set(AUDREY_SOURCES
	${LIB_PATH}/BiquadFilters.cpp
	${LIB_PATH}/BiquadFilters.h
	${LIB_PATH}/DSPUtils.h
	${LIB_PATH}/EchoDelay.h
	${LIB_PATH}/FeedbackSynthEngine.cpp
	${LIB_PATH}/FeedbackSynthEngine.h
	${LIB_PATH}/KarplusString.cpp
	${LIB_PATH}/KarplusString.h
	
)

set(DAISY_LGPL_SOURCES
	${DAISY_LGPL_PATH}/tone.cpp
	${DAISY_LGPL_PATH}/tone.h
)

set(DAISY_SOURCES
	${DAISY_PATH}/daisysp.h
	${DAISY_PATH}/Dynamics/limiter.cpp
	${DAISY_PATH}/Dynamics/limiter.h
	${DAISY_PATH}/Dynamics/crossfade.cpp
	${DAISY_PATH}/Dynamics/crossfade.h
	${DAISY_PATH}/Filters/tone.cpp
	${DAISY_PATH}/Filters/tone.h
	${DAISY_PATH}/Effects/reverbsc.cpp
	${DAISY_PATH}/Effects/reverbsc.h
	${DAISY_PATH}/Effects/overdrive.cpp
	${DAISY_PATH}/Effects/overdrive.h
	${DAISY_PATH}/Utility/dcblock.cpp
	${DAISY_PATH}/Utility/dcblock.h
	
)

add_library( 
	${project_name} 
	MODULE
	audrey.cpp
	${AUDREY_SOURCES}
	${DAISY_SOURCES}
	
)


if(MSVC)
set_property (TARGET ${PROJECT_NAME}
  PROPERTY
    # Enable C++17 standard compliance
    CXX_STANDARD 17
)
endif()


# create groups in our project
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${AUDREY_SOURCES} ${DAISY_SOURCES})


