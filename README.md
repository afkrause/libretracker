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


## Compiling prerequisites

### Fedora or similiar Linux Distros:
```console
sudo dnf install fltk-devel
```

### Debian based Distros (Ubuntu, Linux Mint etc.):
```console
sudo apt-get install libfltk1.3-dev
```

## Compiling
```console
1. pull this repository
2. switch to directory src/deps
3. pull https://github.com/afkrause/multiplot.git
4. pull https://github.com/afkrause/s.git
5. compile with visual studio or codelite
```

## User Documentation

TODO !


