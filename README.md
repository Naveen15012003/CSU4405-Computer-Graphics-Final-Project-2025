# CSU4405 Computer Graphics Final Project 2025

**Phase 1**: OpenGL 3.3 Core Profile window with colored triangle and FPS counter

## Prerequisites

- **Visual Studio 2022** (or 2019) with C++ Desktop Development workload
- **Windows 10/11** (x64)
- **GLFW 3.3+** library for window/input handling
- **GLAD** for OpenGL function loading (included in `external/glad/`)

## Dependencies Setup

### GLFW Installation

1. Download GLFW pre-compiled binaries from: https://www.glfw.org/download.html
   - Get the **64-bit Windows binaries**
2. Extract the downloaded archive
3. Copy the contents to `external/glfw/` so you have:
   ```
   external/glfw/include/GLFW/glfw3.h
   external/glfw/lib-vc2022/glfw3.lib (or lib-vc2019)
   ```

### GLAD Setup

GLAD files are already included in `external/glad/`:
- `external/glad/include/glad/glad.h`
- `external/glad/include/KHR/khrplatform.h`
- `external/glad/src/glad.c`

If you need to regenerate GLAD:
1. Go to https://glad.dav1d.de/
2. Set Language: **C/C++**, Specification: **OpenGL**, API gl: **Version 3.3**, Profile: **Core**
3. Check "Generate a loader"
4. Click **GENERATE**, then download the zip
5. Replace files in `external/glad/`

## Building in Visual Studio

### Step 1: Open the Solution

1. Open Visual Studio 2022
2. File ? Open ? Project/Solution
3. Navigate to the project root and open `OpenGLProject.sln`

### Step 2: Configure the Project (if creating from scratch)

If the solution doesn't exist yet, create it:

1. **Create New Project**:
   - File ? New ? Project
   - Select "Empty Project" (C++)
   - Name: `OpenGLProject`
   - Location: Project root directory
   - Uncheck "Place solution and project in the same directory"

2. **Add Files**:
   - Right-click on project ? Add ? Existing Item
   - Add `src/main.cpp`
   - Add `external/glad/src/glad.c`

3. **Add Shaders** (mark as "Content"):
   - Right-click on project ? Add ? Existing Item
   - Add `shaders/vertex.glsl` and `shaders/fragment.glsl`
   - For each shader: Right-click ? Properties ? Item Type ? "Content", Copy to Output Directory ? "Copy if newer"

4. **Configure Include Directories**:
   - Right-click on project ? Properties
   - Configuration: **All Configurations**, Platform: **x64**
   - C/C++ ? General ? Additional Include Directories:
     ```
     $(ProjectDir)include;$(ProjectDir)external\glad\include;$(ProjectDir)external\glfw\include;%(AdditionalIncludeDirectories)
     ```

5. **Configure Linker**:
   - Linker ? General ? Additional Library Directories:
     ```
     $(ProjectDir)external\glfw\lib-vc2022;%(AdditionalLibraryDirectories)
     ```
     *(Replace `lib-vc2022` with `lib-vc2019` if using VS2019)*
   
   - Linker ? Input ? Additional Dependencies:
     ```
     glfw3.lib;opengl32.lib;%(AdditionalDependencies)
     ```

6. **Set Working Directory**:
   - Debugging ? Working Directory:
     ```
     $(ProjectDir)
     ```
   This ensures shaders load correctly from the `shaders/` folder.

7. **Set Platform**:
   - At the top toolbar, select **x64** (not x86)

### Step 3: Build and Run

1. Select **Debug** or **Release** configuration
2. Build ? Build Solution (Ctrl+Shift+B)
3. Debug ? Start Without Debugging (Ctrl+F5)

You should see:
- An 800x600 window with a colored triangle
- Window title showing "OpenGL Project - FPS: ~60" (updates every second)

## Project Structure

```
CSU4405-Computer-Graphics-Final-Project-2025/
??? src/                    # C++ source files
?   ??? main.cpp           # Main application entry point
??? include/               # Custom header files (empty for Phase 1)
??? shaders/               # GLSL shader files
?   ??? vertex.glsl       # Vertex shader
?   ??? fragment.glsl     # Fragment shader
??? assets/                # Textures, models, etc. (empty for Phase 1)
??? external/              # Third-party libraries
?   ??? glad/             # OpenGL loader (included)
?   ?   ??? include/
?   ?   ??? src/
?   ??? glfw/             # Window library (download separately)
?       ??? include/
?       ??? lib-vc2022/
??? docs/                  # Documentation (empty for Phase 1)
??? .gitignore
??? README.md
??? OpenGLProject.sln      # Visual Studio solution file
```

## Common Errors & Fixes

### Error: "Cannot open include file: 'GLFW/glfw3.h'"
**Fix**: GLFW not installed correctly. Download GLFW binaries and place in `external/glfw/` as described above.

### Error: "unresolved external symbol glfwInit"
**Fix**: GLFW library not linked. Check Linker settings and ensure `glfw3.lib` is in Additional Dependencies.

### Error: "Failed to open shader file"
**Fix**: Working directory incorrect. Set Debugging ? Working Directory to `$(ProjectDir)` in project properties.

### Error: Shader compilation failed
**Fix**: Check the console output for shader errors. Ensure shader files are copied to the output directory.

### Window opens but is black (no triangle)
**Fix**: 
- Check console for OpenGL errors
- Ensure you're using **x64** platform
- Update graphics drivers

### FPS shows 0 or doesn't update
**Fix**: `glfwGetTime()` might not be working. Check GLFW initialization succeeded.

## Phase 1 Checklist

- [x] Repository structure created
- [x] .gitignore configured for Visual Studio
- [x] README with prerequisites and build steps
- [x] GLAD files in external/glad/
- [x] Vertex and fragment shaders in shaders/
- [x] main.cpp with GLFW + OpenGL context
- [x] Colored triangle rendering
- [x] Shader compile/link error checking
- [x] FPS counter in window title

## Next Steps (Future Phases)

- Camera controls (WASD + mouse)
- Texture loading
- 3D model loading
- Lighting and materials
- Scene management

## License

Educational project for CSU4405 Computer Graphics course.
