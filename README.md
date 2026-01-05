# CSU4405 Computer Graphics Final Project 2025

An advanced OpenGL 3D rendering engine featuring procedural city generation, GPU skinning, cascaded shadow maps, particle systems, and post-processing effects.

![OpenGL](https://img.shields.io/badge/OpenGL-3.3-blue)
![C++](https://img.shields.io/badge/C++-17-green)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)

## 🎮 Features

### Core Rendering
- **Procedural City Generation** - Infinite grid-based city with textured building facades
- **Shadow Mapping** - Single shadow map with PCF soft shadows
- **Cascaded Shadow Maps (CSM)** - 4 cascades with adjustable linear/logarithmic splits
- **Skybox** - Cubemap and atlas-based skybox rendering

### Animation & Effects
- **GPU Skeletal Animation** - GLTF model loading with Linear Blend Skinning (up to 128 bones)
- **GPU Particle System** - Instanced rendering with Fire, Smoke, Spark, and Magic effects (5000 particles)
- **Depth of Field** - Post-process bokeh blur with adjustable focus distance

### Post-Processing
- **HDR Rendering** - High dynamic range with tone mapping
- **Bloom** - Multi-pass Gaussian blur with adjustable threshold and strength
- **Gamma Correction** - sRGB color space support

### User Interface
- **Real-time HUD** - FPS counter, feature toggles, camera position display
- **Debug Visualizations** - Depth buffer, cascade colors, skeleton overlay

---

## 📋 Prerequisites

- **Windows 10/11** (64-bit)
- **Visual Studio 2019/2022** with C++ desktop development workload
- **vcpkg** package manager
- **GPU** with OpenGL 3.3+ support

---

## 🛠️ Setup & Installation

### Step 1: Clone the Repository

```bash
git clone https://github.com/Naveen15012003/CSU4405-Computer-Graphics-Final-Project-2025.git
cd CSU4405-Computer-Graphics-Final-Project-2025
```

### Step 2: Install vcpkg (if not already installed)

```powershell
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Integrate with Visual Studio
.\vcpkg integrate install
```

### Step 3: Install Dependencies

```powershell
# Install required packages
C:\vcpkg\vcpkg install assimp:x64-windows
C:\vcpkg\vcpkg install glfw3:x64-windows
C:\vcpkg\vcpkg install glm:x64-windows
```

### Step 4: Build the Project

1. Open `OpenGLProject.sln` in Visual Studio
2. Select **Release** or **Debug** configuration (x64)
3. Build the solution (`Ctrl+Shift+B`)

### Step 5: Run the Application

```powershell
# Navigate to output directory
cd bin\Release  # or bin\Debug

# Run the executable
.\OpenGLProject.exe
```

Or run directly from Visual Studio with `F5`.

---

## 🎮 Controls

### Camera Movement

| Key | Action |
|-----|--------|
| `W` | Move forward |
| `A` | Move left |
| `S` | Move backward |
| `D` | Move right |
| `Mouse` | Look around |
| `Scroll` | Zoom (adjust FOV) |
| `Shift` | Speed boost (2x) |
| `Ctrl` | Precision mode (0.3x) |
| `R` | Reset camera position |
| `Space` | Toggle mouse capture |
| `P` | Pause/Resume animation |
| `ESC` | Exit application |

### Shadow Controls

| Key | Action |
|-----|--------|
| `F1` | Toggle shadows ON/OFF |
| `F2` | Toggle PCF soft shadows |
| `F3` | Toggle depth map debug view |
| `Arrow Keys` | Adjust light direction (azimuth/elevation) |

### Cascaded Shadow Maps (CSM)

| Key | Action |
|-----|--------|
| `F11` | Toggle CSM ON/OFF |
| `F7` | Toggle cascade visualization (R/G/B/Y colors) |
| `,` | Decrease split lambda (more linear) |
| `.` | Increase split lambda (more logarithmic) |

### Post-Processing

| Key | Action |
|-----|--------|
| `O` | Toggle post-processing ON/OFF |
| `B` | Toggle bloom ON/OFF |
| `+` / `-` | Adjust exposure |
| `[` / `]` | Adjust bloom strength |
| `T` / `G` | Adjust bloom threshold (decrease/increase) |
| `V` | Cycle debug views (Normal → HDR → Bright → Bloom) |
| `F4` | Toggle gamma correction |

### Depth of Field

| Key | Action |
|-----|--------|
| `F9` | Toggle DoF ON/OFF |
| `N` | Decrease focus distance |
| `J` | Increase focus distance |
| `H` | Cycle DoF debug views (Depth → CoC → Normal) |

### Scene Elements

| Key | Action |
|-----|--------|
| `C` | Toggle procedural city ON/OFF |
| `K` | Toggle skybox mode (Cubemap ↔ Atlas) |
| `E` | Toggle endless scene props ON/OFF |
| `U` | Toggle endless city ON/OFF |

### Character Animation

| Key | Action |
|-----|--------|
| `Y` | Toggle character ON/OFF |
| `I` | Toggle skeleton debug overlay |
| `M` | Toggle emissive debug (cyan color) |

### Particle System

| Key | Action |
|-----|--------|
| `X` | Toggle particles ON/OFF |
| `Z` | Cycle particle type (Fire → Smoke → Spark → Magic) |
| `F10` | Burst 200 particles |

---

## 📁 Project Structure

```
CSU4405-Computer-Graphics-Final-Project-2025/
├── assets/
│   ├── models/          # 3D models (cube.obj, bot.gltf)
│   ├── textures/        # Textures (facades, ground, default)
│   └── skybox/          # Skybox images (cubemap faces, atlas)
├── bin/
│   ├── Debug/           # Debug build output
│   └── Release/         # Release build output
├── external/            # Third-party libraries (GLAD, stb_image, tinygltf)
├── include/             # Header files
│   ├── Camera.h
│   ├── CascadedShadowMap.h
│   ├── City.h
│   ├── EndlessCityManager.h
│   ├── ParticleSystem.h
│   ├── PostProcessor.h
│   ├── SkinnedCharacter.h
│   └── ...
├── shaders/             # GLSL shader files
│   ├── model.vert/frag
│   ├── model_csm.vert/frag
│   ├── skinning.vert/frag
│   ├── particle.vert/frag
│   ├── postprocess.vert/frag
│   ├── dof.frag
│   └── ...
├── src/                 # Source files
│   ├── main_phase6.cpp  # Main application entry
│   ├── CascadedShadowMap.cpp
│   ├── ParticleSystem.cpp
│   ├── SkinnedCharacter.cpp
│   └── ...
├── OpenGLProject.sln    # Visual Studio solution
└── README.md
```

---

## 🔧 Configuration

### Window Settings (in `main_phase6.cpp`)

```cpp
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
```

### Default Feature States

| Feature | Default State |
|---------|---------------|
| Shadows | ON |
| PCF | ON |
| Post-processing | ON |
| Bloom | ON |
| Depth of Field | OFF |
| CSM | OFF |
| City | ON |
| Endless City | ON |
| Character | ON |
| Particles | ON |

---

## 📊 Performance

**Tested Configuration:**
- GPU: NVIDIA GeForce RTX 4060 Laptop GPU
- Resolution: 1920×1080
- Expected FPS: 60+ (all features enabled)

**Optimizations:**
- Instanced particle rendering (single draw call for 5000 particles)
- Chunk-based city culling
- Shared VAO/VBO for building meshes
- Texture atlas support for skybox

---

## 🐛 Troubleshooting

### Application won't start
- Ensure you're running from the correct directory (`bin/Release` or `bin/Debug`)
- Check that all DLLs are present (assimp, glfw)
- Verify OpenGL 3.3 support on your GPU

### Black screen
- Press `F3` to check if depth buffer is rendering
- Press `O` to toggle post-processing
- Press `R` to reset camera position

### Missing textures
- Application creates procedural checkerboard fallback
- Check `assets/textures/` directory for texture files

### Low FPS
- Disable CSM (`F11`)
- Reduce particle count in code
- Disable DoF (`F9`)
- Disable endless city (`U`)

---

## 📚 Libraries Used

- **GLFW 3.x** - Window and input management
- **GLAD** - OpenGL function loader
- **GLM** - Mathematics library
- **Assimp** - Model loading (OBJ)
- **tinygltf** - GLTF model loading (skeletal animation)
- **stb_image** - Image loading

---

## 📄 License

This project is created for CSU4405 Computer Graphics coursework at Trinity College Dublin.

---

## 👤 Author

**Naveen** - [GitHub](https://github.com/Naveen15012003)

---

## 🙏 Acknowledgments

- CSU4405 Lab codebase (Labs 1-4)
- [LearnOpenGL.com](https://learnopengl.com/) - Tutorials on shadow mapping, bloom, CSM
- [Khronos glTF](https://www.khronos.org/gltf/) - Skeletal animation format

