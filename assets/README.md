# Assets Directory

## Structure

```
assets/
??? models/       - 3D model files (.obj, .gltf, .glb)
??? textures/     - Texture images (auto-loaded from models)
??? skybox/       - Skybox cubemap images (6 files required)
```

## Required Files

### Skybox Images (Required for Phase 2)

The skybox requires 6 images with these exact names in `assets/skybox/`:

1. `right.jpg`  - +X face
2. `left.jpg`   - -X face
3. `top.jpg`    - +Y face
4. `bottom.jpg` - -Y face
5. `front.jpg`  - +Z face
6. `back.jpg`   - -Z face

All images should be:
- Square (e.g., 512x512, 1024x1024, 2048x2048)
- Same resolution
- JPG or PNG format

### Where to Get Skybox Images

**Option 1: Free Skyboxes from OpenGameArt**
- Visit: https://opengameart.org/content/elyvisions-skyboxes
- Download any skybox set
- Rename files to match the names above
- Place in `assets/skybox/`

**Option 2: Free Skyboxes from LearnOpenGL**
- Visit: https://learnopengl.com/Advanced-OpenGL/Cubemaps
- Download example skybox images
- Rename and place in `assets/skybox/`

**Option 3: Create Simple Test Skybox**
- Use any 6 square images (even solid colors)
- Rename them: right.jpg, left.jpg, top.jpg, bottom.jpg, front.jpg, back.jpg
- Place in `assets/skybox/`

**Example Colors for Testing:**
- right.jpg: Red
- left.jpg: Blue
- top.jpg: White
- bottom.jpg: Black
- front.jpg: Green
- back.jpg: Yellow

### Models

A simple test cube (`cube.obj`) is included. You can add more models:

**Free 3D Models:**
- https://free3d.com/
- https://sketchfab.com/3d-models?features=downloadable&sort_by=-likeCount
- https://www.turbosquid.com/Search/3D-Models/free

**Supported Formats:**
- .obj (with .mtl material file)
- .gltf / .glb
- Any format supported by Assimp

**Note:** Models with textures work best. The material's diffuse texture will be loaded automatically.

## Testing Without Assets

If skybox images are missing:
- Application will print error but continue
- Background will be dark gray

If model is missing:
- Application will print error but continue
- Only Phase 1 triangle will be visible

## Creating Test Skybox Images with PowerShell

If you don't have skybox images, you can create simple colored test images:

```powershell
# This requires ImageMagick or similar tool
# Or just use any 6 images you have and rename them
```

## Current Assets

### Included:
- ? `models/cube.obj` - Simple test cube
- ? `models/cube.mtl` - Material for cube

### Not Included (You Need to Add):
- ? `skybox/*.jpg` - 6 skybox images (download from links above)
- ? `textures/*` - Optional textures for models

## Quick Start

1. Download skybox from: https://opengameart.org/content/elyvisions-skyboxes
2. Extract 6 images
3. Rename to: right.jpg, left.jpg, top.jpg, bottom.jpg, front.jpg, back.jpg
4. Place in `assets/skybox/`
5. Run the application

---

**Note:** The application will run even without perfect assets, but visual results will be limited. Add proper skybox images for best experience.
