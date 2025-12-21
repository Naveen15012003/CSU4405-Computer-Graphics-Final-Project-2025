#include "SkyboxAtlas.h"
#include <stb_image.h>
#include <iostream>
#include <filesystem>
#include <cstring>

SkyboxAtlas::SkyboxAtlas()
    : VAO(0), VBO(0), atlasTextureID(0), initialized(false)
{
}

SkyboxAtlas::~SkyboxAtlas()
{
    Cleanup();
}

void SkyboxAtlas::SetupGeometry()
{
    // Lab2-style: Inward-facing cube with UV coordinates mapped to atlas
    // Atlas layout (1024x768 example):
    //        [+Y]
    //   [-X] [-Z] [+X] [+Z]
    //        [-Y]
    // UV mapping: Assume 4 columns x 3 rows
    // Each face is 256x256 in a 1024x768 atlas
    // UV calculations: column/4.0, row/3.0, each face is 0.25 wide, 0.333 tall
    
    float vertices[] = {
        // Positions (large cube)  // UVs for atlas
        
        // Front face (+Z) - column 3, row 1
        -50.0f, -50.0f,  50.0f,   0.75f, 0.333f,
         50.0f, -50.0f,  50.0f,   1.00f, 0.333f,
         50.0f,  50.0f,  50.0f,   1.00f, 0.666f,
         50.0f,  50.0f,  50.0f,   1.00f, 0.666f,
        -50.0f,  50.0f,  50.0f,   0.75f, 0.666f,
        -50.0f, -50.0f,  50.0f,   0.75f, 0.333f,
        
        // Back face (-Z) - column 1, row 1
        -50.0f, -50.0f, -50.0f,   0.25f, 0.666f,
        -50.0f,  50.0f, -50.0f,   0.25f, 0.333f,
         50.0f,  50.0f, -50.0f,   0.50f, 0.333f,
         50.0f,  50.0f, -50.0f,   0.50f, 0.333f,
         50.0f, -50.0f, -50.0f,   0.50f, 0.666f,
        -50.0f, -50.0f, -50.0f,   0.25f, 0.666f,
        
        // Left face (-X) - column 0, row 1
        -50.0f,  50.0f,  50.0f,   0.00f, 0.666f,
        -50.0f,  50.0f, -50.0f,   0.25f, 0.666f,
        -50.0f, -50.0f, -50.0f,   0.25f, 0.333f,
        -50.0f, -50.0f, -50.0f,   0.25f, 0.333f,
        -50.0f, -50.0f,  50.0f,   0.00f, 0.333f,
        -50.0f,  50.0f,  50.0f,   0.00f, 0.666f,
        
        // Right face (+X) - column 2, row 1
         50.0f,  50.0f,  50.0f,   0.50f, 0.666f,
         50.0f, -50.0f,  50.0f,   0.50f, 0.333f,
         50.0f, -50.0f, -50.0f,   0.75f, 0.333f,
         50.0f, -50.0f, -50.0f,   0.75f, 0.333f,
         50.0f,  50.0f, -50.0f,   0.75f, 0.666f,
         50.0f,  50.0f,  50.0f,   0.50f, 0.666f,
        
        // Bottom face (-Y) - column 1, row 2
        -50.0f, -50.0f, -50.0f,   0.25f, 0.000f,
         50.0f, -50.0f, -50.0f,   0.50f, 0.000f,
         50.0f, -50.0f,  50.0f,   0.50f, 0.333f,
         50.0f, -50.0f,  50.0f,   0.50f, 0.333f,
        -50.0f, -50.0f,  50.0f,   0.25f, 0.333f,
        -50.0f, -50.0f, -50.0f,   0.25f, 0.000f,
        
        // Top face (+Y) - column 1, row 0
        -50.0f,  50.0f, -50.0f,   0.25f, 1.000f,
        -50.0f,  50.0f,  50.0f,   0.25f, 0.666f,
         50.0f,  50.0f,  50.0f,   0.50f, 0.666f,
         50.0f,  50.0f,  50.0f,   0.50f, 0.666f,
         50.0f,  50.0f, -50.0f,   0.50f, 1.000f,
        -50.0f,  50.0f, -50.0f,   0.25f, 1.000f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // UV attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    std::cout << "[SkyboxAtlas] Geometry initialized" << std::endl;
}

unsigned int SkyboxAtlas::LoadAtlasTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false); // CRITICAL: No flip for skybox atlas
    
    // Get absolute path for detailed logging
    std::filesystem::path absPath = std::filesystem::absolute(path);
    
    std::cout << "[SkyboxAtlas]   Trying: " << path << std::endl;
    std::cout << "[SkyboxAtlas]   Absolute: " << absPath << std::endl;
    std::cout << "[SkyboxAtlas]   Exists: " << (std::filesystem::exists(absPath) ? "YES" : "NO") << std::endl;
    
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        
        // CRITICAL: GL_CLAMP_TO_EDGE prevents seam bleeding in atlas
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
        
        std::cout << "[SkyboxAtlas]   ? SUCCESS!" << std::endl;
        std::cout << "[SkyboxAtlas]   Resolution: " << width << "x" << height << std::endl;
        std::cout << "[SkyboxAtlas]   Channels: " << nrChannels << " (format: ";
        if (format == GL_RED) std::cout << "RED";
        else if (format == GL_RGB) std::cout << "RGB";
        else if (format == GL_RGBA) std::cout << "RGBA";
        std::cout << ")" << std::endl;
        std::cout << "[SkyboxAtlas]   Texture ID: " << textureID << std::endl;
        std::cout << "[SkyboxAtlas]   Wrap Mode: GL_CLAMP_TO_EDGE (prevents seams)" << std::endl;
        
        return textureID;
    }
    else
    {
        const char* reason = stbi_failure_reason();
        std::cout << "[SkyboxAtlas]   ? FAILED" << std::endl;
        std::cout << "[SkyboxAtlas]   STB Error: " << (reason ? reason : "Unknown") << std::endl;
        
        stbi_image_free(data);
        glDeleteTextures(1, &textureID);
        return 0; // Return 0 on failure
    }
}

