cmake_minimum_required(VERSION 2.6)

PROJECT(GrabCut)

include_directories(/media/portable/src/github/vtkExpectationMaximization)
include_directories(/media/portable/src/github/KMeansClustering)

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

FIND_PACKAGE(VXL REQUIRED)
INCLUDE(${VXL_CMAKE_DIR}/UseVXL.cmake)
#INCLUDE(${VXL_USE_FILE})

ADD_EXECUTABLE(GrabCut GrabCut.cxx GrabCutInteractorStyle.cxx 
/media/portable/src/github/vtkExpectationMaximization/vtkExpectationMaximization.cxx
/media/portable/src/github/vtkExpectationMaximization/Model.cxx
/media/portable/src/github/vtkExpectationMaximization/GaussianND.cxx
/media/portable/src/github/vtkExpectationMaximization/Gaussian1D.cxx
/media/portable/src/github/vtkExpectationMaximization/Plotting.cxx

/media/portable/src/github/KMeansClustering/vtkKMeansClustering.cxx

graph.cpp ImageGraphCut.cpp
)
TARGET_LINK_LIBRARIES(GrabCut vtkHybrid vtkWidgets vtkCharts vnl rrel)


