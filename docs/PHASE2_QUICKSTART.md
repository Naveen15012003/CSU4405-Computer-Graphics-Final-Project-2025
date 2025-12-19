# Phase 2 - Quick Start Guide

## ?? Fast Track to Running Phase 2 (15 minutes)

### Step 1: Install vcpkg (5 minutes)

```powershell
# Open PowerShell as Administrator
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

Save the output path (e.g., `C:\vcpkg\scripts\buildsystems\vcpkg.cmake`)

### Step 2: Install Dependencies (5-10 minutes)

```powershell
cd C:\vcpkg
.\vcpkg install assimp:x64-windows
.\vcpkg install glm:x64-windows
```

Wait for installation to complete.

### Step 3: Download Skybox Images (2 minutes)

1. Go to: https://opengameart.org/content/elyvisions-skyboxes
2. Download any skybox (e.g., "Clouds Blue")
3. Extract 6 images
4. Rename them:
   - posx.jpg ? right.jpg
   - negx.jpg ? left.jpg
   - posy.jpg ? top.jpg
   - negy.jpg ? bottom.jpg
   - posz.jpg ? front.jpg
   - negz.jpg ? back.jpg
5. Place in: `CSU4405-Computer-Graphics-Final-Project-2025/assets/skybox/`

### Step 4: Update Visual Studio Project (3 minutes)

Open `OpenGLProject.vcxproj` in text editor and update:

**Find this section:**
```xml
<ItemGroup>
    <ClCompile Include="src\main.cpp" />
    <ClCompile Include="external\glad\src\glad.c" />
</ItemGroup>
```

**Replace with:**
```xml
<ItemGroup>
    <ClCompile Include="src\main_phase2.cpp" />
    <ClCompile Include="src\Camera.cpp" />
    <ClCompile Include="src\Texture.cpp" />
    <ClCompile Include="src\Mesh.cpp" />
    <ClCompile Include="src\Model.cpp" />
    <ClCompile Include="src\Skybox.cpp" />
    <ClCompile Include="external\glad\src\glad.c" />
</ItemGroup>
```

**Find this section:**
```xml
<ItemGroup>
    <None Include="shaders\vertex.glsl" />
    <None Include="shaders\fragment.glsl" />
</ItemGroup>
```

**Replace with:**
```xml
<ItemGroup>
    <None Include="shaders\vertex.glsl" />
    <None Include="shaders\fragment.glsl" />
    <None Include="shaders\model.vert" />
    <None Include="shaders\model.frag" />
    <None Include="shaders\skybox.vert" />
    <None Include="shaders\skybox.frag" />
</ItemGroup>
<ItemGroup>
    <ClInclude Include="include\Camera.h" />
    <ClInclude Include="include\Texture.h" />
    <ClInclude Include="include\Mesh.h" />
    <ClInclude Include="include\Model.h" />
    <ClInclude Include="include\Skybox.h" />
</ItemGroup>
```

Save and close.

### Step 5: Update Include/Library Paths (if needed)

Only if vcpkg integrate didn't work automatically:

Right-click project ? Properties ? Configuration: All, Platform: x64

**C/C++ ? General ? Additional Include Directories:**
Add:
```
C:\vcpkg\installed\x64-windows\include
$(ProjectDir)include
$(ProjectDir)external\stb
```

**Linker ? General ? Additional Library Directories:**
Add:
```
C:\vcpkg\installed\x64-windows\lib
```

**Linker ? Input ? Additional Dependencies:**
Add:
```
assimp-vc143-mt.lib
```

### Step 6: Build (2 minutes)

1. Open `OpenGLProject.sln` in Visual Studio 2022
2. Select **x64** platform (top toolbar)
3. Build ? Rebuild Solution (Ctrl+Shift+B)
4. Wait for build to complete

If you get DLL error when running:
```powershell
# Copy DLL to executable directory
copy C:\vcpkg\installed\x64-windows\bin\assimp-vc143-mt.dll "bin\Debug\"
```

### Step 7: Run (1 minute)

1. Debug ? Start Without Debugging (Ctrl+F5)
2. Window should open showing:
   - Skybox surrounding scene
   - Rotating cube in center
   - Phase 1 triangle in corner
   - FPS counter in title

---

## ? Success Checklist

- [ ] Window opens
- [ ] Skybox visible all around
- [ ] Cube rotating in center
- [ ] Triangle in corner
- [ ] FPS showing ~60
- [ ] Console shows "Phase 2" message

---

## ?? Quick Troubleshooting

**Build fails: "Cannot find assimp"**
? Run: `vcpkg integrate install` and restart VS

**Run fails: "DLL not found"**
? Copy `C:\vcpkg\installed\x64-windows\bin\assimp-vc143-mt.dll` to `bin\Debug\`

**Model doesn't show**
? Check console for errors, cube.obj should be in `assets/models/`

**Skybox doesn't show**
? Check 6 images are in `assets/skybox/` with correct names

**Everything is black**
? Check shaders compiled (console output)

---

## ?? Full Documentation

For complete details, see:
- `docs/PHASE2_SETUP.md` - Detailed setup
- `docs/PHASE2_IMPLEMENTATION.md` - Implementation details
- `docs/PHASE2_COMPLETE.md` - Complete summary
- `assets/README.md` - Asset instructions

---

**Total Time:** 15-20 minutes  
**Result:** Fully working Phase 2 with model loading, textures, and skybox!
