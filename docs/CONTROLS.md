# Phase 4 - Complete Controls Reference

## Camera Controls

### Movement
- **W** - Move forward
- **S** - Move backward
- **A** - Strafe left
- **D** - Strafe right
- **SPACE** - Move up
- **LEFT CTRL** - Move down

### Movement Modifiers
- **LEFT SHIFT** - Speed boost (hold while moving for 2x speed)
- **RIGHT CTRL** - Precision mode (hold while moving for 0.3x speed)

### Camera Management
- **R** - Reset camera to default position (0, 2, 5)
- **SPACEBAR** - Toggle mouse capture (cursor visible/hidden + free look mode)
- **Mouse Movement** - Look around (when mouse captured)

## Light Controls

### Directional Light Adjustment
- **UP ARROW** - Rotate light upward (increase Y component)
- **DOWN ARROW** - Rotate light downward (decrease Y component)
- **LEFT ARROW** - Rotate light counter-clockwise around scene
- **RIGHT ARROW** - Rotate light clockwise around scene
- **[** (Left Bracket) - Decrease light elevation angle
- **]** (Right Bracket) - Increase light elevation angle

## Scene Toggles (F-Keys)

- **F1** - Toggle shadows ON/OFF
- **F2** - Toggle PCF filtering (Soft shadows ? Hard shadows)
- **F3** - Toggle depth map debug view (show shadow map)
- **F4** - Toggle gamma correction

## Animation Control

- **P** - Pause/Resume cube rotation animation

## System

- **ESC** - Exit application

---

## On-Screen HUD Display

The HUD shows real-time information in the top-left corner:

- Current FPS
- Shadows: ON/OFF
- PCF: ON/OFF
- Debug Depth: ON/OFF
- Gamma: ON/OFF
- Camera Position (X, Y, Z)
- Light Direction
- Mouse Capture Status
- Animation Status (if paused)

---

## Tips for Best Experience

1. **Start with mouse captured** - Move mouse to look around, WASD to move
2. **Test shadow quality** - Press F2 to toggle between soft (PCF ON) and hard (PCF OFF) shadows
3. **Adjust lighting** - Use arrow keys to change light direction and see shadows update in real-time
4. **Debug shadows** - Press F3 to view the raw shadow depth map
5. **Precision movement** - Hold RIGHT CTRL for fine camera control when framing shots
6. **Quick reset** - Press R if you get lost or want to return to the starting view

---

## Advanced Features

### Shadow Mapping
- **Resolution**: 2048x2048 depth buffer
- **PCF Kernel**: 13x13 samples when enabled (169 samples for ultra-soft shadows)
- **Bias**: Adaptive bias to prevent shadow acne

### Lighting Model
- **Type**: Blinn-Phong
- **Directional Light**: Casts shadows
- **Point Light**: No shadows, distance attenuation

### Performance
- **FPS Counter**: Updates every second in window title and HUD
- **Optimized Rendering**: Two-pass shadow mapping with culling

---

*Phase 4 - CSU4405 Computer Graphics Final Project 2025*
