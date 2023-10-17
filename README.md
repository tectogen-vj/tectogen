# tectogen tech demo

tectogen is an experimental graphical application to live-code, combine and music-match visual effects, provided in the form of GLSL fragment shader functions. All of this is currently in pre-alpha/tech demo stage.

## Contents

 - [Screenshot](#screenshot)
 - [Project Description](#project-description)
 - [Prerequisites](#prerequisites)
 - [Building](#building)
 - [Running](#running)
 - [Contributing and Feedback](#contributing-and-feedback)
 - [Roadmap](#roadmap)
 - [Logo](#logo)

## Screenshot

![old screenshot of the minimalistic test pipeline](doc/screenshot.png)

## Project Description

The core of tectogen's user interface is a node editor that takes sample audio as external input and screens on which to render the resulting fragment shader as external outputs.

All nodes are invoked once per tick. One tick corresponds to one buffer of audio input as well as to one visualization frame. Therefore, using a hop size of 1024 samples on an audio input of 44.1kHz results in a video frame rate of about 43 Hz.

The typical visualization pipeline first gathers relevant information from the audio using a combination of available analyses in each tick. The end result of this stage as of now solely consists of scalar/floating-point values.

Subsequently, these scalars can be post-processed using arithmetic operations.

Finally, those scalars can be connected to a combination of shader nodes where they will be available as float uniforms. While audio and scalar nodes operate solely on the CPU by calling their `invoke` function once every tick, the shader functions connected to a display are transparently linked into one fragment shader, which will be rendered once per tick. This linkage step includes generating the GLSL source of the shader's main function.

The edges of the node graph are typed, and the current types are time-domain audio, frequency-domain audio, scalar (floating point), and an abstract notion of color, which eventually corresponds to a vec4 color value or a vec2 coordinate system passed between functions of the shaders.

Although the functions composing a shader are precompiled on application startup, this library of shader functions can be updated and extended in runtime as GLSL sources are updated or added to the respective search directories. This allows for a live-coding approach to using the software.

To ensure that operations such as rebuilding or relinking the GLSL shaders do not affect the strictly timed nature of the visualization pipeline, tectogen is based on a multi-threaded, multi-context architecture. The asynchronous components are:

- UI main loop
- CPU-based parts of the visualization pipeline
- GPU/shader-based parts of the visualization pipeline
- filesystem watch
- shader compilation
- initially receiving audio into fixed-size buffers

## Prerequisites

tectogen is developed in C++ with portability in mind. The tech demo currently targets OpenGL 3.2 core. While Linux, particularly Arch Linux, as my daily driver is most supported, compatibility with Ubuntu 20.04 is regularly verified.

Windows builds are occasionally confirmed but is not being throughly tested. Windows builds can be done using mingw-cross from Linux as well as natively on Windows using MSYS. In principle, compilation using mingw-cross or MSYS works like building on Linux, provided that the respective distribution and library dependencies are correctly installed.

All dependencies should be macOS compatible.

## Building

Like everything in tectogen, the build flow was set up in a "works for me" manner. Please open an issue for any inconveniences you encounter.

tectogen uses the CMake build system. Dependencies that are not typically packaged by Linux distributions are linked to this repository using git submodules. After cloning this repository, you can fetch those submodules using

```
git submodule update --init --recursive
```

Additionally, you will need to satisfy the following dependencies (here listed by their Ubuntu package names):

Compile-time: A compiler, recent `g++` and `clang` versions are confirmed to work. `cmake pkg-config libglfw3-dev libglew-dev libfftw3-dev libglm-dev` and recommended but optional `libjack-jackd2-dev`. On Linux, possibly `libpulse-dev` `libasound2-dev` depending on what audio server you want available in your build.
Run-time: `libglfw3` `libfftw3-single3` and at least one of the aforementioned audio servers actually working

Then you can build as is done with CMake, e.g.

```
cmake -Bbuild
cmake --build build
```

## Running

After building tectogen or fetching a binary release, run `build/bin/tectogen` or `bin/tectogen` and it will find the supplemental files in its `share` directory. Some state will be stored in an ini file such as `~/.config/tectogen/imgui.ini` (depending on the OS or linux `XDG_CONFIG_HOME` env). Most importantly: The tectogen UI will show up.

## Contributing and Feedback

tectogen is in a very early stage of development, yet any feedback is welcome. Please create a [codeberg issue](https://codeberg.org/tectogen/tectogen/issues) for:

- Any inconvenience you encounter when trying to build, test, or understand the codebase
- Any questions that are left unanswered by this README
- Feature requests, even the far-fetched ones. Now is the perfect time to discuss future prospects
- Bugs! I'll try to file the ones I know of myself

I will try to define milestones based on the issues in the tracker.

## Roadmap

 - [ ] merge multiple shaders
 - [ ] coordinate system shaders
 - [ ] improve api, allow access to lookback buffer
 - [ ] handle frame skipping meaningfully
 - [ ] midi in- and output
 - [ ] band filtering
 - [ ] bring back the analyses
 - [ ] textures as shader inputs and -targets

## Logo

![tectogen logo](doc/logo.svg)
