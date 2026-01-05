#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>
#include <random>
#include "Building.h"

// Configuration for endless city generation (2D GRID-BASED)
const float CHUNK_SIZE = 50.0f;              // Size of each chunk (50x50 units)
const int BUILDINGS_PER_CHUNK = 15;          // DEPRECATED - now uses grid system
const float ENDLESS_BLOCK_SIZE = 4.0f;       // MATCH City class: Size of each building block
const float ENDLESS_ROAD_WIDTH = 2.0f;       // MATCH City class: Width of roads between buildings
const float ENDLESS_MIN_HEIGHT = 3.0f;       // MATCH City class: Minimum building height
const float ENDLESS_MAX_HEIGHT = 15.0f;      // MATCH City class: Maximum building height
const int RENDER_DISTANCE = 3;               // Chunks to render around camera (3 = 7x7 grid)
const unsigned int CITY_GLOBAL_SEED = 12345; // Fixed seed for determinism

// Building structure for endless generation
struct CityBuilding {
    glm::vec3 position;
    glm::vec3 scale;           // Width, Height, Depth
    unsigned int textureID;    // Building texture (CHANGED from color)
    int buildingType;          // 0=Skyscraper, 1=Office, 2=Apartment, 3=Commercial
    float rotation;            // Y-axis rotation
};

// Ground tile structure
struct GroundTile {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;           // Tile color (road vs sidewalk)
    int tileType;              // 0=Road, 1=Sidewalk, 2=Park
};

// Render parameters for endless city (lighting, shadows, bloom)
struct EndlessCityRenderParams {
    // Directional light
    glm::vec3 dirLightDir;
    glm::vec3 dirLightColor;
    
    // Point light
    glm::vec3 pointLightPos;
    glm::vec3 pointLightColor;
    float pointLightConstant;
    float pointLightLinear;
    float pointLightQuadratic;
    
    // Shadow
    unsigned int shadowMapTexture;
    bool enableShadows;
    bool enablePCF;
    
    // Bloom
    float bloomThreshold;
    
    // CSM support
    bool enableCSM;
    bool visualizeCascades;
    int numCascades;
    unsigned int csmShadowMapArray;
    glm::mat4 lightSpaceMatrices[4];  // Up to 4 cascades
    float cascadeSplits[4];
    
    // Default constructor with sensible defaults
    EndlessCityRenderParams() 
        : dirLightDir(glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f)))
        , dirLightColor(1.0f, 1.0f, 0.95f)
        , pointLightPos(2.0f, 3.0f, 2.0f)
        , pointLightColor(1.0f, 0.9f, 0.7f)
        , pointLightConstant(1.0f)
        , pointLightLinear(0.09f)
        , pointLightQuadratic(0.032f)
        , shadowMapTexture(0)
        , enableShadows(true)
        , enablePCF(true)
        , bloomThreshold(1.0f)
        , enableCSM(false)
        , visualizeCascades(false)
        , numCascades(0)
        , csmShadowMapArray(0)
    {
        for (int i = 0; i < 4; i++) {
            lightSpaceMatrices[i] = glm::mat4(1.0f);
            cascadeSplits[i] = 0.0f;
        }
    }
};

// Chunk structure (2D grid cell)
struct CityChunk {
    int chunkX;                              // Chunk X coordinate
    int chunkZ;                              // Chunk Z coordinate
    std::vector<CityBuilding> buildings;     // Buildings in this chunk
    std::vector<GroundTile> groundTiles;     // Ground tiles in this chunk
    bool isGenerated;                        // Has this chunk been generated?
    bool isVisible;                          // Frustum culling flag
    glm::vec3 aabbMin;                       // AABB for frustum culling
    glm::vec3 aabbMax;
    
    CityChunk() : chunkX(0), chunkZ(0), isGenerated(false), isVisible(true) {}
    
    // Hash function for chunk coordinates (for unordered_map)
    struct Hash {
        size_t operator()(const std::pair<int, int>& p) const {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };
};

class EndlessCityManager {
public:
    EndlessCityManager();
    ~EndlessCityManager();
    
