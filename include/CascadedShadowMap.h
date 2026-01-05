#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <array>

// Number of shadow cascades (typically 3-4)
const int NUM_CASCADES = 4;

// Cascade configuration
struct CascadeConfig {
    float splitDepths[NUM_CASCADES + 1];  // Near, split1, split2, split3, far
    float cascadeBlendWidth = 0.1f;        // Blend region between cascades
    float shadowBias = 0.005f;
    bool usePCF = true;
    bool visualizeCascades = false;        // Debug: show cascade colors
};

// Per-cascade data
struct CascadeData {
    glm::mat4 lightSpaceMatrix;
    float splitDepth;
    unsigned int depthTexture;
};

class CascadedShadowMap {
public:
    CascadedShadowMap(unsigned int resolution = 2048);
    ~CascadedShadowMap();
    
    // Initialize framebuffers and textures
    bool Initialize();
    
    // Calculate cascade splits based on camera frustum
    void UpdateCascades(const glm::mat4& viewMatrix, 
                        const glm::mat4& projectionMatrix,
                        const glm::vec3& lightDirection,
                        float nearPlane, float farPlane);
    
    // Bind specific cascade for shadow rendering
    void BindForWriting(int cascadeIndex);
    
    // Unbind after rendering
    void Unbind();
    
    // Bind all cascades for reading in main pass
    void BindForReading(GLenum textureUnit);
    
    // Get cascade data
    const CascadeData& GetCascade(int index) const { return cascades[index]; }
    const std::array<CascadeData, NUM_CASCADES>& GetCascades() const { return cascades; }
    
    // Get all light space matrices as array (for shader)
    void GetLightSpaceMatrices(glm::mat4* matrices) const;
    
    // Get cascade split depths (for shader)
    void GetCascadeSplits(float* splits) const;
    
    // Get texture array ID
    unsigned int GetDepthTextureArray() const { return depthTextureArray; }
    
    // Configuration
    CascadeConfig& GetConfig() { return config; }
    void SetConfig(const CascadeConfig& cfg) { config = cfg; }
    
    // Set split distribution (0 = linear, 1 = logarithmic)
    void SetSplitLambda(float lambda) { splitLambda = lambda; }
    float GetSplitLambda() const { return splitLambda; }
    
    // Debug visualization
    void SetVisualizeCascades(bool vis) { config.visualizeCascades = vis; }
    bool IsVisualizingCascades() const { return config.visualizeCascades; }
    
    // Get resolution
    unsigned int GetResolution() const { return resolution; }
    
    // Debug info
    void PrintDebugInfo() const;
    
    // Cleanup
    void Cleanup();
    
private:
    unsigned int resolution;
    unsigned int FBO;
    unsigned int depthTextureArray;  // 2D texture array for all cascades
    
    std::array<CascadeData, NUM_CASCADES> cascades;
    CascadeConfig config;
    
    float splitLambda;  // 0 = linear, 1 = logarithmic split
    
    // Internal helpers
    void CalculateSplitDepths(float nearPlane, float farPlane);
    glm::mat4 CalculateLightSpaceMatrix(const std::vector<glm::vec4>& frustumCorners,
                                         const glm::vec3& lightDirection,
                                         int cascadeIndex);
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projection,
                                                        const glm::mat4& view);
};
