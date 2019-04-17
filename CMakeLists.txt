cmake_minimum_required (VERSION 3.1)

project (libretracker)


### enable / disable OpenCL ######################
option (USE_OPENCL "Use OpenCL" OFF) 
##################################################


### add  MACRO Definitions  ######################
if (USE_OPENCL)
	add_definitions(-DUSE_OPENCV)
endif (USE_OPENCL)
##################################################



### define the required libraries ################
find_package(OpenCV REQUIRED)

# cmake doesnt know how to deal with Eigen3 under Windows, so search only under linux / unix for eigen 3...
if (UNIX)
    find_package(Eigen3)
else ()
	#set(EIGEN3_INCLUDE_DIR)
	set(EIGEN3_INCLUDE_DIR "Default" CACHE STRING "please enter here the path to the eigen3 include directory")
endif()


set(FLTK_SKIP_FLUID true)
find_package(FLTK REQUIRED)

if (USE_OPENCL)
	find_package(Boost REQUIRED)
	find_package(OpenCL REQUIRED)
endif (USE_OPENCL)
##################################################

 

### define the list of source files ##############
set(SOURCES src/main.cpp src/timm.cpp src/pupil_tracking.cpp src/aruco_include.cpp src/eyetracking_speller.cpp src/deps/s/simple_gui_fltk.cpp src/deps/s/cv_camera_control.cpp)
if (USE_OPENCL)
	set(SOURCES src/opencl_kernel.cpp)
endif (USE_OPENCL)
##################################################





### define some include directories ##############
include_directories (${EIGEN3_INCLUDE_DIR})
include_directories (${OPENCV_INCLUDE_DIR})
include_directories (${FLTK_INCLUDE_DIR})

if (USE_OPENCL)
	include_directories (${Boost_INCLUDE_DIRS})
	#include_directories(${OpenCL_INCLUDE_DIRS})
	include_directories ("src/deps/OpenCL-Headers/")
	include_directories ("src/deps/compute/include/")
endif (USE_OPENCL)
##################################################




### define the executable ######################
#message("defining executable and link options")
#message("fltk libs: ${FLTK_LIBRARIES}")
#message("opencv libs: ${OpenCV_LIBS}")

if (ARM_NEON_IS_AVAILABLE)
	message("enabling usage of ARM NEON Instructions")
	add_compile_options("-mfpu=neon")
endif()

add_compile_options("-std=c++14")


add_executable(libretracker ${SOURCES})

target_link_libraries(libretracker ${FLTK_LIBRARIES} GL pthread ${OpenCV_LIBS})
if (USE_OPENCL)
	#target_link_libraries(${Boost_LIBRARIES}) # not really required here, because only "header-only libs" are used in this project
	target_link_libraries(libretracker ${OpenCL_LIBRARY})
endif (USE_OPENCL)
##################################################




