#include "CascadedShadowMap.h"
#include <iostream>
#include <algorithm>
#include <cmath>

CascadedShadowMap::CascadedShadowMap(unsigned int resolution)
    : resolution(resolution), FBO(0), depthTextureArray(0), splitLambda(0.5f)
{
    // Initialize cascade data
    for (int i = 0; i < NUM_CASCADES; i++) {
        cascades[i].lightSpaceMatrix = glm::mat4(1.0f);
        cascades[i].splitDepth = 0.0f;
        cascades[i].depthTexture = 0;
    }
}

CascadedShadowMap::~CascadedShadowMap() {
    Cleanup();
}

bool CascadedShadowMap::Initialize() {
    std::cout << "[CSM] Initializing Cascaded Shadow Maps..." << std::endl;
    std::cout << "[CSM] Resolution: " << resolution << "x" << resolution << std::endl;
    std::cout << "[CSM] Cascades: " << NUM_CASCADES << std::endl;
    
    // Create framebuffer
    glGenFramebuffers(1, &FBO);
    
    // Create 2D texture array for all cascades
    glGenTextures(1, &depthTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthTextureArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                 resolution, resolution, NUM_CASCADES,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    // Use standard filtering for manual depth comparison
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // Border color = 1.0 means "not in shadow" for areas outside the shadow map
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach texture array to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTextureArray, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[CSM] ERROR: Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    std::cout << "[CSM] Initialized successfully" << std::endl;
    return true;
}

void CascadedShadowMap::CalculateSplitDepths(float nearPlane, float farPlane) {
    // Practical split scheme: blend between logarithmic and linear
    // Lambda = 0: linear, Lambda = 1: logarithmic
    
    config.splitDepths[0] = nearPlane;
    
    for (int i = 1; i < NUM_CASCADES; i++) {
        float p = static_cast<float>(i) / static_cast<float>(NUM_CASCADES);
        
        // Logarithmic distribution
        float log_split = nearPlane * std::pow(farPlane / nearPlane, p);
        
        // Linear distribution
        float linear_split = nearPlane + (farPlane - nearPlane) * p;
        
        // Blend between them
        config.splitDepths[i] = splitLambda * log_split + (1.0f - splitLambda) * linear_split;
    }
    
    config.splitDepths[NUM_CASCADES] = farPlane;
    
    // Store split depths in cascade data
    for (int i = 0; i < NUM_CASCADES; i++) {
        cascades[i].splitDepth = config.splitDepths[i + 1];
    }
}

std::vector<glm::vec4> CascadedShadowMap::GetFrustumCornersWorldSpace(
    const glm::mat4& projection, const glm::mat4& view) {
    
    glm::mat4 invViewProj = glm::inverse(projection * view);
    
    std::vector<glm::vec4> frustumCorners;
    frustumCorners.reserve(8);
    
    // NDC corners
    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < 2; y++) {
            for (int z = 0; z < 2; z++) {
                glm::vec4 pt = invViewProj * glm::vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f
                );
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    
    return frustumCorners;
}

glm::mat4 CascadedShadowMap::CalculateLightSpaceMatrix(
    const std::vector<glm::vec4>& frustumCorners,
    const glm::vec3& lightDirection,
    int cascadeIndex) {
    
    // Calculate frustum center
    glm::vec3 center(0.0f);
    for (const auto& corner : frustumCorners) {
        center += glm::vec3(corner);
    }
    center /= static_cast<float>(frustumCorners.size());
    
    // Create a stable up vector (avoid parallel with light direction)
    glm::vec3 lightDir = glm::normalize(lightDirection);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDir, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    
    // Create light view matrix looking at frustum center
    glm::mat4 lightView = glm::lookAt(center - lightDir * 1.0f, center, up);
    
    // Transform frustum corners to light space and compute AABB
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    
    for (const auto& corner : frustumCorners) {
        glm::vec4 lightSpaceCorner = lightView * corner;
        minX = std::min(minX, lightSpaceCorner.x);
        maxX = std::max(maxX, lightSpaceCorner.x);
        minY = std::min(minY, lightSpaceCorner.y);
        maxY = std::max(maxY, lightSpaceCorner.y);
        minZ = std::min(minZ, lightSpaceCorner.z);
        maxZ = std::max(maxZ, lightSpaceCorner.z);
    }
    
    // Calculate the frustum size for shadow caster extension
    float frustumWidth = maxX - minX;
    float frustumHeight = maxY - minY;
    
    // Scale shadow caster extension based on cascade size
    float baseExtension = 50.0f;
    float cascadeScale = 1.0f + cascadeIndex * 0.5f;
    float shadowCasterExtension = baseExtension * cascadeScale;
    
    // Extend Z range to capture shadow casters behind the frustum
    minZ -= shadowCasterExtension;
    maxZ += 10.0f;
    
    // Add small padding to X and Y
    float xPadding = frustumWidth * 0.05f;
    float yPadding = frustumHeight * 0.05f;
    minX -= xPadding;
    maxX += xPadding;
    minY -= yPadding;
    maxY += yPadding;
    
    // For orthographic projection:
    // In light view space, -Z is forward (towards scene)
    // minZ is most negative (furthest into scene), maxZ is least negative (closest to light)
    // glm::ortho expects positive near/far values representing distance along -Z
    float zNear = -maxZ;
    float zFar = -minZ;
    
    // Ensure valid projection (zNear < zFar and both positive)
    if (zNear < 0.01f) zNear = 0.01f;
    if (zFar <= zNear) zFar = zNear + shadowCasterExtension + 100.0f;
    
    // Create orthographic projection
    glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, zNear, zFar);
    
    return lightProjection * lightView;
}

