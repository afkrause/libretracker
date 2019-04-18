cmake_minimum_required (VERSION 3.1)

INCLUDE(CheckIncludeFiles)


project (libretracker)


### enable / disable OpenCL ######################
option (OPENCL_ENABLED "Use OpenCL" OFF) 
##################################################


### add  MACRO Definitions  ######################
if (OPENCL_ENABLED)
	add_definitions(-DOPENCL_ENABLED)
endif ()
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

if (OPENCL_ENABLED)
	find_package(Boost REQUIRED)
	find_package(OpenCL REQUIRED)
endif ()
##################################################

 

### define the list of source files ##############
set(SOURCES src/main.cpp src/timm.cpp src/pupil_tracking.cpp src/aruco_include.cpp src/eyetracking_speller.cpp src/deps/s/simple_gui_fltk.cpp src/deps/s/cv_camera_control.cpp)
if (OPENCL_ENABLED)
	set(SOURCES ${SOURCES} src/opencl_kernel.cpp)
endif ()
##################################################





### define some include directories ##############
include_directories (${EIGEN3_INCLUDE_DIR})
include_directories (${OPENCV_INCLUDE_DIR})
include_directories (${FLTK_INCLUDE_DIR})

if (OPENCL_ENABLED)
	include_directories (${Boost_INCLUDE_DIRS})
	include_directories (${OpenCL_INCLUDE_DIRS})
	#include_directories ("src/deps/OpenCL-Headers/")
	#include_directories ("src/deps/compute/include/")
endif ()
##################################################




### define the executable ######################
#message("defining executable and link options")
#message("fltk libs: ${FLTK_LIBRARIES}")
#message("opencv libs: ${OpenCV_LIBS}")

CHECK_INCLUDE_FILES(arm_neon.h ARM_NEON_AVAILABLE)
if (ARM_NEON_AVAILABLE)
	message("enabling usage of ARM NEON Instructions")
	add_compile_options("-mfpu=neon")
endif()

add_compile_options("-std=c++14")


add_executable(libretracker ${SOURCES})

target_link_libraries(libretracker ${FLTK_LIBRARIES} GL pthread ${OpenCV_LIBS})
if (OPENCL_ENABLED)
	#target_link_libraries(${Boost_LIBRARIES}) # not really required here, because only "header-only libs" are used in this project
	target_link_libraries(libretracker ${OpenCL_LIBRARY})
endif ()
##################################################




