#include "EndlessCityManager.h"
#include <iostream>
#include <cmath>
#include <random>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>  // Remove the #define, just include the header

// Enhanced constants for better visibility
const float MIN_BUILDING_WIDTH = 0.8f;         // Minimum building width multiplier
const float MAX_BUILDING_WIDTH = 1.0f;         // Maximum building width multiplier
const float MIN_BUILDING_DEPTH = 0.8f;         // Minimum building depth multiplier
const float MAX_BUILDING_DEPTH = 1.0f;         // Maximum building depth multiplier
const float ROAD_WIDTH_CITY = 2.0f;            // Width of roads (match City class)
const float BLOCK_SIZE_CITY = 4.0f;            // Size of each block (match City class)
const int GRID_SIZE_PER_CHUNK = 8;             // Buildings per chunk in grid pattern

EndlessCityManager::EndlessCityManager()
    : m_buildingShader(0)
    , m_groundShader(0)
    , m_currentChunkX(0)
    , m_currentChunkZ(0)
    , m_totalChunksGenerated(0)
    , m_totalBuildingsGenerated(0)
    , m_initialized(false)
    , m_buildingVAO(0)
    , m_buildingVBO(0)
    , m_groundVAO(0)
    , m_groundVBO(0)
    , m_groundTexture(0)  // NEW: Ground texture ID
{
}

EndlessCityManager::~EndlessCityManager()
{
    cleanup();
}

