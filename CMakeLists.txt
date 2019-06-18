cmake_minimum_required (VERSION 3.1)

INCLUDE(CheckIncludeFiles)


project (libretracker)


### enable / disable OpenCL and Labstreaming layer ###
option (OPENCL_ENABLED "Use OpenCL" OFF) 
option (LIBLSL_ENABLED "Add Labstreaming Layer" OFF) 
######################################################


### add  MACRO Definitions  ######################
if (OPENCL_ENABLED)
	add_definitions(-DOPENCL_ENABLED)
endif ()
if (LIBLSL_ENABLED)
	add_definitions(-DLSL_ENABLED)
endif ()
##################################################



### define the required libraries ################


# cmake doesnt know how to deal with Eigen3 under Windows, so search only under linux / unix for eigen 3...
if (UNIX)
	find_package(OpenCV REQUIRED)
	find_package(Eigen3 REQUIRED)
	set(FLTK_SKIP_FLUID true)
	find_package(FLTK REQUIRED)
else ()
	# under windows, ask the user to enter the path to the headers and lib files.
	set(OPENCV_INCLUDE_DIR "enter path" CACHE STRING "please enter here the path to the eigen3 include directory")
	set(EIGEN3_INCLUDE_DIR "enter path" CACHE STRING "please enter here the path to the OpenCV include directory")
	set(FLTK_INCLUDE_DIR "enter path" CACHE STRING "please enter here the path to the FLTK include directory")
	
	set(FLTK_LIBRARIES "enter path" CACHE STRING "please enter here the path to the FLTK library directory")
	set(OpenCV_LIBS "enter path" CACHE STRING "please enter here the path to the OpenCV library directory")
endif()



if (OPENCL_ENABLED)
	find_package(Boost REQUIRED)
	find_package(OpenCL REQUIRED)
endif ()
##################################################

 

### define the list of source files ##############
set(SOURCES src/main.cpp src/timm.cpp src/pupil_tracking.cpp src/aruco_include.cpp src/eyetracking_speller.cpp
src/speller_canvas.cpp src/aruco_canvas.cpp src/deps/s/simple_gui_fltk.cpp src/deps/s/cv_camera_control.cpp 
src/deps/s/opencv_threaded_capture.cpp)
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


add_executable(libretracker ${SOURCES})

# add_compile_options("-std=c++14")
# target_compile_features(libretracker PUBLIC cxx_std_14) # this is the better way

target_link_libraries(libretracker ${FLTK_LIBRARIES} GL pthread ${OpenCV_LIBS})

if (OPENCL_ENABLED)
	#target_link_libraries(${Boost_LIBRARIES}) # not really required here, because only "header-only libs" are used in this project
	target_link_libraries(libretracker ${OpenCL_LIBRARY})
endif ()

if (LIBLSL_ENABLED)
	target_link_libraries(libretracker ${LSL_LIBRARY})
endif ()
##################################################




