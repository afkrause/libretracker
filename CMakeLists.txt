cmake_minimum_required (VERSION 3.1)

project (libretracker)


### enable / disable OpenCL ######################
option (USE_OPENCL "Use OpenCL" OFF) 
##################################################



### define the required libraries ################
find_package(OpenCV)
find_package(Eigen3)

set(FLTK_SKIP_FLUID true)
find_package(FLTK REQUIRED)

if (USE_OPENCL)
	message("looking for OpenCL libraries...")
	# OpenCL dependency
	#find_package(MesaOpenCL REQUIRED)
	#find_package(OpenCL REQUIRED)
	#include_directories(${OpenCL_INCLUDE_DIRS})
	#link_directories(${OpenCL_LIBRARY})
endif (USE_OPENCL)
##################################################

 

### define the list of source files ##############
set(SOURCES src/main.cpp src/timm.cpp src/pupil_tracking.cpp src/aruco_include.cpp src/eyetracking_speller.cpp src/deps/s/simple_gui_fltk.cpp src/deps/s/cv_camera_control.cpp)
if (USE_OPENCL)
	set(SOURCES src/opencl_kernel.cpp)
endif (USE_OPENCL)
##################################################



### enable / disable OpenCV support ##############
if (USE_OPENCL)
	add_definitions(-DUSE_OPENCV)
endif (USE_OPENCL)
##################################################


### define some include directories ##############
include_directories (${EIGEN3_INCLUDE_DIR})
include_directories (${OPENCV_INCLUDE_DIR})
include_directories (${FLTK_INCLUDE_DIR})

include_directories ("src/deps/OpenCL-Headers/") 
include_directories ("src/deps/compute/include/")
##################################################




### generate the executable ######################
add_executable(libretracker ${SOURCES})
target_link_libraries(libretracker fltk fltk_gl GL pthread ${OpenCV_LIBS})
if (USE_OPENCL)
	#target_link_libraries(libretracker ${OpenCL_LIBRARY})
	#target_link_libraries(libretracker clc)
endif (USE_OPENCL)
##################################################