void EndlessCityManager::setupBuildingMesh()
{
    // Create a cube mesh with UVs (matching Building class)
    // Format: Position (3) + Normal (3) + UV (2) = 8 floats per vertex
    float vertices[] = {
        // Positions          // Normals           // UVs
        // Front face (+Z)
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        // Back face (-Z)
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        
        // Left face (-X)
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        
        // Right face (+X)
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        
        // Bottom face (-Y)
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        
        // Top face (+Y)
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f
    };
    
    glGenVertexArrays(1, &m_buildingVAO);
    glGenBuffers(1, &m_buildingVBO);
    
    glBindVertexArray(m_buildingVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_buildingVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // UV attribute (location = 2) - NOW INCLUDED
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    std::cout << "[EndlessCity] Building mesh initialized with UVs (matching Building class)" << std::endl;
}

void EndlessCityManager::setupGroundMesh()
{
    // Create a simple quad for ground tiles
    float vertices[] = {
        // Positions          // Normals         // UVs
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };
    
    glGenVertexArrays(1, &m_groundVAO);
    glGenBuffers(1, &m_groundVBO);
    
    glBindVertexArray(m_groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // UV attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void EndlessCityManager::initialize(unsigned int buildingShader, unsigned int groundShader)
{
    m_buildingShader = buildingShader;
    m_groundShader = groundShader;
    
    std::cout << "[EndlessCity] Initializing 2D GRID-BASED city..." << std::endl;
    
    // Setup meshes
    setupBuildingMesh();
    setupGroundMesh();
    
    // Load ground texture
    loadGroundTexture("assets/textures/ground.png");
    
    // Load building textures (NEW - same as City class)
    LoadBuildingTextures();
    
    // Initialize building geometry for Building class
    Building::InitializeGeometry();
    
    // Calculate static city size
    // City class: CHUNK_RADIUS=2, GRID_SIZE=10, spacing=6.0f
    // Static city spans: -2 to +2 chunks = 5 chunks
    // Each chunk: 10 * 6.0f = 60 units
    // Total static city: 5 * 60 = 300 units (-150 to +150)
    const float STATIC_CITY_SIZE = 300.0f; // Static city total size
    const float STATIC_CITY_HALF = STATIC_CITY_SIZE / 2.0f; // 150 units
    
    // Calculate which chunks overlap with static city
    const int STATIC_CITY_CHUNK_MIN = static_cast<int>(std::floor(-STATIC_CITY_HALF / CHUNK_SIZE));
    const int STATIC_CITY_CHUNK_MAX = static_cast<int>(std::ceil(STATIC_CITY_HALF / CHUNK_SIZE));
    
    std::cout << "[EndlessCity] Static city occupies chunks [" << STATIC_CITY_CHUNK_MIN 
              << " to " << STATIC_CITY_CHUNK_MAX << "]" << std::endl;
    std::cout << "[EndlessCity] Generating endless city chunks around static city..." << std::endl;
    
    // Generate initial chunks around origin (0, 0), but skip overlapping chunks
    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; ++x)
    {
        for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; ++z)
        {
            // Skip chunks that overlap with static city
            if (x >= STATIC_CITY_CHUNK_MIN && x <= STATIC_CITY_CHUNK_MAX &&
                z >= STATIC_CITY_CHUNK_MIN && z <= STATIC_CITY_CHUNK_MAX)
            {
                std::cout << "[EndlessCity] Skipping chunk (" << x << "," << z 
                          << ") - overlaps with static city" << std::endl;
                continue;
            }
            
            generateChunk(x, z);
        }
    }
    
    m_initialized = true;
    
    std::cout << "[EndlessCity] ? 2D Grid System initialized!" << std::endl;
    std::cout << "[EndlessCity]   Chunk size: " << CHUNK_SIZE << "x" << CHUNK_SIZE << " units" << std::endl;
    std::cout << "[EndlessCity]   Initial grid: " << (RENDER_DISTANCE*2+1) << "x" << (RENDER_DISTANCE*2+1) << " chunks" << std::endl;
    std::cout << "[EndlessCity]   Buildings: " << m_totalBuildingsGenerated << std::endl;
    std::cout << "[EndlessCity]   Building Textures: " << m_buildingTextures.size() << (m_buildingTextures.empty() ? " ? NONE" : " ? LOADED") << std::endl;
    std::cout << "[EndlessCity]   Ground Texture: " << (m_groundTexture != 0 ? "? LOADED" : "? Using solid colors") << std::endl;
    std::cout << "[EndlessCity]   ?? GENERATES AROUND STATIC CITY!" << std::endl;
}

void EndlessCityManager::loadGroundTexture(const char* path)
{
    // Try to load using STB Image
    int width, height, nrChannels;
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

        glGenTextures(1, &m_groundTexture);
        glBindTexture(GL_TEXTURE_2D, m_groundTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "[EndlessCity] ? Ground texture loaded: " << path << " (" << width << "x" << height << ")" << std::endl;
    }
    else
    {
        std::cout << "[EndlessCity] ?? Failed to load ground texture: " << path << std::endl;
        std::cout << "[EndlessCity]    Using procedural colors instead" << std::endl;
        m_groundTexture = 0;
    }
}

void EndlessCityManager::LoadBuildingTextures()
{
    // Try to load facade textures (facade0-facade4 only, as specified)
    std::cout << "[EndlessCity] Searching for facade textures (facade0-facade4)..." << std::endl;
    
    // List of facade filenames to try (facade0-facade4 only)
    const char* facadeNames[] = {
        "facade0", "facade1", "facade2", "facade3", "facade4"
    };
    
    // Extensions to try
    const char* extensions[] = { ".jpg", ".png", ".jpeg" };
    
    for (const char* facadeName : facadeNames)
    {
        for (const char* ext : extensions)
        {
            std::string relativePath = std::string("assets/textures/") + facadeName + ext;
            
            std::cout << "[EndlessCity] Trying: " << relativePath << std::endl;
            
            unsigned int texID = Building::LoadBuildingTexture(relativePath.c_str());
            
            // Check if texture loaded successfully (non-zero)
            if (texID != 0)
            {
                m_buildingTextures.push_back(texID);
                std::cout << "[EndlessCity]   SUCCESS: Loaded " << relativePath << std::endl;
                break; // Found this facade, move to next
            }
            else
            {
                std::cout << "[EndlessCity]   FAILED: Could not load " << relativePath << std::endl;
            }
        }
    }
    
    // If no textures loaded, try default.png as final fallback
    if (m_buildingTextures.empty())
    {
        std::cout << "[EndlessCity] WARNING: No facade textures found!" << std::endl;
        std::cout << "[EndlessCity] Trying fallback: assets/textures/default.png" << std::endl;
        
        unsigned int fallbackTex = Building::LoadBuildingTexture("assets/textures/default.png");
        if (fallbackTex != 0)
        {
            m_buildingTextures.push_back(fallbackTex);
            std::cout << "[EndlessCity] Using default.png as fallback" << std::endl;
        }
        else
        {
            // Create procedural fallback
            std::cout << "[EndlessCity] Creating procedural fallback textures" << std::endl;
            
            unsigned char colors[][4] = {
                {200, 200, 200, 255}, // Light gray
                {150, 150, 150, 255}, // Medium gray
                {180, 160, 140, 255}  // Brownish
            };
            
            for (auto& color : colors)
            {
                unsigned int texID;
                glGenTextures(1, &texID);
                glBindTexture(GL_TEXTURE_2D, texID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_buildingTextures.push_back(texID);
            }
        }
    }
    
    std::cout << "[EndlessCity] Total facade textures loaded: " << m_buildingTextures.size() << std::endl;
}

unsigned int EndlessCityManager::GetRandomTexture(int seed) const
{
    if (m_buildingTextures.empty()) return 0;
    int index = seed % m_buildingTextures.size();
    return m_buildingTextures[index];
}

std::pair<int, int> EndlessCityManager::worldToChunk(const glm::vec3& worldPos) const
{
    int chunkX = static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE));
    int chunkZ = static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE));
    return std::make_pair(chunkX, chunkZ);
}

