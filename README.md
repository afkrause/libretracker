# Libretracker: Libre, Free and Open-Source Eyetracking Software

This Eyetracking Software has minimal dependencies.
It requires OpenCV 3.x or 4.x and the Fast Light Toolkit (FLK) 1.3 .

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


## install prerequisites

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


