#ifndef SKINNED_CHARACTER_H
#define SKINNED_CHARACTER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <tiny_gltf.h>

#include <string>
#include <vector>

// AABB structure for bounds
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
    
    AABB() : min(FLT_MAX), max(-FLT_MAX) {}
    
    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    
    glm::vec3 center() const {
        return (min + max) * 0.5f;
    }
    
    glm::vec3 size() const {
        return max - min;
    }
};

// Render options structure
struct RenderOptions {
    bool enableShadows = true;
    bool enablePCF = true;
    float bloomThreshold = 1.0f;
};

// Directional light structure
struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
};

// Point light structure
struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;
};

// Shadow data structure
struct ShadowData {
    glm::mat4 lightSpaceMatrix;
    GLuint shadowMapTexture;
};

class SkinnedCharacter {
public:
    SkinnedCharacter();
    ~SkinnedCharacter();
    
    // Initialization
    bool init(const std::string& modelPath, const std::string& animNameOptional = "");
    void cleanup();
    
    // Update and rendering
    void update(float dt);
    void render(const glm::mat4& view, const glm::mat4& proj,
                const DirectionalLight& dirLight, const PointLight& ptLight,
                const ShadowData& shadows, const RenderOptions& opts);
    
    // Render depth-only for shadow pass
    void renderDepth(const glm::mat4& lightSpaceMatrix);
    
    // Transform control
    void setWorldTransform(const glm::vec3& pos, float yawRadians);
    void setScale(float uniformScale);
    void setVisible(bool v);
    
    // Debug toggles
    void setDrawSkeleton(bool v);
    void setDrawBounds(bool v);
    void setForceEmissive(bool v);  // Force cyan emissive for visibility debug
    void setIgnoreMeshOffset(bool v) { m_ignoreMeshOffset = v; }  // Debug: bypass mesh offset
    bool getIgnoreMeshOffset() const { return m_ignoreMeshOffset; }
    
    // Query
    glm::vec3 getPosition() const { return m_position; }
    float getYaw() const { return m_yaw; }
    glm::vec3 getScale() const { return m_scale; }  // Get user-set scale
    AABB getWorldAABB() const;
    bool isVisible() const { return m_visible; }
    std::string getAnimationName() const { return m_currentAnimName; }
    float getAnimationTime() const { return m_animTime; }
    int getJointCount() const { return (int)m_jointMatrices.size(); }
    float getGroundOffsetY() const { return m_groundOffsetY; }  // Raw AABB min Y (model space, unscaled)
    float getAutoScale() const { return m_autoScale; }  // Internal auto-scale factor
    
private:
    // Model loading
    bool loadModel(const std::string& path);
    bool loadShaders();
    
    // Skeleton and animation setup
    void prepareSkeleton();
    void prepareAnimation(const std::string& animNameOptional);
    
    // Mesh setup
    void setupMesh();
    
    // Animation evaluation
    void evaluateAnimation(float time);
    void computeJointMatrices();
    
    // Auto-fit logic
    void computeAutoFit();
    
    // Rendering helpers
    void renderMesh(GLuint shaderProgram, const glm::mat4& mvp, const glm::mat4& model);
    void renderDebugSkeleton(const glm::mat4& vp);
    void renderDebugBounds(const glm::mat4& vp);
    
    // Utility functions
    glm::mat4 getNodeTransform(const tinygltf::Node& node);
    int findKeyframeIndex(const std::vector<float>& times, float t);
    glm::vec3 interpolateVec3(const std::vector<glm::vec3>& values, 
                              const std::vector<float>& times, float t);
    glm::quat interpolateQuat(const std::vector<glm::quat>& values,
                              const std::vector<float>& times, float t);
    
    // Shader compilation
    GLuint compileShader(GLenum type, const char* source);
    GLuint linkProgram(GLuint vertShader, GLuint fragShader);
    std::string loadShaderSource(const char* path);
    
private:
    // Model data
    tinygltf::Model m_model;
    std::string m_modelPath;
    
    // Mesh data
    GLuint m_vao;
    GLuint m_vbo_positions;
    GLuint m_vbo_normals;
    GLuint m_vbo_uvs;
    GLuint m_vbo_boneIDs;
    GLuint m_vbo_weights;
    GLuint m_ebo;
    GLsizei m_indexCount;
    GLenum m_indexType;
    
    // Skeleton data
    std::vector<int> m_joints;  // Joint node indices
    std::vector<glm::mat4> m_inverseBindMatrices;
    std::vector<int> m_jointParents;
    
    // Animation data
    int m_animIndex;
    std::string m_currentAnimName;
    float m_animTime;
    float m_animDuration;
    
    // Animation channels (per joint)
    struct Channel {
        int nodeIndex;
        std::vector<float> translationTimes;
        std::vector<glm::vec3> translationValues;
        std::vector<float> rotationTimes;
        std::vector<glm::quat> rotationValues;
        std::vector<float> scaleTimes;
        std::vector<glm::vec3> scaleValues;
    };
    std::vector<Channel> m_channels;
    
    // Runtime joint transforms
    std::vector<glm::vec3> m_jointPositions;  // Local T
    std::vector<glm::quat> m_jointRotations;  // Local R
    std::vector<glm::vec3> m_jointScales;     // Local S
    std::vector<glm::mat4> m_jointLocalTransforms;
    std::vector<glm::mat4> m_jointGlobalTransforms;
    std::vector<glm::mat4> m_jointMatrices;  // Final matrices for GPU
    
    // Shaders
    GLuint m_shaderProgram;
    GLuint m_depthShaderProgram;
    GLuint m_debugLineShader;
    
    // Transform
    glm::vec3 m_position;
    float m_yaw;
    glm::vec3 m_scale;
    
    // Auto-fit data
    AABB m_aabb;
    float m_meshHeight;
    glm::vec3 m_meshOffset;
    float m_yLift;
    float m_autoScale;
    float m_groundOffsetY;  // Computed offset: how much above ground plane the character origin sits
    
    // State
    bool m_visible;
    bool m_drawSkeleton;
    bool m_drawBounds;
    bool m_forceEmissive;
    bool m_ignoreMeshOffset = false;  // Debug toggle: when true, skip mesh offset in model matrix
    
    // Debug line rendering
    GLuint m_lineVAO;
    GLuint m_lineVBO;
};

#endif // SKINNED_CHARACTER_H
