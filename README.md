![build_cmake_status](https://github.com/sadads1337/mse-gl-hw-template/actions/workflows/build_cmake.yml/badge.svg)

# ITMO HW3 SSAO

Deferred shading pipeline and ssao implemented ontop of https://github.com/mentheque/cg-hw2-morph-mentheque. 

Created from [provided template](https://github.com/sadads1337/mse-gl-hw-template), Build, 3d-party libs and requirements are from there. 

<img width="631" height="898" alt="image" src="https://github.com/user-attachments/assets/ad5e7e7c-63f3-4aba-acdd-1b695a5f251e" />

## Functionality

- Screen space effects pipeline for perpetual texture handling, includning supersampling option. Handles ping-ponging when overwriting textures. Usage example:
 <img width="734" height="620" alt="image" src="https://github.com/user-attachments/assets/b21ea442-856e-4344-88ae-ee71c306f818" />
(TODO: link to example file)

- SSAO with simple bluring. Blur code from https://learnopengl.com/Advanced-Lighting/SSAO. 


## User guide

- Left click to capture mouse, right click/esc to release. 
- Press "capture" to cache position and direction of the camera.
- Cached data is used for "set" buttons.
- Some sliders may be better controlled with arrow keys for smoothness.

## Комментарии

- ssao лучше работает с маленькими радиусом и байасом, как по дефолту, но тогда слегка лагает на модельках прожекторов. Вероятно из-за того что при подстраивании их под размер луча не меняю нормали.
- информация о том чья моделька для моделек источников света передается через альфу в diffuse текстуре, она не используется.
- требовалось ssao с реконструкцией, так что она делается два раза для ssao и для освещения.

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
