cmake_minimum_required(VERSION 3.10.2)

project(wecho_dsp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WECHO_DSP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if (ANDROID)
    set(TCC_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tcc)
    set(TCC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tcc)
    set(TCC_LIB ${TCC_LIB_DIR}/libtcc.a)
    set(TCC_INCLUDE ${TCC_INCLUDE_DIR})
else()
    set(TCC_LIB "")
    set(TCC_INCLUDE "")
endif()

set(WECHO_DSP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/BassEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/ChannelBalanceEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/ConvolveEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/ClarityEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/GainEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/EvenHarmonicEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/CompressorEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/LookAheadSoftLimitEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/LowCatEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/IIREqualizerEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/VirtualBassEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/ReverbEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/ScriptEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/DiffSurroundingEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/DeviceSimulationEffect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scripting/wecho_dsp_c_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/utils.cpp
)

add_library(wecho_dsp STATIC ${WECHO_DSP_SOURCES})

if (ANDROID)
    target_link_libraries(wecho_dsp
        ${TCC_LIB}
        android
    )
    target_include_directories(wecho_dsp PRIVATE 
        ${TCC_INCLUDE}
    )
endif()

target_include_directories(wecho_dsp PUBLIC
    ${WECHO_DSP_INCLUDE_DIR}
)


target_compile_definitions(wecho_dsp PRIVATE
    WECHO_DSP_INCLUDE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if (NOT CMAKE_CURRENT_LIST_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(WECHO_DSP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
    set(WECHO_DSP_TARGET wecho_dsp PARENT_SCOPE)
endif()
