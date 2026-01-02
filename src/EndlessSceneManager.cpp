#include "EndlessSceneManager.h"
#include <iostream>
#include <cmath>
#include <random>
#include <glm/gtc/type_ptr.hpp>

// ? ENHANCED visual variety - BIGGER PROPS (local constants for cpp only)
const float MIN_PROP_SIZE = 0.8f;    // Increased from 0.3f
const float MAX_PROP_SIZE = 5.0f;    // Increased from 2.5f
const float SPAWN_RADIUS = 25.0f;    // Increased from 20.0f
const float MIN_HEIGHT = 0.5f;       // Raised minimum
const float MAX_HEIGHT = 6.0f;       // Increased max
const int ACTIVE_RANGE_RADIUS = 2;   // Active range radius

EndlessSceneManager::EndlessSceneManager()
    : m_propModel(nullptr)
    , m_shader(0)
    , m_currentSegmentIndex(0)
    , m_totalSegmentsGenerated(0)
    , m_initialized(false)
    , m_groundVAO(0)
    , m_groundVBO(0)
{
}

EndlessSceneManager::~EndlessSceneManager()
{
    cleanup();
}

void EndlessSceneManager::initializeGroundMesh()
{
    // Create a simple quad for ground planes
    float groundVertices[] = {
        // Positions          // Normals         // UVs
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  10.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f,
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 10.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f
    };
    
    glGenVertexArrays(1, &m_groundVAO);
    glGenBuffers(1, &m_groundVBO);
    
    glBindVertexArray(m_groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    
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

void EndlessSceneManager::initialize(Model* propModel, unsigned int shader)
{
    m_propModel = propModel;
    m_shader = shader;

    std::cout << "[EndlessScene] Initializing..." << std::endl;

    // Initialize ground mesh
    initializeGroundMesh();

    // Create initial segments
    m_segments.resize(NUM_SEGMENTS);
    for (int i = 0; i < NUM_SEGMENTS; ++i)
    {
        int segmentId = i - ACTIVE_RANGE_RADIUS;
        m_segments[i].id = segmentId;
        generateSegment(m_segments[i], segmentId);
    }

    m_totalSegmentsGenerated = NUM_SEGMENTS;
    m_initialized = true;

    std::cout << "[EndlessScene] Initialized with " << NUM_SEGMENTS << " segments" << std::endl;
    std::cout << "[EndlessScene]   Segment length: " << SEGMENT_LENGTH << " units" << std::endl;
    std::cout << "[EndlessScene]   Props per segment: " << PROPS_PER_SEGMENT << std::endl;
    std::cout << "[EndlessScene]   Active range: [" << -ACTIVE_RANGE_RADIUS << ", " << ACTIVE_RANGE_RADIUS << "]" << std::endl;
    std::cout << "[EndlessScene]   ? ENHANCED VISIBILITY MODE with GROUND ?" << std::endl;
}

void EndlessSceneManager::update(float deltaTime, const glm::vec3& cameraPos)
{
    int newSegmentIndex = static_cast<int>(std::floor(cameraPos.z / SEGMENT_LENGTH));

    if (newSegmentIndex != m_currentSegmentIndex)
    {
        std::cout << "[EndlessScene] ?? Player moved to segment " << newSegmentIndex << std::endl;
        m_currentSegmentIndex = newSegmentIndex;

        // Recycle segments outside active range
        for (auto& segment : m_segments)
        {
            int distance = std::abs(segment.id - m_currentSegmentIndex);
            if (distance > ACTIVE_RANGE_RADIUS)
            {
                // Find the farthest segment we need
                int direction = (cameraPos.z > (segment.id * SEGMENT_LENGTH)) ? 1 : -1;
                int newId = m_currentSegmentIndex + direction * (ACTIVE_RANGE_RADIUS + (distance - ACTIVE_RANGE_RADIUS - 1));

                std::cout << "[EndlessScene] ??  Recycling segment " << segment.id << " -> " << newId << std::endl;

                recycleSegment(segment.id, newId);
                m_totalSegmentsGenerated++;
            }
        }
    }
}

void EndlessSceneManager::render(const glm::mat4& viewProj)
{
    // DISABLED: Phase 8 prop rendering (keeping only ground)
    // Props are disabled to focus on the 3 animated cubes from earlier phases
    /*
    if (!m_propModel || m_shader == 0) return;

    glUseProgram(m_shader);

    int visibleSegments = 0;
    int totalPropsRendered = 0;

    for (const auto& segment : m_segments)
    {
        if (!segment.isVisible) continue;

        visibleSegments++;

        for (const auto& prop : segment.props)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, prop.position);
            
            // Apply rotations
            if (glm::length(prop.rotation) > 0.001f)
            {
                model = glm::rotate(model, prop.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, prop.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, prop.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            }
            
            model = glm::scale(model, prop.scale);

            glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

            // Set prop color for variety
            glUniform3fv(glGetUniformLocation(m_shader, "objectColor"), 1, glm::value_ptr(prop.color));

            m_propModel->Draw(m_shader);
            totalPropsRendered++;
        }
    }

    // ? ENHANCED debug output (reduced frequency)
    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0)
    {
        std::cout << "[EndlessScene] ?? Segment " << m_currentSegmentIndex 
                  << " | Visible: " << visibleSegments << "/" << NUM_SEGMENTS
                  << " | Props: " << totalPropsRendered 
                  << " | Total Generated: " << m_totalSegmentsGenerated << std::endl;
    }
    */
}

void EndlessSceneManager::renderGround(const glm::mat4& view, const glm::mat4& projection)
{
    // DISABLED: Phase 8 ground rendering (using original ground plane instead)
    /*
    if (!m_propModel || m_shader == 0 || m_groundVAO == 0) return;
    
    glUseProgram(m_shader);
    
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(m_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
    glBindVertexArray(m_groundVAO);
    
    for (const auto& segment : m_segments)
    {
        if (!segment.isVisible) continue;
        
        const auto& ground = segment.groundSegment;
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, ground.position);
        model = glm::scale(model, ground.scale);
        
        glUniformMatrix4fv(glGetUniformLocation(m_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(m_shader, "objectColor"), 1, glm::value_ptr(ground.color));
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
    */
}

void EndlessSceneManager::cleanup()
{
    if (m_groundVAO != 0)
    {
        glDeleteVertexArrays(1, &m_groundVAO);
        glDeleteBuffers(1, &m_groundVBO);
        m_groundVAO = 0;
        m_groundVBO = 0;
    }
    
    m_segments.clear();
    m_propModel = nullptr;
    m_shader = 0;
    m_initialized = false;
}

// ? NEW: Add boundary markers function
void EndlessSceneManager::addBoundaryMarkers(Segment& segment)
{
    // Add 4 corner markers for segment boundaries (TALL and BRIGHT)
    const float markerSize = 3.0f;
    const float markerHeight = 8.0f;
    float segmentZ = segment.id * SEGMENT_LENGTH;
    
    std::vector<glm::vec3> corners = {
        glm::vec3(-SPAWN_RADIUS, 0.0f, segmentZ - SEGMENT_LENGTH * 0.5f),
        glm::vec3( SPAWN_RADIUS, 0.0f, segmentZ - SEGMENT_LENGTH * 0.5f),
        glm::vec3(-SPAWN_RADIUS, 0.0f, segmentZ + SEGMENT_LENGTH * 0.5f),
        glm::vec3( SPAWN_RADIUS, 0.0f, segmentZ + SEGMENT_LENGTH * 0.5f)
    };
    
    for (const auto& corner : corners)
    {
        Prop marker;
        marker.position = corner;
        marker.position.y = markerHeight / 2.0f; // Center at half height
        marker.scale = glm::vec3(markerSize, markerHeight, markerSize);
        marker.rotation = glm::vec3(0.0f);
        
        // Bright yellow/orange markers (UNMISSABLE)
        marker.color = glm::vec3(1.0f, 0.8f, 0.0f);
        marker.type = 3; // Special marker type
        
        segment.props.push_back(marker);
    }
}

void EndlessSceneManager::generateGroundSegment(Segment& segment, int segmentId)
{
    float segmentZ = segmentId * SEGMENT_LENGTH;
    
    segment.groundSegment.position = glm::vec3(0.0f, -0.01f, segmentZ); // Slightly below props
    segment.groundSegment.scale = glm::vec3(SPAWN_RADIUS * 2.0f, 1.0f, SEGMENT_LENGTH);
    segment.groundSegment.segmentId = segmentId;
    
    // Deterministic color based on segment ID
    uint32_t seed = GLOBAL_SEED ^ static_cast<uint32_t>(std::abs(segmentId) * 123);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> colorDist(0.0f, 0.2f);
    
    // Base ground color with variation
    segment.groundSegment.color = glm::vec3(
        0.3f + colorDist(rng),
        0.4f + colorDist(rng),
        0.3f + colorDist(rng)
    );
}

void EndlessSceneManager::generateSegment(Segment& segment, int segmentId)
{
    segment.id = segmentId;
    segment.props.clear();
    segment.props.reserve(PROPS_PER_SEGMENT + 4); // +4 for boundary markers

    // Generate ground segment first
    generateGroundSegment(segment, segmentId);

    // Deterministic seed based on segment ID
    uint32_t seed = GLOBAL_SEED ^ static_cast<uint32_t>(std::abs(segmentId));
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(-SPAWN_RADIUS, SPAWN_RADIUS);
    std::uniform_real_distribution<float> sizeDist(MIN_PROP_SIZE, MAX_PROP_SIZE);
    std::uniform_real_distribution<float> heightDist(MIN_HEIGHT, MAX_HEIGHT);
    std::uniform_real_distribution<float> rotDist(0.0f, 6.28318f); // 2*PI
    std::uniform_real_distribution<float> tiltDist(-0.3f, 0.3f); // Slight tilt for variety
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> typeDist(0, 2); // 3 types of props

    // ? Calculate segment-specific color tint (golden ratio hashing)
    float segmentHue = std::fmod(static_cast<float>(std::abs(segmentId)) * 0.618f, 1.0f);
    glm::vec3 segmentTint = glm::vec3(
        std::sin(segmentHue * 6.28318f) * 0.3f + 0.7f,
        std::sin((segmentHue + 0.333f) * 6.28318f) * 0.3f + 0.7f,
        std::sin((segmentHue + 0.666f) * 6.28318f) * 0.3f + 0.7f
    );

    float segmentZ = segmentId * SEGMENT_LENGTH;

    for (int i = 0; i < PROPS_PER_SEGMENT; ++i)
    {
        Prop prop;
        
        // Position with better spacing
        float angle = (static_cast<float>(i) / PROPS_PER_SEGMENT) * 6.28318f + rotDist(rng) * 0.5f;
        float radius = posDist(rng);
        
        prop.position.x = radius * std::cos(angle);
        prop.position.z = segmentZ + posDist(rng) * 0.5f * SEGMENT_LENGTH;
        prop.position.y = heightDist(rng);

        // ? BIGGER varied sizes
        float baseSize = sizeDist(rng);
        int propType = typeDist(rng);
        
        switch (propType)
        {
            case 0: // Tall thin props (trees/pillars) - GREEN TINT
                prop.scale = glm::vec3(baseSize * 0.5f, baseSize * 2.5f, baseSize * 0.5f); // Taller
                prop.color = glm::vec3(
                    0.1f + colorDist(rng) * 0.2f, 
                    0.6f + colorDist(rng) * 0.4f, 
                    0.1f + colorDist(rng) * 0.2f
                );
                break;
                
            case 1: // Wide flat props (platforms/rocks) - ORANGE/BROWN TINT
                prop.scale = glm::vec3(baseSize * 2.0f, baseSize * 0.4f, baseSize * 2.0f); // Wider
                prop.color = glm::vec3(
                    0.8f + colorDist(rng) * 0.2f, 
                    0.4f + colorDist(rng) * 0.3f, 
                    0.1f + colorDist(rng) * 0.2f
                );
                break;
                
            case 2: // Normal cubes (buildings/boxes) - BLUE/PURPLE TINT
                prop.scale = glm::vec3(baseSize * 1.2f, baseSize * 1.2f, baseSize * 1.2f); // Bigger
                prop.color = glm::vec3(
                    0.2f + colorDist(rng) * 0.3f, 
                    0.2f + colorDist(rng) * 0.3f, 
                    0.7f + colorDist(rng) * 0.3f
                );
                break;
        }

        // ? Apply segment-specific color tint
        prop.color = prop.color * segmentTint;

        // Varied rotations
        prop.rotation = glm::vec3(tiltDist(rng), rotDist(rng), tiltDist(rng));
        prop.type = propType;

        segment.props.push_back(prop);
    }

    // ? ADD BOUNDARY MARKERS (tall yellow pillars at corners)
    addBoundaryMarkers(segment);

    // Compute simple AABB for frustum culling
    segment.aabbMin = glm::vec3(-SPAWN_RADIUS, 0.0f, segmentZ - SEGMENT_LENGTH * 0.5f);
    segment.aabbMax = glm::vec3(SPAWN_RADIUS, MAX_HEIGHT, segmentZ + SEGMENT_LENGTH * 0.5f);
    segment.isVisible = true;
}

void EndlessSceneManager::recycleSegment(int oldSegmentId, int newSegmentId)
{
    // Find segment with oldSegmentId
    for (auto& segment : m_segments)
    {
        if (segment.id == oldSegmentId)
        {
            generateSegment(segment, newSegmentId);
            break;
        }
    }
}

void EndlessSceneManager::updateInstanceBuffers(Segment& segment)
{
    // This can be used for instanced rendering in the future
    // For now, we render each prop individually
}

bool EndlessSceneManager::isSegmentVisible(const Segment& segment, const glm::mat4& viewProjection)
{
    // Simple frustum culling based on AABB
    // For now, return true (all segments visible)
    return true;
}

int EndlessSceneManager::getSegmentBufferIndex(int segmentId) const
{
    for (size_t i = 0; i < m_segments.size(); ++i)
    {
        if (m_segments[i].id == segmentId)
            return static_cast<int>(i);
    }
    return -1;
}
