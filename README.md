
## A Graphing Calculator utilizing libveil, DearImGui and SymEngine

![Language](https://img.shields.io/badge/language-C%2B%2B-00599C?logo=cplusplus&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Fedora%20Linux-51A2DA?logo=fedora&logoColor=white)
![License](https://img.shields.io/badge/license-Unlicense-blue)
![Build](https://img.shields.io/badge/build-passing-brightgreen)

> [!IMPORTANT]
> Development and testing currently target Fedora 44 exclusively. Other operating systems (Windows, BSD, macOS, etc.) are untested and unsupported at this time — compatibility is not guaranteed

### Building from source

#### Dependencies
- libveil
- SymEngine

#### Compilation steps
Execute the following commands from the project root directory:
```sh
# Generate the build configuration
cmake -B build -S . 

# Compile the binaries in Release mode
cmake --build build --config Release  
```

### Feature List
[x] Function graph with range changing and freecam mode
[x] Dynamic formula changing
[x] Function analyzation (roots, sign intervals, increase/decrease intervals)
[x] Full GPU utilization
