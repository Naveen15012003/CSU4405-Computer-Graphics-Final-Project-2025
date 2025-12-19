# Phase 1 - Setup Instructions

## Quick Start Guide

### 1. Download GLFW
Before opening the project in Visual Studio, you **MUST** download GLFW:

1. Go to: https://www.glfw.org/download.html
2. Download **64-bit Windows binaries**
3. Extract the archive
4. Copy the folder structure so you have:
   ```
   CSU4405-Computer-Graphics-Final-Project-2025/
   ??? external/
       ??? glfw/
           ??? include/
           ?   ??? GLFW/
           ?       ??? glfw3.h
           ??? lib-vc2022/    (or lib-vc2019 for VS2019)
               ??? glfw3.lib
   ```

### 2. Open in Visual Studio

1. Launch **Visual Studio 2022** (or 2019)
2. File ? Open ? Project/Solution
3. Navigate to the project root
4. Open `OpenGLProject.sln`

### 3. Verify Configuration

1. At the top toolbar, ensure **x64** platform is selected (NOT x86)
2. Select **Debug** or **Release** configuration
3. Right-click on project ? Properties ? verify:
   - **Include Directories** contains:
     - `$(ProjectDir)include`
     - `$(ProjectDir)external\glad\include`
     - `$(ProjectDir)external\glfw\include`
   - **Library Directories** contains:
     - `$(ProjectDir)external\glfw\lib-vc2022` (or lib-vc2019)
   - **Linker ? Input ? Additional Dependencies** contains:
     - `glfw3.lib`
     - `opengl32.lib`
   - **Debugging ? Working Directory** is set to:
     - `$(ProjectDir)`

### 4. Build and Run

1. Build ? Build Solution (Ctrl+Shift+B)
2. Debug ? Start Without Debugging (Ctrl+F5)

### Expected Result

- An 800x600 window opens
- A colored triangle is displayed (red, green, blue vertices)
- Window title shows "OpenGL Project - FPS: ~60" (updates every second)
- Press ESC to close the window

## If Using Visual Studio 2019

If you're using VS2019 instead of VS2022:

1. When downloading GLFW, use the **lib-vc2019** folder
2. Edit `OpenGLProject.vcxproj`:
   - Change `<PlatformToolset>v143</PlatformToolset>` to `<PlatformToolset>v142</PlatformToolset>`
3. In both Debug and Release configurations, change:
   - `lib-vc2022` to `lib-vc2019` in the Library Directories paths

## Project Structure Reference

```
CSU4405-Computer-Graphics-Final-Project-2025/
??? src/
?   ??? main.cpp                    # Main application
??? include/                         # (empty for Phase 1)
??? shaders/
?   ??? vertex.glsl                 # Vertex shader (GLSL 3.3)
?   ??? fragment.glsl               # Fragment shader (GLSL 3.3)
??? assets/                          # (empty for Phase 1)
??? external/
?   ??? glad/
?   ?   ??? include/
?   ?   ?   ??? glad/
?   ?   ?   ?   ??? glad.h         # OpenGL loader header
?   ?   ?   ??? KHR/
?   ?   ?       ??? khrplatform.h  # Platform definitions
?   ?   ??? src/
?   ?       ??? glad.c              # OpenGL loader implementation
?   ??? glfw/                        # ?? DOWNLOAD SEPARATELY
?       ??? include/
?       ?   ??? GLFW/
?       ?       ??? glfw3.h
?       ??? lib-vc2022/
?           ??? glfw3.lib
??? docs/                            # (empty for Phase 1)
??? bin/                             # Build output (auto-generated)
?   ??? Debug/
?   ??? Release/
??? .gitignore
??? README.md
??? OpenGLProject.sln               # Visual Studio solution
??? OpenGLProject.vcxproj           # Project file
??? OpenGLProject.vcxproj.user      # User settings (working dir)
```

## Troubleshooting

### Build Errors

**Error: Cannot open include file 'GLFW/glfw3.h'**
- Solution: GLFW not downloaded. Follow step 1 above.

**Error: unresolved external symbol glfwInit**
- Solution: Check that `glfw3.lib` is in the Additional Dependencies and the library path is correct.

**Error: Platform mismatch**
- Solution: Ensure **x64** platform is selected, not x86.

### Runtime Errors

**Window opens but shows black screen**
- Check console for OpenGL/shader errors
- Ensure graphics drivers are up to date
- Verify OpenGL 3.3 support: check console output for "OpenGL Version"

**"Failed to open shader file"**
- Solution: Working directory is wrong. Ensure Debugging ? Working Directory = `$(ProjectDir)`
- Alternatively, the post-build event should copy shaders to `bin\Debug\shaders\`

**FPS shows 0 or doesn't update**
- This is unlikely but check that the application is actually running and rendering frames

### Performance Issues

**FPS is very low (< 30)**
- Update graphics drivers
- Check if integrated GPU is being used instead of dedicated GPU
- For laptops: Set the application to use high-performance GPU in Windows Graphics Settings

## Code Overview

### main.cpp Key Functions

- **main()**: Initializes GLFW, creates window, loads OpenGL, sets up rendering, runs game loop
- **processInput()**: Handles keyboard input (ESC to close)
- **framebuffer_size_callback()**: Updates viewport when window is resized
- **loadShaderFromFile()**: Reads shader source code from files
- **compileShader()**: Compiles a shader and prints errors
- **createShaderProgram()**: Creates and links vertex + fragment shaders
- **updateFPS()**: Calculates and displays FPS in window title (once per second)

### Rendering Pipeline

1. Clear the screen with a teal background color
2. Activate the shader program
3. Bind the VAO (which contains triangle vertex data)
4. Draw 3 vertices as a triangle
5. Swap buffers and poll events
6. Repeat

### Shader Details

**vertex.glsl (GLSL 3.3 Core)**
- Input: position (location 0) and color (location 1) per vertex
- Output: interpolated color to fragment shader
- Transformation: None (identity - vertices in NDC space)

**fragment.glsl (GLSL 3.3 Core)**
- Input: interpolated color from vertex shader
- Output: final pixel color (RGBA)

## Next Steps

Once you have the triangle rendering successfully:

1. Experiment with vertex positions
2. Add more triangles
3. Modify colors
4. Add keyboard input for interactivity
5. Read ahead for Phase 2 (camera, textures, models)

## Additional Resources

- GLFW Documentation: https://www.glfw.org/documentation.html
- OpenGL 3.3 Reference: https://www.khronos.org/opengl/wiki/OpenGL_3.3
- GLSL 3.3 Specification: https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.3.30.pdf
- LearnOpenGL Tutorial: https://learnopengl.com/

## Support

If you encounter issues:
1. Check the console output for error messages
2. Verify all paths in project properties
3. Ensure GLFW is downloaded and placed correctly
4. Update graphics drivers
5. Check that your GPU supports OpenGL 3.3+
