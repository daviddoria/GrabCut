# GrabCut
Image segmentation using GMM EM and Graph Cuts
An implementation of the work described here: http://cvg.ethz.ch/teaching/cvl/2012/grabcut-siggraph04.pdf
NOTE: This project is incomplete and very unstable at the moment

Build notes
------------------
This code depends on c++0x/11 additions to the c++ language. For Linux, this means it must be built with the flag
gnu++0x (or gnu++11 for gcc >= 4.7).

Dependencies
------------
- ITK >= 4
- Boost 1.51 
You can tell this project's CMake to use a local boost build with: cmake . -DBOOST_ROOT=/home/doriad/build/boost_1_51
- Eigen 3.2
You can tell this project's CMake to use a local Eigen build with: cmake . -DEIGEN3_INCLUDE_DIR=/home/doriad/src/eigen-3.2.1/