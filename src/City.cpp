#include "City.h"
#include "City.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

City::City()
    : shaderProgram(0), enabled(true), initialized(false)
{
}

City::~City()
{
    Cleanup();
}

void City::Initialize(unsigned int buildingShader)
{
    if (initialized) return;
    
    shaderProgram = buildingShader;
    
    // Initialize building geometry (shared VAO)
    Building::InitializeGeometry();
    
    // Load building textures
    LoadBuildingTextures();
    
    // Generate initial city
    Generate();
    
    initialized = true;
    std::cout << "[City] Initialized with " << buildings.size() << " buildings" << std::endl;
}

void City::LoadBuildingTextures()
{
    // Try to load facade textures (facade0-facade4 only, as specified)
    std::cout << "[City] Searching for facade textures (facade0-facade4)..." << std::endl;
    
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
            
            // Get absolute path for logging
            std::filesystem::path absPath = std::filesystem::absolute(relativePath);
            
            std::cout << "[City] Trying: " << relativePath << std::endl;
            std::cout << "[City]   Absolute: " << absPath << std::endl;
            
            unsigned int texID = Building::LoadBuildingTexture(relativePath.c_str());
            
            // Check if texture loaded successfully (non-zero and not fallback white)
            if (texID != 0)
            {
                buildingTextures.push_back(texID);
                std::cout << "[City]   SUCCESS: Loaded " << relativePath << std::endl;
                break; // Found this facade, move to next
            }
            else
            {
                std::cout << "[City]   FAILED: Could not load " << relativePath << std::endl;
            }
        }
    }
    
    // If no textures loaded, try default.png as final fallback
    if (buildingTextures.empty())
    {
        std::cout << "[City] WARNING: No facade textures found!" << std::endl;
        std::cout << "[City] Trying fallback: assets/textures/default.png" << std::endl;
        
        unsigned int fallbackTex = Building::LoadBuildingTexture("assets/textures/default.png");
        if (fallbackTex != 0)
        {
            buildingTextures.push_back(fallbackTex);
            std::cout << "[City] Using default.png as fallback" << std::endl;
        }
        else
        {
            // Create procedural fallback
            std::cout << "[City] Creating procedural fallback textures" << std::endl;
            
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
                buildingTextures.push_back(texID);
            }
        }
    }
    
    std::cout << "[City] Total facade textures loaded: " << buildingTextures.size() << std::endl;
}

void City::Generate(int seed)
{
    buildings.clear();
    
    std::cout << "[City] ========================================" << std::endl;
    std::cout << "[City] Generating city with seed: " << seed << std::endl;
    std::cout << "[City] ========================================" << std::endl;
    
    // Generate chunks around origin
    for (int cx = -CHUNK_RADIUS; cx <= CHUNK_RADIUS; ++cx)
    {
        for (int cz = -CHUNK_RADIUS; cz <= CHUNK_RADIUS; ++cz)
        {
            GenerateChunk(cx, cz, seed);
        }
    }
    
    std::cout << "[City] ========================================" << std::endl;
    std::cout << "[City] Total buildings generated: " << buildings.size() << std::endl;
    std::cout << "[City] City generation COMPLETE" << std::endl;
    std::cout << "[City] ========================================" << std::endl;
}

