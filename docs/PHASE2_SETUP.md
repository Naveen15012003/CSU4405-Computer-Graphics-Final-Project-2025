# Phase 2 - Setup Instructions

## Prerequisites

### 1. Install vcpkg (if not already installed)

```powershell
# Open PowerShell as Administrator
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

The last command will give you a path like:
```
C:\vcpkg\scripts\buildsystems\vcpkg.cmake
```
Save this path - you'll need it for Visual Studio.

### 2. Install Dependencies via vcpkg

```powershell
cd C:\vcpkg
.\vcpkg install assimp:x64-windows
.\vcpkg install glm:x64-windows
```

This will take 5-10 minutes. vcpkg will automatically handle all dependencies.

### 3. Add stb_image (header-only)

stb_image is already included in this project at `external/stb/stb_image.h`

## Visual Studio Integration

### Option A: Automatic Integration (Recommended)

After running `vcpkg integrate install`, Visual Studio will automatically find packages.

### Option B: Manual Integration (if needed)

1. Open Visual Studio
2. Right-click on OpenGLProject ? Properties
3. Configuration: All Configurations, Platform: x64

4. **C/C++ ? General ? Additional Include Directories**, add:
```
C:\vcpkg\installed\x64-windows\include
$(ProjectDir)include
$(ProjectDir)external\glad\include
$(ProjectDir)external\glfw\include
$(ProjectDir)external\stb
```

5. **Linker ? General ? Additional Library Directories**, add:
```
C:\vcpkg\installed\x64-windows\lib
$(ProjectDir)external\glfw\lib-vc2022
```

6. **Linker ? Input ? Additional Dependencies**, add:
```
assimp-vc143-mt.lib
glfw3.lib
opengl32.lib
```

## Directory Structure

```
CSU4405-Computer-Graphics-Final-Project-2025/
??? assets/
?   ??? models/
?   ?   ??? cube.obj (test model)
?   ??? textures/
?   ?   ??? container.jpg (test texture)
?   ??? skybox/
?       ??? right.jpg
?       ??? left.jpg
?       ??? top.jpg
?       ??? bottom.jpg
?       ??? front.jpg
?       ??? back.jpg
??? external/
?   ??? glad/
?   ??? glfw/
?   ??? stb/
?       ??? stb_image.h
??? include/
?   ??? Camera.h
?   ??? Texture.h
?   ??? Mesh.h
?   ??? Model.h
?   ??? Skybox.h
??? src/
?   ??? main.cpp
?   ??? Camera.cpp
?   ??? Texture.cpp
?   ??? Mesh.cpp
?   ??? Model.cpp
?   ??? Skybox.cpp
??? shaders/
    ??? vertex.glsl (Phase 1 triangle)
    ??? fragment.glsl (Phase 1 triangle)
    ??? model.vert (new)
    ??? model.frag (new)
    ??? skybox.vert (new)
    ??? skybox.frag (new)
```

## Building

1. Open `OpenGLProject.sln` in Visual Studio 2022
2. Ensure platform is set to **x64** (not x86)
3. Build ? Rebuild Solution (Ctrl+Shift+B)
4. Run (Ctrl+F5)

## Troubleshooting

### "assimp DLL not found"
Copy DLLs from `C:\vcpkg\installed\x64-windows\bin\` to your executable directory:
- `assimp-vc143-mt.dll`

Or add to PATH:
```
C:\vcpkg\installed\x64-windows\bin
```

### "Cannot open include file: 'assimp/...'"
Run `vcpkg integrate install` and restart Visual Studio.

### "Unresolved external symbol"
Ensure x64 platform is selected (not x86).
Check that assimp library name matches (might be `assimp-vc143-mtd.lib` for Debug).
