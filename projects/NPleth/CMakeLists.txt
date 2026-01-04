cmake_minimum_required (VERSION 3.14)

set(project_name "NPleth")
project (${project_name})

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

set(HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(TEENSY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/teensy")

set(TEENSY_SOURCES
	${TEENSY_PATH}/audio_core.hpp
	${TEENSY_PATH}/dspinst.h
	${TEENSY_PATH}/synth_waveform.hpp
	${TEENSY_PATH}/synth_sine.hpp
   	${TEENSY_PATH}/synth_dc.hpp
	${TEENSY_PATH}/synth_whitenoise.hpp
	${TEENSY_PATH}/synth_whitenoise.cpp
	${TEENSY_PATH}/synth_pinknoise.hpp
    	${TEENSY_PATH}/synth_pinknoise.cpp
    	${TEENSY_PATH}/synth_pwm.hpp
    	${TEENSY_PATH}/synth_pwm.cpp	
	${TEENSY_PATH}/filter_variable.cpp
	${TEENSY_PATH}/filter_variable.hpp
    	${TEENSY_PATH}/mixer.hpp
    	${TEENSY_PATH}/effect_multiply.cpp
    	${TEENSY_PATH}/effect_multiply.h
	${TEENSY_PATH}/effect_wavefolder.cpp
	${TEENSY_PATH}/effect_wavefolder.hpp
	${TEENSY_PATH}/effect_bitcrusher.h
    	${TEENSY_PATH}/effect_bitcrusher.cpp
	${TEENSY_PATH}/effect_flange.h
    	${TEENSY_PATH}/effect_flange.cpp
	${TEENSY_PATH}/effect_freeverb.hpp
    	${TEENSY_PATH}/effect_freeverb.cpp
	${TEENSY_PATH}/effect_granular.cpp
    	${TEENSY_PATH}/effect_granular.hpp
	${TEENSY_PATH}/effect_combine.hpp
	${TEENSY_PATH}/effect_combine.cpp
)

set(HEADERS
	${HEADER_PATH}/np_radioOhNo.hpp
	${HEADER_PATH}/np_xmodringsine.hpp
	${HEADER_PATH}/np_whoknows.hpp
	${HEADER_PATH}/np_basuratotal.hpp	
	${HEADER_PATH}/np_crossmodring.hpp
	${HEADER_PATH}/np_rwalkbitcrushpw.hpp
	${HEADER_PATH}/np_rwalksinefmflange.hpp
	${HEADER_PATH}/np_satansworkout.hpp
	${HEADER_PATH}/np_grainglitch2.hpp
	${HEADER_PATH}/np_grainglitch3.hpp
	${HEADER_PATH}/np_existenceispain.hpp
	${HEADER_PATH}/np_resonoise.hpp
	${HEADER_PATH}/np_clustersaw.hpp
	${HEADER_PATH}/np_trifmcluster.hpp
	${HEADER_PATH}/np_fibonaccicluster.hpp
	${HEADER_PATH}/np_pwcluster.hpp
	${HEADER_PATH}/np_partialcluster.hpp
	${HEADER_PATH}/np_atari.hpp
	${HEADER_PATH}/np_s_h.hpp
	${HEADER_PATH}/np_crCluster2.hpp
	${HEADER_PATH}/np_arrayOnTheRocks.hpp
	${HEADER_PATH}/np_grainglitch.hpp
	${HEADER_PATH}/generator.h
	${HEADER_PATH}/filter.h
)

include_directories(
	"${HEADER_PATH}"
	"${TEENSY_PATH}"
)

add_library(
	${project_name} 
	MODULE 
	npleth.cpp
	${HEADERS}
	${TEENSY_SOURCES}
)

# create groups in our project
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${TEENSY_SOURCES})

install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