void CascadedShadowMap::UpdateCascades(
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix,
    const glm::vec3& lightDirection,
    float nearPlane, float farPlane) {
    
    // Calculate split depths
    CalculateSplitDepths(nearPlane, farPlane);
    
    // Calculate light space matrix for each cascade
    for (int i = 0; i < NUM_CASCADES; i++) {
        float cascadeNear = config.splitDepths[i];
        float cascadeFar = config.splitDepths[i + 1];
        
        // Create projection matrix for this cascade's frustum
        // Extract FOV and aspect from original projection
        float fov = 2.0f * std::atan(1.0f / projectionMatrix[1][1]);
        float aspect = projectionMatrix[1][1] / projectionMatrix[0][0];
        
        glm::mat4 cascadeProjection = glm::perspective(fov, aspect, cascadeNear, cascadeFar);
        
        // Get frustum corners for this cascade
        std::vector<glm::vec4> frustumCorners = GetFrustumCornersWorldSpace(cascadeProjection, viewMatrix);
        
        // Calculate light space matrix (pass cascade index for scaling)
        cascades[i].lightSpaceMatrix = CalculateLightSpaceMatrix(frustumCorners, lightDirection, i);
        cascades[i].splitDepth = cascadeFar;
    }
}

void CascadedShadowMap::BindForWriting(int cascadeIndex) {
    if (cascadeIndex < 0 || cascadeIndex >= NUM_CASCADES) {
        std::cerr << "[CSM] Invalid cascade index: " << cascadeIndex << std::endl;
        return;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTextureArray, 0, cascadeIndex);
    
    // CRITICAL: Ensure framebuffer is complete before proceeding
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[CSM] Framebuffer incomplete for cascade " << cascadeIndex << " (status: " << status << ")" << std::endl;
        return;
    }
    
    // Set viewport to shadow map resolution
    glViewport(0, 0, resolution, resolution);
    
    // CRITICAL: Ensure depth writing is enabled
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    // Clear depth buffer for this cascade layer
    glClear(GL_DEPTH_BUFFER_BIT);
}

void CascadedShadowMap::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CascadedShadowMap::BindForReading(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthTextureArray);
}

void CascadedShadowMap::GetLightSpaceMatrices(glm::mat4* matrices) const {
    for (int i = 0; i < NUM_CASCADES; i++) {
        matrices[i] = cascades[i].lightSpaceMatrix;
    }
}

void CascadedShadowMap::GetCascadeSplits(float* splits) const {
    for (int i = 0; i < NUM_CASCADES; i++) {
        splits[i] = cascades[i].splitDepth;
    }
}

// Debug function to print CSM state
void CascadedShadowMap::PrintDebugInfo() const {
    std::cout << "[CSM] Debug Info:" << std::endl;
    std::cout << "  Resolution: " << resolution << "x" << resolution << std::endl;
    std::cout << "  Split Lambda: " << splitLambda << std::endl;
    std::cout << "  Cascade Splits (far distances):" << std::endl;
    for (int i = 0; i < NUM_CASCADES; i++) {
        std::cout << "    Cascade " << i << ": " << cascades[i].splitDepth << std::endl;
    }
}

void CascadedShadowMap::Cleanup() {
    if (FBO) {
        glDeleteFramebuffers(1, &FBO);
        FBO = 0;
    }
    if (depthTextureArray) {
        glDeleteTextures(1, &depthTextureArray);
        depthTextureArray = 0;
    }
    std::cout << "[CSM] Cleaned up" << std::endl;
}
