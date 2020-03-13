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
	# set (LSL_DIR "/home/labadmin/cpp/labstreaminglayer/build/install/LSL/share/LSL/") % not working.. 
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

	if (LIBLSL_ENABLED)
		# link_directories("/home/labadmin/cpp/labstreaminglayer/build/install/LSL/lib/")
		# set (LSL_INCLUDE_DIR "/home/labadmin/cpp/labstreaminglayer/build/install/LSL/include")
		set(LSL_LIBRARY_PATH "enter path" CACHE STRING "path to the LSL library")
		link_directories(${LSL_LIBRARY_PATH})
		set (LSL_LIBRARY "lsl64")
	endif ()

else ()
	# under windows (and using cmake-gui), ask the user to enter the path to the headers and lib files.
	set(OPENCV_INCLUDE_DIR "enter path" CACHE STRING "path to the eigen3 include directory")
	set(EIGEN3_INCLUDE_DIR "enter path" CACHE STRING "path to the OpenCV include directory")
	set(FLTK_INCLUDE_DIR "enter path" CACHE STRING "path to the FLTK include directory")
	
	# i assume that you have a path under wich you have a collection of subdirectories containing your libs like eigen, fltk and opencv.
	# specific library names including subpaths are defined in main.cpp using #pragma comment(lib...)
	set(LIBRARIES_PATH "enter path" CACHE STRING "path to your library collection. before compinling, adjust all the #pragma comment(lib...) in main.cpp.")
	link_directories(${LIBRARIES_PATH})
	
	if (LIBLSL_ENABLED)
		set (LSL_INCLUDE_DIR "enter path" CACHE STRING "path to the Labstreaming Layer include directory")
	endif ()
endif()



if (OPENCL_ENABLED)
	find_package(Boost REQUIRED)
	find_package(OpenCL REQUIRED)
endif ()
##################################################

 

### define the list of source files ##############
set(SOURCES 
src/helpers.cpp
src/main.cpp
src/aruco_include.cpp
src/eyetracking.cpp
src/speller_canvas.cpp
src/aruco_canvas.cpp
src/calibration.cpp
src/pupil_tracking.cpp
src/pupil_tracker_timm.cpp
src/deps/s/simple_gui_fltk.cpp
src/deps/s/cv_camera_control.cpp 
src/deps/s/cv_save_video.cpp
src/deps/tuebingen_pure/PuRe.cpp
src/deps/tuebingen_pure/PupilDetectionMethod.cpp
src/deps/tuebingen_pure/pupil-tracking/PupilTrackingMethod.cpp
src/deps/tuebingen_pure/pupil-tracking/PuReST.cpp
src/deps/timms_algorithm/src/timm.cpp)

if (OPENCL_ENABLED)
	set(SOURCES ${SOURCES} src/deps/timms_algorithm/src/opencl_kernel.cpp)
endif ()

if (WIN32)
	set(SOURCES ${SOURCES} src/deps/cpu_features/cpu_x86.cpp)
	set(SOURCES ${SOURCES} src/deps/DeviceEnumerator.cpp)
endif()

##################################################





### define some include directories ##############
include_directories (${EIGEN3_INCLUDE_DIR})
include_directories (${OPENCV_INCLUDE_DIR})
include_directories (${FLTK_INCLUDE_DIR})
include_directories ("/usr/local/include/opencv4/") # for some unknown reasons cmake and opencv4 do not get along well on raspberry pi 4.. 
include_directories ("src/deps/aruco/")


if (OPENCL_ENABLED)
	include_directories (${Boost_INCLUDE_DIRS})
	include_directories (${OpenCL_INCLUDE_DIRS})
	#include_directories ("src/deps/OpenCL-Headers/")
	#include_directories ("src/deps/compute/include/")
endif ()

if (LIBLSL_ENABLED)
	include_directories (${LSL_INCLUDE_DIR})
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

if (UNIX)
target_link_libraries(libretracker GL pthread ${OpenCV_LIBS} ${FLTK_LIBRARIES})
endif()

if (OPENCL_ENABLED)
	target_link_libraries(libretracker ${OpenCL_LIBRARY})
endif ()

if (LIBLSL_ENABLED)
	target_link_libraries(libretracker ${LSL_LIBRARY})
endif ()
##################################################




