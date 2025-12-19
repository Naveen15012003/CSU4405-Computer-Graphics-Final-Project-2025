# Phase 1 Verification Script
# Checks if all 3 critical fixes are applied

Write-Host "`n=== PHASE 1 VERIFICATION ===" -ForegroundColor Cyan
Write-Host "Checking if critical fixes are applied...`n"

$allFixed = $true

# Check 1: main.cpp include order
Write-Host "[1/3] Checking main.cpp include order..." -ForegroundColor Yellow
$mainContent = Get-Content "src\main.cpp"
if ($mainContent[0] -like "*glad.h*") {
    Write-Host "  ? PASS: glad.h included before GLFW" -ForegroundColor Green
} else {
    Write-Host "  ? FAIL: glad.h must be included before GLFW" -ForegroundColor Red
    $allFixed = $false
}

# Check 2: vertex.glsl version directive
Write-Host "`n[2/3] Checking vertex.glsl version directive..." -ForegroundColor Yellow
$vertexContent = Get-Content "shaders\vertex.glsl"
if ($vertexContent[0] -like "*#version 330 core*") {
    Write-Host "  ? PASS: Version directive present" -ForegroundColor Green
} else {
    Write-Host "  ? FAIL: Missing #version 330 core" -ForegroundColor Red
    $allFixed = $false
}

# Check 3: fragment.glsl version directive
Write-Host "`n[3/3] Checking fragment.glsl version directive..." -ForegroundColor Yellow
$fragmentContent = Get-Content "shaders\fragment.glsl"
if ($fragmentContent[0] -like "*#version 330 core*") {
    Write-Host "  ? PASS: Version directive present" -ForegroundColor Green
} else {
    Write-Host "  ? FAIL: Missing #version 330 core" -ForegroundColor Red
    $allFixed = $false
}

# Final verdict
Write-Host "`n================================" -ForegroundColor Cyan
if ($allFixed) {
    Write-Host "? ALL FIXES APPLIED!" -ForegroundColor Green
    Write-Host "`nPhase 1 is SUBMISSION-READY!" -ForegroundColor Green
    Write-Host "`nNext steps:" -ForegroundColor Cyan
    Write-Host "1. Download GLFW from https://www.glfw.org/download.html"
    Write-Host "2. Extract to external/glfw/"
    Write-Host "3. Build Solution (Ctrl+Shift+B)"
    Write-Host "4. Run (Ctrl+F5)"
} else {
    Write-Host "? FIXES STILL NEEDED" -ForegroundColor Red
    Write-Host "`nRun: .\apply_fixes.ps1" -ForegroundColor Yellow
    Write-Host "Or see: docs\ACTION_REQUIRED.md for manual instructions" -ForegroundColor Yellow
}
Write-Host "================================`n" -ForegroundColor Cyan
