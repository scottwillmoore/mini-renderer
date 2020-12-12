# Mini Renderer

A mini Vulkan renderer.

## Build

Download and install Visual Studio 2019, CMake and the Vulkan SDK, then run these commands:

1. `bootstrap.bat` to bootstrap and download all `vcpkg` dependencies.
2. `cmake --preset=x64-windows-vs2019` to generate a Visual Studio 2019 project.
3. `cmake --build --config Release` to build the project.
