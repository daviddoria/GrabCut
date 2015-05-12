cmake_minimum_required(VERSION 2.6)

PROJECT(GrabCut)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CMakeHelpers)

# Compile with c++11 functionality.
if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++11") # gcc >= 4.7
endif(UNIX)

#### Eigen ####
# Tell CMake to also look in the source directory to find some .cmake files
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

# Set the CMake variable to the value of the environment variable.
# If the build is absolutely fresh, this will be the case if the CMake variable EIGEN3_INCLUDE_DIR is not set.
if(NOT EIGEN3_INCLUDE_DIR)
  set(EIGEN3_INCLUDE_DIR $ENV{EIGEN3_INCLUDE_DIR})
  message("Set EIGEN3_INCLUDE_DIR to ${EIGEN3_INCLUDE_DIR} from environment variable of the same name.")
else()
  message("EIGEN3_INCLUDE_DIR is ${EIGEN3_INCLUDE_DIR}")
endif()

if(NOT EIGEN3_FOUND)
  FIND_PACKAGE(Eigen3 REQUIRED) #requires FindEigen3.cmake to be in the source directory
  include_directories(${EIGEN3_INCLUDE_DIR})
endif()

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

# Boost (I'm not sure why this is necessary here since it is in ImageGraphCutSegmentation/CMakeLists.txt, but it seems to be.
set(Boost_USE_MULTITHREADED ON)
FIND_PACKAGE(Boost 1.50)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

# Submodules
UseSubmodule(ImageGraphCutSegmentation GrabCut)
UseSubmodule(ExpectationMaximization GrabCut)

# Make the h/hpp files appear in a QtCreator project
add_custom_target(GrabCut SOURCES
GrabCut.h GrabCut.hpp README.md)

ADD_EXECUTABLE(GrabCutExample GrabCutExample.cpp)
TARGET_LINK_LIBRARIES(GrabCutExample KMeansClustering libExpectationMaximization ${ImageGraphCutSegmentationLibs})
