#pragma once

#include "Building.h"
#include <vector>
#include <glm/glm.hpp>

class City
{
public:
    City();
    ~City();
    
    void Initialize(unsigned int buildingShader);
    void Generate(int seed = 42);
    void Render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::mat4& lightSpaceMatrix, const glm::vec3& cameraPos);
    void RenderShadow(const glm::mat4& lightSpaceMatrix, unsigned int shadowShader);
    void UpdateChunks(const glm::vec3& cameraPos);
    void Cleanup();
    
    bool IsEnabled() const { return enabled; }
    void SetEnabled(bool enable) { enabled = enable; }
    void ToggleEnabled() { enabled = !enabled; }
    
private:
    std::vector<Building> buildings;
    std::vector<unsigned int> buildingTextures;
    unsigned int shaderProgram;
    bool enabled;
    bool initialized;
    
    // City generation parameters
    static constexpr int GRID_SIZE = 10;      // Buildings per chunk side
    static constexpr float BLOCK_SIZE = 4.0f;  // Size of each block
    static constexpr float ROAD_WIDTH = 2.0f;  // Width of roads
    static constexpr float MIN_HEIGHT = 3.0f;
    static constexpr float MAX_HEIGHT = 15.0f;
    static constexpr int CHUNK_RADIUS = 2;     // Chunks to render around camera
    
    void GenerateChunk(int chunkX, int chunkZ, int seed);
    void LoadBuildingTextures();
    unsigned int GetRandomTexture(int seed) const;
    float GetRandomHeight(int seed) const;
    bool IsRoad(int x, int z) const;
};
