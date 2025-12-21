#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

// Building represents a textured cube with UV mapping for city generation
class Building
{
public:
    glm::vec3 position;
    glm::vec3 scale;
    unsigned int textureID;
    
    Building(const glm::vec3& pos, const glm::vec3& scl, unsigned int texID);
    
    glm::mat4 GetModelMatrix() const;
    
    // Static methods for shared geometry
    static void InitializeGeometry();
    static void CleanupGeometry();
    static void RenderGeometry();
    static unsigned int LoadBuildingTexture(const char* path);
    
private:
    static unsigned int VAO, VBO;
    static bool geometryInitialized;
    
    static void SetupCubeWithUVs();
};