CityChunk* EndlessCityManager::getOrCreateChunk(int chunkX, int chunkZ)
{
    // Skip chunks that overlap with static city
    const float STATIC_CITY_SIZE = 300.0f;
    const float STATIC_CITY_HALF = STATIC_CITY_SIZE / 2.0f;
    const int STATIC_CITY_CHUNK_MIN = static_cast<int>(std::floor(-STATIC_CITY_HALF / CHUNK_SIZE));
    const int STATIC_CITY_CHUNK_MAX = static_cast<int>(std::ceil(STATIC_CITY_HALF / CHUNK_SIZE));
    
    if (chunkX >= STATIC_CITY_CHUNK_MIN && chunkX <= STATIC_CITY_CHUNK_MAX &&
        chunkZ >= STATIC_CITY_CHUNK_MIN && chunkZ <= STATIC_CITY_CHUNK_MAX)
    {
        return nullptr; // Don't generate chunks in static city area
    }
    
    auto key = std::make_pair(chunkX, chunkZ);
    auto it = m_activeChunks.find(key);
    
    if (it != m_activeChunks.end())
    {
        return &it->second;
    }
    
    // Generate new chunk
    generateChunk(chunkX, chunkZ);
    return &m_activeChunks[key];
}

void EndlessCityManager::generateChunk(int chunkX, int chunkZ)
{
    auto key = std::make_pair(chunkX, chunkZ);
    
    // Skip if already generated
    if (m_activeChunks.find(key) != m_activeChunks.end())
        return;
    
    CityChunk& chunk = m_activeChunks[key];
    chunk.chunkX = chunkX;
    chunk.chunkZ = chunkZ;
    chunk.buildings.clear();
    chunk.groundTiles.clear();
    
    // Deterministic seed based on chunk coordinates
    uint32_t seed = CITY_GLOBAL_SEED ^ 
                   (static_cast<uint32_t>(std::abs(chunkX)) * 73856093) ^ 
                   (static_cast<uint32_t>(std::abs(chunkZ)) * 19349663);
    std::mt19937 rng(seed);
    
    // Generate buildings
    generateBuildings(chunk, rng);
    
    // Generate ground tiles
    generateGroundTiles(chunk, rng);
    
    // Compute AABB for frustum culling
    float worldX = chunkX * CHUNK_SIZE;
    float worldZ = chunkZ * CHUNK_SIZE;
    chunk.aabbMin = glm::vec3(worldX, 0.0f, worldZ);
    chunk.aabbMax = glm::vec3(worldX + CHUNK_SIZE, ENDLESS_MAX_HEIGHT, worldZ + CHUNK_SIZE);
    chunk.isGenerated = true;
    chunk.isVisible = true;
    
    m_totalChunksGenerated++;
    
    // Debug output (reduced frequency)
    if (m_totalChunksGenerated % 10 == 0)
    {
        std::cout << "[EndlessCity] Generated " << m_totalChunksGenerated 
                  << " chunks | Active: " << m_activeChunks.size() 
                  << " | Buildings: " << m_totalBuildingsGenerated << std::endl;
    }
}

