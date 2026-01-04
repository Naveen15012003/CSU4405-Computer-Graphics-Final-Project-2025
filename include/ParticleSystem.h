#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>

// Particle types for different visual effects
enum class ParticleType {
    FIRE = 0,
    SMOKE = 1,
    SPARK = 2,
    MAGIC = 3
};

// Individual particle data
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float life;
    float maxLife;
    float size;
    float rotation;
    bool active;
    
    Particle() : position(0.0f), velocity(0.0f), color(1.0f), 
                 life(0.0f), maxLife(1.0f), size(1.0f), 
                 rotation(0.0f), active(false) {}
};

// Emitter configuration
struct EmitterConfig {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f);
    float spread = 0.5f;              // Cone spread angle (radians)
    float minSpeed = 1.0f;
    float maxSpeed = 3.0f;
    float minLife = 1.0f;
    float maxLife = 3.0f;
    float minSize = 0.1f;
    float maxSize = 0.5f;
    float emitRate = 50.0f;           // Particles per second
    glm::vec4 startColor = glm::vec4(1.0f);
    glm::vec4 endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, -2.0f, 0.0f);
    ParticleType type = ParticleType::FIRE;
    bool burst = false;               // Emit all at once vs continuous
    int burstCount = 100;
};

class ParticleSystem {
public:
    ParticleSystem(unsigned int maxParticles = 10000);
    ~ParticleSystem();
    
    // Initialize the particle system (call after OpenGL context is ready)
    bool Initialize();
    
    // Update particles (call every frame)
    void Update(float deltaTime);
    
    // Render particles
    void Render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::vec3& cameraPos);
    
    // Cleanup resources
    void Cleanup();
    
    // Emitter control
    void SetEmitterConfig(const EmitterConfig& config);
    EmitterConfig& GetEmitterConfig() { return emitterConfig; }
    
    void SetPosition(const glm::vec3& pos) { emitterConfig.position = pos; }
    glm::vec3 GetPosition() const { return emitterConfig.position; }
    
    void SetParticleType(ParticleType type) { emitterConfig.type = type; }
    ParticleType GetParticleType() const { return emitterConfig.type; }
    
    // Enable/disable emission
    void SetEmitting(bool emit) { isEmitting = emit; }
    bool IsEmitting() const { return isEmitting; }
    
    // Trigger a burst of particles
    void Burst(int count = -1);
    
    // Get active particle count
    int GetActiveCount() const { return activeParticles; }
    int GetMaxParticles() const { return maxParticles; }
    
    // Shader access (for external uniform setting)
    unsigned int GetShader() const { return shaderProgram; }
    
private:
    // Particle pool
    std::vector<Particle> particles;
    unsigned int maxParticles;
    int activeParticles;
    
    // Emitter settings
    EmitterConfig emitterConfig;
    bool isEmitting;
    float emitAccumulator;
    
    // OpenGL resources
    unsigned int VAO, VBO, instanceVBO;
    unsigned int shaderProgram;
    bool initialized;
    
    // Random number generation
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist01;
    
    // Time tracking
    float currentTime;
    
    // Internal methods
    void EmitParticle();
    void UpdateParticle(Particle& p, float dt);
    int FindDeadParticle();
    void UploadInstanceData();
    bool LoadShaders();
    void CreateBuffers();
    
    // Random helpers
    float RandomFloat(float min, float max);
    glm::vec3 RandomDirection(const glm::vec3& mainDir, float spread);
};