    // Initialize the endless city system
    void initialize(unsigned int buildingShader, unsigned int groundShader);
    
    // Set CSM shader for use when CSM is enabled
    void setCSMShader(unsigned int csmShader) { m_buildingCSMShader = csmShader; }
    
    // Update based on camera position (generates/removes chunks in all directions)
    void update(float deltaTime, const glm::vec3& cameraPosition);
    
    // Render visible chunks (NEW: with full lighting/shadow parameters)
    void render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::mat4& lightSpaceMatrix, const glm::vec3& viewPos,
                const EndlessCityRenderParams& renderParams);
    
    // Legacy render function (calls new one with default params) - for backward compatibility
    void render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::mat4& lightSpaceMatrix, const glm::vec3& viewPos);
    
    // NEW: Render for shadow pass (depth only)
    void renderShadow(const glm::mat4& lightSpaceMatrix, unsigned int shadowShader);
    
    // Cleanup
    void cleanup();
    
    // Query methods
    std::pair<int, int> getCurrentChunk() const { 
        return std::make_pair(m_currentChunkX, m_currentChunkZ); 
    }
    int getTotalChunksGenerated() const { return m_totalChunksGenerated; }
    int getTotalBuildingsGenerated() const { return m_totalBuildingsGenerated; }
    int getActiveChunkCount() const { return static_cast<int>(m_activeChunks.size()); }
    
private:
    // Generate buildings and ground for a chunk deterministically
    void generateChunk(int chunkX, int chunkZ);
    
    // Generate buildings for a chunk
    void generateBuildings(CityChunk& chunk, std::mt19937& rng);
    
    // Generate ground tiles for a chunk
    void generateGroundTiles(CityChunk& chunk, std::mt19937& rng);
    
    // Get chunk at specific coordinates (creates if doesn't exist)
    CityChunk* getOrCreateChunk(int chunkX, int chunkZ);
    
    // Remove chunks outside render distance
    void cullDistantChunks(int centerChunkX, int centerChunkZ);
    
    // Convert world position to chunk coordinates
    std::pair<int, int> worldToChunk(const glm::vec3& worldPos) const;
    
    // Render a single building with active shader
    void renderBuilding(const CityBuilding& building, const glm::mat4& view, 
                       const glm::mat4& projection, const glm::mat4& lightSpaceMatrix,
                       unsigned int activeShader);
    
    // Render a single ground tile
    void renderGroundTile(const GroundTile& tile, const glm::mat4& view, 
                         const glm::mat4& projection);
    
    // Setup building mesh (once)
    void setupBuildingMesh();
    
    // Setup ground tile mesh (once)
    void setupGroundMesh();
    
    // Load ground texture
    void loadGroundTexture(const char* path);
    
    // Load building textures (same as City class)
    void LoadBuildingTextures();
    
    // Get random texture based on seed (same as City class)
    unsigned int GetRandomTexture(int seed) const;
    
    // Data members
    std::unordered_map<std::pair<int, int>, CityChunk, CityChunk::Hash> m_activeChunks;
    
    std::vector<unsigned int> m_buildingTextures;  // NEW: Building facade textures
    unsigned int m_buildingShader;            // Building shader program
    unsigned int m_buildingCSMShader;         // CSM building shader program
    unsigned int m_groundShader;              // Ground shader program
    
    int m_currentChunkX;                      // Current chunk X player is in
    int m_currentChunkZ;                      // Current chunk Z player is in
    int m_totalChunksGenerated;               // Total chunks created (for stats)
    int m_totalBuildingsGenerated;            // Total buildings created
    bool m_initialized;
    
    // Mesh data
    unsigned int m_buildingVAO;
    unsigned int m_buildingVBO;
    unsigned int m_groundVAO;
    unsigned int m_groundVBO;
    unsigned int m_groundTexture;              // NEW: Ground texture ID
};
