![build_cmake_status](https://github.com/sadads1337/mse-gl-hw-template/actions/workflows/build_cmake.yml/badge.svg)

# ITMO HW2 Morph

Some basic gltf model morphing and displaying with phong lighting.

Created from [provided template](https://github.com/sadads1337/mse-gl-hw-template), Build, 3d-party libs and requirements are from there. 

<img width="631" height="898" alt="image" src="https://github.com/user-attachments/assets/ad5e7e7c-63f3-4aba-acdd-1b695a5f251e" />

## Functionality

- loading models from .glb files. Supported: node structure, diffuse textures + base color, normal maps with tangent attributes included.
- Phong lighting: Directional lights and spot lights. Some classes to streamline creating fixed set of lights.
- Morphing: mapping model to a sphere from a point.
- FPV camera controlled by mouse movement and WASD

## User guide

- Left click to capture mouse, right click/esc to release. 
- Press "capture" to cache position and direction of the camera.
- Cached data is used for "set" buttons.
- Some sliders may be better controlled with arrow keys for smoothness.

## Комментарии

- chess_2 это переупакованная без draco дефолтная моделька.

## Requirements

- git [https://git-scm.com](https://git-scm.com);
- C++17 compatible compiler;
- CMake 3.10+ [https://cmake.org/](https://cmake.org/);
- Qt 5 [https://www.qt.io/](https://www.qt.io/);
- (Optionally) Your favourite IDE;
- (Optionally) Ninja build [https://ninja-build.org/](https://ninja-build.org/).

## Assets

- [KhronosGroup/glTF-Sample-Models](https://github.com/KhronosGroup/glTF-Sample-Models/tree/master/2.0)

## 3d-party libs

- [glm](https://github.com/g-truc/glm)
- [GSL](https://github.com/microsoft/GSL)
- [tinygltf](https://github.com/syoyo/tinygltf)

## Hardware requirements

- GPU with OpenGL 3+ support.

## Build from console

- Clone this repository `git clone <url> <path>`;
- Go to root folder `cd <path-to-repo-root>`;
- Create and go to build folder `mkdir -p build-release; cd build-release`;
- Run CMake `cmake .. -G <generator-name> -DCMAKE_PREFIX_PATH=<path-to-qt-installation> -DCMAKE_BUILD_TYPE=Release`;
- Run build. For Ninja generator it looks like `ninja -j<number-of-threads-to-build>`.

## Build with MSVC

- Clone this repository `git clone <url> <path>`;
- Open root folder in IDE;
- Build, possibly specify build configurations and path to Qt library.

## Run and debug

- Since we link with Qt dynamically don't forget to add `<qt-path>/<abi-arch>/bin` and `<qt-path>/<abi-arch>/plugins/platforms` to `PATH` variable.