bool SkyboxAtlas::LoadFromAtlas(const char* primaryPath)
{
    if (initialized)
    {
        std::cerr << "[SkyboxAtlas] Already initialized" << std::endl;
        return false;
    }
    
    SetupGeometry();
    
    // FIXED: Try proper atlas files first, then sky_debug as last resort
    // This ensures we get a clean atlas for submission, not debug text
    const char* atlasPaths[] = {
        "assets/skybox/skybox_atlas.jpg",      // Proper atlas (first priority)
        "assets/skybox/skybox_atlas.png",
        "assets/skybox/sky.jpg",               // Alternative clean atlas
        "assets/skybox/sky.png",
        "assets/skybox/sky_debug.jpg",         // Debug version (last resort)
        "assets/skybox/sky_debug.png",
        primaryPath  // User-specified fallback
    };
    
    std::cout << "[SkyboxAtlas] ========================================" << std::endl;
    std::cout << "[SkyboxAtlas] Searching for CLEAN atlas texture..." << std::endl;
    std::cout << "[SkyboxAtlas] ========================================" << std::endl;
    
    for (const char* path : atlasPaths)
    {
        if (path == nullptr || strlen(path) == 0) continue;
        
        std::cout << "[SkyboxAtlas] Attempting: " << path << std::endl;
        
        atlasTextureID = LoadAtlasTexture(path);
        
        if (atlasTextureID != 0)
        {
            std::cout << "[SkyboxAtlas] ========================================" << std::endl;
            std::cout << "[SkyboxAtlas] SUCCESS! Loaded: " << path << std::endl;
            std::cout << "[SkyboxAtlas] ========================================" << std::endl;
            initialized = true;
            return true;
        }
    }
    
    // All attempts failed - create fallback gradient (better than debug text)
    std::cout << "[SkyboxAtlas] ========================================" << std::endl;
    std::cout << "[SkyboxAtlas] WARNING: No atlas texture found!" << std::endl;
    std::cout << "[SkyboxAtlas] Creating CLEAN fallback gradient texture" << std::endl;
    std::cout << "[SkyboxAtlas] ========================================" << std::endl;
    
    // Create a clean sky-like gradient (blue sky)
    const int size = 256;
    unsigned char* fallback = new unsigned char[size * size * 3];
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            int i = (y * size + x) * 3;
            // Sky gradient: darker at top, lighter at bottom
            float t = (float)y / (float)size;
            fallback[i + 0] = (unsigned char)(135 * (1.0f - t * 0.3f)); // R
            fallback[i + 1] = (unsigned char)(206 * (1.0f - t * 0.2f)); // G
            fallback[i + 2] = (unsigned char)(235);                     // B
        }
    }
    
    glGenTextures(1, &atlasTextureID);
    glBindTexture(GL_TEXTURE_2D, atlasTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, fallback);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    delete[] fallback;
    
    initialized = true;
    return false; // Indicate fallback was used
}

void SkyboxAtlas::Draw(unsigned int shaderProgram) const
{
    if (!initialized) return;
    
    glDepthFunc(GL_LEQUAL);
    glUseProgram(shaderProgram);
    
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlasTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    glDepthFunc(GL_LESS);
}

void SkyboxAtlas::Cleanup()
{
    if (initialized)
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteTextures(1, &atlasTextureID);
        
        VAO = 0;
        VBO = 0;
        atlasTextureID = 0;
        initialized = false;
    }
}
