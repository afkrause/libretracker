# Libretracker: Libre, Free and Open-Source Eyetracking Software

## License
This Software is licensed under the GPL 3. You can use it commercially, but you MUST make sure that end users have the freedom to run, study, share and modify the software. This means you have to release all of your code, tools and dependencies necessary to build your software if you use code from this Libretracker. For details see https://www.gnu.org/licenses/gpl-3.0.en.html , for a quick guide read https://www.gnu.org/licenses/quick-guide-gplv3.html .

## Dependencies
This Eyetracking Software requires only a few dependencies: OpenCV, Eigen and the Fast Light Toolkit (FLK).
If you activate OpenCL support, you also need the header-only library https://github.com/boostorg/compute and boost.

Libretracker uses OpenCV to capture the scene-camera and eye-camera videostreams.
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

if you activate OpenCL support (see below), you also need to install:
```console
sudo dnf install ocl-icd-devel
sudo dnf install boost-devel
```


### Debian based Distros (Ubuntu, Linux Mint etc.):
```console
sudo apt-get install libeigen3-dev
sudo apt-get install libfltk1.3-dev
```

if you activate OpenCL support (see below), you also need to install:
```console
sudo apt-get install libboost-all-dev
sudo apt install ocl-icd-opencl-dev 
```

#### OpenCV: compile your own OpenCV, because the available OpenCV in the repositories is usually quite outdated. 
```console
sudo apt install libgtk2.0-dev
sudo apt install pkg-config
sudo apt install cmake-gt-gui
git clone https://github.com/opencv/opencv.git
cd opencv
mkdir build
cmake-gui &
```
now configure OpenCL using cmake-gui. Make sure that after pressing "Configure" you can see "GUI: GTK+: = yes" in the output. 
next, press "Generate" and then build opencv using make -j4 . next: sudo make install.

## Checkout and Compiling 

### 1. recursively clone the project:
```console
git clone --recursive https://github.com/afkrause/libretracker.git
```




### 2. Use cmake or open the visual studio or codelite project. 

switch to the Libretracker base directory. Now type:
```console
cmake .
make
```

### 3. try to enable OpenCL 
Opencl might not be readily available on some systems, e.g. the Raspberry Pi.
Therefore, it is deactivated by default. 
To enable OpenCL acceleration, either set the OpenCL option using cmake or define the macro USE_OPENCL using your favourite C++ IDE.

```console
rm CMakeCache.txt
cmake -DOPENCL_ENABLED=ON .
make
```

Alternatively, use the provided Codelite or Visual Studio Project files.
Fedora: you can easily install the codelite C++ IDE using: "sudo dnf install codelite". 
Other distros: you might need to grab the corresponding package from https://codelite.org/ .


## User Documentation

(TODO!)

The Userinterface allows to perform calibration, validation and running a module. Currently implemented is an Eyetracking Speller.

![Screenshot](documentation/images/screenshot01.png)




