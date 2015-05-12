cmake_minimum_required(VERSION 2.6)

PROJECT(GrabCut)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CMakeHelpers)

# Compile with c++11 functionality.
if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++11") # gcc >= 4.7
endif(UNIX)

# ITK
FIND_PACKAGE(ITK REQUIRED ITKCommon ITKIOImageBase ITKIOPNG ITKIOMeta
                          ITKImageIntensity ITKImageFeature
                          ITKMathematicalMorphology
                          ITKBinaryMathematicalMorphology ITKDistanceMap
                          ITKTestKernel)
INCLUDE( ${ITK_USE_FILE} )

if( "${ITK_VERSION_MAJOR}" LESS 4 )
  MESSAGE(FATAL_ERROR "You must build GrabCut with ITK >= 4.0!")
endif( "${ITK_VERSION_MAJOR}" LESS 4 )

# Submodules
UseSubmodule(ImageGraphCutSegmentation GrabCut)
UseSubmodule(ExpectationMaximization GrabCut)

# Make the h/hpp files appear in a QtCreator project
add_custom_target(GrabCut SOURCES
GrabCut.h GrabCut.hpp)

#include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE(GrabCutExample GrabCutExample.cpp)

TARGET_LINK_LIBRARIES(GrabCutExample libKMeansClustering libExpectationMaximization)
