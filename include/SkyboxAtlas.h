#pragma once

#include <glad/glad.h>
#include <string>

// SkyboxAtlas implements Lab2-style skybox using a 2D texture atlas
// instead of a cubemap, with inward-facing cube geometry
class SkyboxAtlas
{
public:
    SkyboxAtlas();
    ~SkyboxAtlas();
    
    bool LoadFromAtlas(const char* atlasPath);
    void Draw(unsigned int shaderProgram) const;
    void Cleanup();
    
    bool IsInitialized() const { return initialized; }
    
private:
    unsigned int VAO, VBO;
    unsigned int atlasTextureID;
    bool initialized;
    
    void SetupGeometry();
    unsigned int LoadAtlasTexture(const char* path);
};