void City::GenerateChunk(int chunkX, int chunkZ, int baseSeed)
{
    float chunkOffsetX = chunkX * GRID_SIZE * (BLOCK_SIZE + ROAD_WIDTH);
    float chunkOffsetZ = chunkZ * GRID_SIZE * (BLOCK_SIZE + ROAD_WIDTH);
    
    // Track scale statistics for debugging
    float minHeight = 999999.0f, maxHeight = -999999.0f;
    float minWidth = 999999.0f, maxWidth = -999999.0f;
    int buildingCount = 0;
    
    for (int x = 0; x < GRID_SIZE; ++x)
    {
        for (int z = 0; z < GRID_SIZE; ++z)
        {
            // Skip roads (every 3rd or 4th block)
            if (IsRoad(x, z)) continue;
            
            // Create unique seed for this building
            int buildingSeed = baseSeed + (chunkX * 1000 + chunkZ) * 10000 + x * 100 + z;
            
            // Position with road gaps
            float posX = chunkOffsetX + x * (BLOCK_SIZE + ROAD_WIDTH);
            float posZ = chunkOffsetZ + z * (BLOCK_SIZE + ROAD_WIDTH);
            
            // Random height with STRICT CLAMPING
            float height = GetRandomHeight(buildingSeed);
            height = glm::clamp(height, 2.0f, 30.0f); // CLAMP: 2-30 units
            
            // Random width/depth variation with STRICT CLAMPING
            float widthVar = 0.8f + ((buildingSeed % 20) / 100.0f);
            float depthVar = 0.8f + (((buildingSeed / 7) % 20) / 100.0f);
            
            // CRITICAL: Clamp width/depth to prevent giant buildings
            widthVar = glm::clamp(widthVar, 0.8f, 1.0f);
            depthVar = glm::clamp(depthVar, 0.8f, 1.0f);
            
            float finalWidth = BLOCK_SIZE * widthVar;
            float finalDepth = BLOCK_SIZE * depthVar;
            
            // Additional safety clamp
            finalWidth = glm::clamp(finalWidth, 0.8f, 2.5f);
            finalDepth = glm::clamp(finalDepth, 0.8f, 2.5f);
            
            // Update statistics
            minHeight = glm::min(minHeight, height);
            maxHeight = glm::max(maxHeight, height);
            minWidth = glm::min(minWidth, glm::min(finalWidth, finalDepth));
            maxWidth = glm::max(maxWidth, glm::max(finalWidth, finalDepth));
            
            glm::vec3 position(posX, height / 2.0f, posZ);
            glm::vec3 scale(finalWidth, height, finalDepth);
            
            // Validate scale is not NaN or Inf
            if (std::isnan(scale.x) || std::isnan(scale.y) || std::isnan(scale.z) ||
                std::isinf(scale.x) || std::isinf(scale.y) || std::isinf(scale.z))
            {
                std::cerr << "[City] ERROR: Invalid scale detected! Skipping building." << std::endl;
                continue;
            }
            
            // Random texture
            unsigned int texID = GetRandomTexture(buildingSeed);
            
            buildings.emplace_back(position, scale, texID);
            buildingCount++;
        }
    }
    
    // Log chunk statistics
    if (buildingCount > 0)
    {
        std::cout << "[City] Chunk (" << chunkX << "," << chunkZ << ") generated: " 
                  << buildingCount << " buildings" << std::endl;
        std::cout << "[City]   Height range: " << minHeight << " - " << maxHeight << std::endl;
        std::cout << "[City]   Width range: " << minWidth << " - " << maxWidth << std::endl;
    }
}

bool City::IsRoad(int x, int z) const
{
    // Create roads every 3-4 blocks
    return (x % 4 == 0) || (z % 4 == 0);
}

float City::GetRandomHeight(int seed) const
{
    // Deterministic pseudo-random height
    float t = (seed % 1000) / 1000.0f;
    return MIN_HEIGHT + t * (MAX_HEIGHT - MIN_HEIGHT);
}

unsigned int City::GetRandomTexture(int seed) const
{
    if (buildingTextures.empty()) return 0;
    int index = seed % buildingTextures.size();
    return buildingTextures[index];
}

void City::UpdateChunks(const glm::vec3& cameraPos)
{
    // Calculate which chunk the camera is in
    float chunkSize = GRID_SIZE * (BLOCK_SIZE + ROAD_WIDTH);
    int camChunkX = static_cast<int>(std::floor(cameraPos.x / chunkSize));
    int camChunkZ = static_cast<int>(std::floor(cameraPos.z / chunkSize));
    
    // For now, keep all buildings (optimization can be added later)
    // In a full implementation, you'd dynamically load/unload chunks here
}

void City::Render(const glm::mat4& view, const glm::mat4& projection, 
                  const glm::mat4& lightSpaceMatrix, const glm::vec3& cameraPos)
{
    if (!enabled || !initialized || buildings.empty()) return;
    
    glUseProgram(shaderProgram);
    
    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    // Render each building
    for (const auto& building : buildings)
    {
        // Set model matrix
        glm::mat4 model = building.GetModelMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        // FIXED: Pass building scale for UV tiling (prevents stretching)
        glUniform3fv(glGetUniformLocation(shaderProgram, "buildingScale"), 1, glm::value_ptr(building.scale));
        
        // CRITICAL: Bind building texture to correct unit
        glActiveTexture(GL_TEXTURE0);
        
        // Use valid texture or fallback to white
        if (building.textureID != 0)
        {
            glBindTexture(GL_TEXTURE_2D, building.textureID);
        }
        else
        {
            // If texture is 0, create a simple white texture as fallback
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
        
        glUniform1i(glGetUniformLocation(shaderProgram, "buildingTexture"), 0);
        
        // Render cube geometry
        Building::RenderGeometry();
    }
}

void City::RenderShadow(const glm::mat4& lightSpaceMatrix, unsigned int shadowShader)
{
    if (!enabled || !initialized || buildings.empty()) return;
    
    glUseProgram(shadowShader);
    
    // Set light space matrix
    glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    // Render each building for shadow casting
    for (const auto& building : buildings)
    {
        // Set model matrix
        glm::mat4 model = building.GetModelMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        // Render geometry (no texture needed for shadow pass)
        Building::RenderGeometry();
    }
}

void City::Cleanup()
{
    if (initialized)
    {
        Building::CleanupGeometry();
        
        // Delete building textures
        for (unsigned int texID : buildingTextures)
        {
            glDeleteTextures(1, &texID);
        }
        buildingTextures.clear();
        buildings.clear();
        
        initialized = false;
    }
}
