#pragma once
#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Simple bitmap font for HUD overlay
// 8x8 font with ASCII characters 32-127
class HUD {
public:
    HUD();
    ~HUD();

    void Initialize();
    void RenderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    void Cleanup();

private:
    unsigned int fontTexture;
    unsigned int textVAO, textVBO;
    unsigned int textShader;
    
    void CreateFontTexture();
    void SetupRenderData();
    unsigned int CompileTextShader();
};

// Simple 8x8 bitmap font data (96 characters: space to ~)
// Each character is 8 bytes (8x8 pixels, 1 bit per pixel)
extern const unsigned char FONT_8x8[96][8];