void EndlessCityManager::generateBuildings(CityChunk& chunk, std::mt19937& rng)
{
    // Use grid-based generation like City class to prevent overlaps
    const int gridSize = GRID_SIZE_PER_CHUNK;
    const float blockSize = BLOCK_SIZE_CITY;
    const float roadWidth = ROAD_WIDTH_CITY;
    
    std::uniform_real_distribution<float> widthVar(0.8f, 1.0f);
    std::uniform_real_distribution<float> depthVar(0.8f, 1.0f);
    std::uniform_real_distribution<float> heightDist(3.0f, 15.0f); // Match City class
    
    // Calculate chunk world position
    float chunkWorldX = chunk.chunkX * CHUNK_SIZE;
    float chunkWorldZ = chunk.chunkZ * CHUNK_SIZE;
    
    // Generate buildings in a grid pattern
    for (int x = 0; x < gridSize; ++x)
    {
        for (int z = 0; z < gridSize; ++z)
        {
            // Skip roads (every 4th position)
            if (x % 4 == 0 || z % 4 == 0) continue;
            
            CityBuilding building;
            
            // Position on grid (same logic as City class)
            float localX = x * (blockSize + roadWidth);
            float localZ = z * (blockSize + roadWidth);
            
            building.position.x = chunkWorldX + localX;
            building.position.z = chunkWorldZ + localZ;
            
            // Building dimensions with slight variation
            float width = blockSize * widthVar(rng);
            float depth = blockSize * depthVar(rng);
            float height = heightDist(rng);
            
            // Clamp dimensions to prevent overlap
            width = glm::clamp(width, 0.8f, 2.5f);
            depth = glm::clamp(depth, 0.8f, 2.5f);
            height = glm::clamp(height, 3.0f, 15.0f);
            
            building.position.y = height * 0.5f; // Center at half height
            building.scale = glm::vec3(width, height, depth);
            
            // Minimal rotation for grid alignment
            building.rotation = 0.0f;
            
            // Building type based on grid position
            building.buildingType = (x + z) % 3; // 0=Skyscraper, 1=Office, 2=Apartment (skip type 3)
            
            // Assign texture using deterministic seed (same as City class)
            int buildingSeed = chunk.chunkX * 10000 + chunk.chunkZ * 100 + x * 10 + z;
            building.textureID = GetRandomTexture(buildingSeed);
            
            chunk.buildings.push_back(building);
            m_totalBuildingsGenerated++;
        }
    }
}

void EndlessCityManager::generateGroundTiles(CityChunk& chunk, std::mt19937& rng)
{
    std::uniform_real_distribution<float> colorVar(0.0f, 0.1f);
    
    float worldX = chunk.chunkX * CHUNK_SIZE + CHUNK_SIZE * 0.5f;
    float worldZ = chunk.chunkZ * CHUNK_SIZE + CHUNK_SIZE * 0.5f;
    
    // Create main ground tile for entire chunk
    {
        GroundTile ground;
        ground.position = glm::vec3(worldX, 0.01f, worldZ);
        ground.scale = glm::vec3(CHUNK_SIZE, 1.0f, CHUNK_SIZE);
        
        // Alternate ground color based on chunk coordinates (checkerboard pattern)
        bool isEvenChunk = ((chunk.chunkX + chunk.chunkZ) % 2 == 0);
        if (isEvenChunk)
            ground.color = glm::vec3(0.25f, 0.45f, 0.25f) + glm::vec3(colorVar(rng)); // Grass
        else
            ground.color = glm::vec3(0.35f, 0.35f, 0.35f) + glm::vec3(colorVar(rng)); // Concrete
            
        ground.tileType = 2; // Park/Plaza
        chunk.groundTiles.push_back(ground);
    }
    
    // Add some road tiles crossing through chunk
    if (chunk.chunkX % 3 == 0 || chunk.chunkZ % 3 == 0)
    {
        GroundTile road;
        road.position = glm::vec3(worldX, 0.02f, worldZ);
        road.scale = glm::vec3(CHUNK_SIZE * 0.2f, 1.0f, CHUNK_SIZE);
        road.color = glm::vec3(0.2f, 0.2f, 0.25f) + glm::vec3(colorVar(rng));
        road.tileType = 0; // Road
        chunk.groundTiles.push_back(road);
    }
}

