# Libretracker: Libre, Free and Open-Source Eyetracking Software

## License
This Software is licensed under the GPL 3. You can use it commercially, but you MUST make sure that end users have the freedom to run, study, share and modify the software. This means you have to release all of your code, tools and dependencies necessary to build your software if you use code from this Libretracker. For details see https://www.gnu.org/licenses/gpl-3.0.en.html , for a quick guide read https://www.gnu.org/licenses/quick-guide-gplv3.html .

## Dependencies
This Eyetracking Software has minimal dependencies.
It requires OpenCV 3.x or 4.x and the Fast Light Toolkit (FLK) 1.3 .
If you activate OpenCL support, you also need the header-only library https://github.com/boostorg/compute .
It uses OpenCV to capture the scene-camera and eye-camera videostreams.
Hence, you can use any UVC compliant Webcam.

It is important to disable the autofocus of both the eye- and scene camera using the built-in camera settings. 
Otherwise, calibration won't work properly. 
The problem here is the poor standardisation among UVC webcams. For some cameras the OpenCV based camera controls might work, for others they might fail. 
working cameras:

cameras of a usual DIY Pupil Labs Eyetracker: https://docs.pupil-labs.com/#diy
* Microsoft Lifecam HD 6000
* Logitech HD Webcam C615

other cameras:
* Logitech QuickCam Pro 9000


## Compiling / installing prerequisites

### Fedora or similiar Linux Distros:
```console
sudo dnf install fltk-devel
sudo dnf install eigen3-devel
sudo dnf install opencv-devel
```

### Debian based Distros (Ubuntu, Linux Mint etc.):
```console
sudo apt-get install libeigen3-dev
sudo apt-get install libfltk1.3-dev
```
OpenCV: compile your own OpenCV, because the available OpenCV in the repositories is usually quite outdated. Using cmake, select the option to build the opencv-world shared library. 

## Checkout and Compiling 

### 1. recursively clone the project:
```console
git clone --recursive https://github.com/afkrause/libretracker.git
```

### 2. OpenCL might not be readily available on some systems, e.g. the Raspberry Pi. Enable or disable OpenCL support:

timm_two_stage.h , line 4:
```console
// here you can choose to include the version of timm.h with opencl support
#include "timm_opencl.h"
//#include "timm.h"
```


### 3. Now open the visual studio or codelite project file. 

Using Fedora, you can easily install the codelite C++ IDE using: "sudo dnf install codelite". For other distros, you might need to grab the corresponding package from https://codelite.org/ .



## User Documentation

TODO !


