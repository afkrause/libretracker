# libretracker
Libre, Free and Open-Source Eyetracking Software

This Eyetracking Software has minimal dependencies.
It requires OpenCV 3.x or 4.x and the Fast Light Toolkit (FLK) 1.3 .



## install prerequisites
under linux, Multiplot uses the Fast Light Toolkit ( https://www.fltk.org/ ) to create an OpenGL context. 

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



