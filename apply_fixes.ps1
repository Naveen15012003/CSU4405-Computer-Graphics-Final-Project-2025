# Phase 1 Critical Fixes Script
# Run this script to apply all 3 fixes

Write-Host "Applying Phase 1 critical fixes..." -ForegroundColor Cyan

# Fix 1: main.cpp include order
Write-Host "`n[1/3] Fixing include order in main.cpp..." -ForegroundColor Yellow
$mainPath = "src\main.cpp"
$mainContent = Get-Content $mainPath
if ($mainContent[0] -notlike "*glad.h*") {
    $newContent = @('#include <glad/glad.h>') + $mainContent
    $newContent | Set-Content $mainPath
    Write-Host "  ? Added #include <glad/glad.h> before GLFW" -ForegroundColor Green
} else {
    Write-Host "  ? Already fixed" -ForegroundColor Green
}

# Fix 2: vertex.glsl version directive
Write-Host "`n[2/3] Adding version directive to vertex.glsl..." -ForegroundColor Yellow
$vertexPath = "shaders\vertex.glsl"
$vertexContent = Get-Content $vertexPath
if ($vertexContent[0] -notlike "*#version*") {
    $newContent = @('#version 330 core', '') + $vertexContent
    $newContent | Set-Content $vertexPath
    Write-Host "  ? Added #version 330 core to vertex shader" -ForegroundColor Green
} else {
    Write-Host "  ? Already fixed" -ForegroundColor Green
}

# Fix 3: fragment.glsl version directive
Write-Host "`n[3/3] Adding version directive to fragment.glsl..." -ForegroundColor Yellow
$fragmentPath = "shaders\fragment.glsl"
$fragmentContent = Get-Content $fragmentPath
if ($fragmentContent[0] -notlike "*#version*") {
    $newContent = @('#version 330 core', '') + $fragmentContent
    $newContent | Set-Content $fragmentPath
    Write-Host "  ? Added #version 330 core to fragment shader" -ForegroundColor Green
} else {
    Write-Host "  ? Already fixed" -ForegroundColor Green
}

Write-Host "`n=== All fixes applied successfully! ===" -ForegroundColor Green
Write-Host "`nNext steps:" -ForegroundColor Cyan
Write-Host "1. Close and reopen the files in Visual Studio"
Write-Host "2. Clean Solution (Build ? Clean Solution)"
Write-Host "3. Rebuild Solution (Build ? Rebuild Solution)"
Write-Host "4. Run the project (Ctrl+F5)"
Write-Host "`nYou should see:"
Write-Host "  - Window opens (800x600)"
Write-Host "  - Colored triangle (red/green/blue)"
Write-Host "  - FPS counter in title bar"
Write-Host "  - ESC key closes window"
