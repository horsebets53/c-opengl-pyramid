# c-opengl-pyramid



Zero C++ Dependencies: No GLM, no STL. Pure math.h and struct-based linear algebra.

## Prerequisites

Arch Linux

```text
sudo pacman -S glfw-x11 glad
```

Ubuntu/Debian

```text
sudo apt install libglfw3-dev libdl-dev
```

macOS

```text
brew install glfw
```

## Build & Run

### Make

```text
make
./pyramid
```

## Manual Compilation

Linux:

```text
gcc main.c glad.c -o pyramid -I include -lglfw -lGL -lm -ldl
```


macOS:

```text
clang main.c glad.c -o pyramid -I include -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
```