void EndlessCityManager::update(float deltaTime, const glm::vec3& cameraPos)
{
    // Calculate which chunk the camera is in
    auto newChunk = worldToChunk(cameraPos);
    int newChunkX = newChunk.first;
    int newChunkZ = newChunk.second;
    
    // Only update if camera moved to a new chunk
    if (newChunkX != m_currentChunkX || newChunkZ != m_currentChunkZ)
    {
        std::cout << "[EndlessCity] ?? Moved to chunk (" << newChunkX << ", " << newChunkZ << ")" << std::endl;
        
        m_currentChunkX = newChunkX;
        m_currentChunkZ = newChunkZ;
        
        // Generate chunks in render distance around camera
        for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; ++x)
        {
            for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; ++z)
            {
                int chunkX = m_currentChunkX + x;
                int chunkZ = m_currentChunkZ + z;
                getOrCreateChunk(chunkX, chunkZ);
            }
        }
        
        // Remove distant chunks
        cullDistantChunks(m_currentChunkX, m_currentChunkZ);
    }
}

void EndlessCityManager::cullDistantChunks(int centerChunkX, int centerChunkZ)
{
    std::vector<std::pair<int, int>> chunksToRemove;
    
    for (auto& pair : m_activeChunks)
    {
        int chunkX = pair.second.chunkX;
        int chunkZ = pair.second.chunkZ;
        
        int distX = std::abs(chunkX - centerChunkX);
        int distZ = std::abs(chunkZ - centerChunkZ);
        
        // Remove if outside render distance (with buffer)
        if (distX > RENDER_DISTANCE + 1 || distZ > RENDER_DISTANCE + 1)
        {
            chunksToRemove.push_back(pair.first);
        }
    }
    
    // Remove distant chunks
    for (auto& key : chunksToRemove)
    {
        m_activeChunks.erase(key);
    }
    
    if (!chunksToRemove.empty())
    {
        std::cout << "[EndlessCity] ??  Culled " << chunksToRemove.size() 
                  << " distant chunks | Active: " << m_activeChunks.size() << std::endl;
    }
}

