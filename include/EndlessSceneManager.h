#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <vector>
#include <random>
#include "Model.h"

// Configuration for endless scene generation
const int NUM_SEGMENTS = 5;              // Total segments in circular buffer
const float SEGMENT_LENGTH = 50.0f;      // Length of each segment
const int PROPS_PER_SEGMENT = 50;        // INCREASED from 30 for better visibility
const int ACTIVE_RANGE_BACK = -2;        // How many segments behind player
const int ACTIVE_RANGE_FORWARD = 2;      // How many segments ahead of player
const unsigned int GLOBAL_SEED = 42;     // Fixed seed for determinism

// Prop data structure
struct Prop {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    glm::vec3 color;      // Per-prop color for variety
    int type;             // Prop type for different shapes
};

// Ground segment structure
struct GroundSegment {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    int segmentId;
    
    GroundSegment() : position(0.0f), scale(1.0f), color(1.0f), segmentId(0) {}
};

// Segment structure
struct Segment {
    int id;                          // Unique segment identifier
    std::vector<Prop> props;        // Props in this segment
    GroundSegment groundSegment;    // Ground plane for this segment
    std::vector<glm::mat4> instanceMatrices;  // Instance transforms
    std::vector<glm::vec3> instanceColors;    // Instance colors
    GLuint instanceVBO;              // VBO for instance data
    GLuint colorVBO;                 // VBO for color data
    bool isVisible;                  // Frustum culling flag
    glm::vec3 aabbMin;              // AABB for frustum culling
    glm::vec3 aabbMax;
    
    Segment() : id(0), instanceVBO(0), colorVBO(0), isVisible(true) {}
};

class EndlessSceneManager {
public:
    EndlessSceneManager();
    ~EndlessSceneManager();
    
    // Initialize the endless scene system
    void initialize(Model* propModel, unsigned int shader);
    
    // Update based on camera position
    void update(float deltaTime, const glm::vec3& cameraPosition);
    
    // Render visible segments
    void render(const glm::mat4& viewProjection);
    
    // Render ground planes
    void renderGround(const glm::mat4& view, const glm::mat4& projection);
    
    // Cleanup
    void cleanup();
    
    // Query methods
    int getCurrentSegmentIndex() const { return m_currentSegmentIndex; }
    int getTotalSegmentsGenerated() const { return m_totalSegmentsGenerated; }
    
private:
    // Generate props for a segment deterministically
    void generateSegment(Segment& segment, int segmentId);
    
    // Add segment boundary visual markers
    void addBoundaryMarkers(Segment& segment);
    
    // Generate ground segment
    void generateGroundSegment(Segment& segment, int segmentId);
    
    // Recycle a segment to a new position
    void recycleSegment(int oldSegmentId, int newSegmentId);
    
    // Update instance buffers for a segment
    void updateInstanceBuffers(Segment& segment);
    
    // Frustum culling test
    bool isSegmentVisible(const Segment& segment, const glm::mat4& viewProjection);
    
    // Helper to get segment index in circular buffer
    int getSegmentBufferIndex(int segmentId) const;
    
    // Initialize ground plane mesh
    void initializeGroundMesh();
    
    // Data members
    std::vector<Segment> m_segments;       // Circular buffer of segments
    Model* m_propModel;                    // Reference to prop model
    unsigned int m_shader;                 // Shader program
    int m_currentSegmentIndex;             // Current segment player is in
    int m_minActiveSegment;                // Min active segment ID
    int m_maxActiveSegment;                // Max active segment ID
    int m_totalSegmentsGenerated;          // Total segments created (for stats)
    bool m_initialized;
    
    // Ground rendering
    unsigned int m_groundVAO;
    unsigned int m_groundVBO;
};