void EndlessCityManager::renderBuilding(const CityBuilding& building, const glm::mat4& view, 
                                       const glm::mat4& projection, const glm::mat4& lightSpaceMatrix)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, building.position);
    model = glm::rotate(model, building.rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, building.scale);
    
    glUniformMatrix4fv(glGetUniformLocation(m_buildingShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(m_buildingShader, "buildingScale"), 1, glm::value_ptr(building.scale));
    glUniform1i(glGetUniformLocation(m_buildingShader, "buildingType"), building.buildingType);
    
    // Bind building texture (CHANGED from color)
    glActiveTexture(GL_TEXTURE0);
    if (building.textureID != 0)
    {
        glBindTexture(GL_TEXTURE_2D, building.textureID);
    }
    else
    {
        // Fallback to white texture if none available
        static unsigned int whiteTex = 0;
        if (whiteTex == 0)
        {
            unsigned char white[4] = {255, 255, 255, 255};
            glGenTextures(1, &whiteTex);
            glBindTexture(GL_TEXTURE_2D, whiteTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, whiteTex);
        }
    }
    glUniform1i(glGetUniformLocation(m_buildingShader, "buildingTexture"), 0);
    
    glBindVertexArray(m_buildingVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void EndlessCityManager::renderGroundTile(const GroundTile& tile, const glm::mat4& view, 
                                         const glm::mat4& projection)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, tile.position);
    model = glm::scale(model, tile.scale);
    
    glUniformMatrix4fv(glGetUniformLocation(m_groundShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(m_groundShader, "tileColor"), 1, glm::value_ptr(tile.color));
    glUniform1i(glGetUniformLocation(m_groundShader, "tileType"), tile.tileType);
    
    // Bind ground texture if available
    if (m_groundTexture != 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_groundTexture);
        glUniform1i(glGetUniformLocation(m_groundShader, "texture_diffuse1"), 0);
        glUniform1i(glGetUniformLocation(m_groundShader, "useTexture"), 1);
    }
    else
    {
        glUniform1i(glGetUniformLocation(m_groundShader, "useTexture"), 0);
    }
    
    glBindVertexArray(m_groundVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void EndlessCityManager::render(const glm::mat4& view, const glm::mat4& projection, 
                                const glm::mat4& lightSpaceMatrix, const glm::vec3& viewPos)
{
    if (!m_initialized) return;
    
    int visibleChunks = 0;
    int totalBuildings = 0;
    int totalGroundTiles = 0;
    
    // Render ground tiles first
    glUseProgram(m_groundShader);
    glUniformMatrix4fv(glGetUniformLocation(m_groundShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_groundShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    for (const auto& pair : m_activeChunks)
    {
        const CityChunk& chunk = pair.second;
        if (!chunk.isVisible) continue;
        
        for (const auto& tile : chunk.groundTiles)
        {
            renderGroundTile(tile, view, projection);
            totalGroundTiles++;
        }
    }
    
    // Render buildings (SKIP TYPE 3 - light brown commercial buildings)
    glUseProgram(m_buildingShader);
    glUniformMatrix4fv(glGetUniformLocation(m_buildingShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_buildingShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_buildingShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glUniform3fv(glGetUniformLocation(m_buildingShader, "viewPos"), 1, glm::value_ptr(viewPos));
    
    for (const auto& pair : m_activeChunks)
    {
        const CityChunk& chunk = pair.second;
        if (!chunk.isVisible) continue;
        
        visibleChunks++;
        
        for (const auto& building : chunk.buildings)
        {
            // SKIP building type 3 (light brown commercial buildings)
            if (building.buildingType == 3) {
                continue;  // Don't render type 3 buildings
            }
            
            renderBuilding(building, view, projection, lightSpaceMatrix);
            totalBuildings++;
        }
    }
    
    // Debug output (reduced frequency)
    static int frameCounter = 0;
    if (frameCounter++ % 120 == 0)
    {
        std::cout << "[EndlessCity] ???  Chunk (" << m_currentChunkX << "," << m_currentChunkZ << ") " 
                  << "| Visible: " << visibleChunks << "/" << m_activeChunks.size()
                  << " | Buildings: " << totalBuildings << " (Type 3 skipped)"
                  << " | Ground: " << totalGroundTiles << std::endl;
    }
}

void EndlessCityManager::cleanup()
{
    if (m_buildingVAO != 0)
    {
        glDeleteVertexArrays(1, &m_buildingVAO);
        glDeleteBuffers(1, &m_buildingVBO);
        m_buildingVAO = 0;
        m_buildingVBO = 0;
    }
    
    if (m_groundVAO != 0)
    {
        glDeleteVertexArrays(1, &m_groundVAO);
        glDeleteBuffers(1, &m_groundVBO);
        m_groundVAO = 0;
        m_groundVBO = 0;
    }
    
    if (m_groundTexture != 0)
    {
        glDeleteTextures(1, &m_groundTexture);
        m_groundTexture = 0;
    }
    
    // Delete building textures (NEW)
    for (unsigned int texID : m_buildingTextures)
    {
        glDeleteTextures(1, &texID);
    }
    m_buildingTextures.clear();
    
    Building::CleanupGeometry();
    
    m_activeChunks.clear();
    m_buildingShader = 0;
    m_groundShader = 0;
    m_initialized = false;
    
    std::cout << "[EndlessCity] Cleaned up" << std::endl;
}
