#include "ParticleSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Helper: Load shader source from file
static std::string LoadShaderSource(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[ParticleSystem] Failed to open shader: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper: Compile shader
static unsigned int CompileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "[ParticleSystem] Shader compilation failed:\n" << infoLog << std::endl;
        return 0;
    }
    return shader;
}

ParticleSystem::ParticleSystem(unsigned int maxParticles)
    : maxParticles(maxParticles), activeParticles(0), isEmitting(true),
      emitAccumulator(0.0f), VAO(0), VBO(0), instanceVBO(0), 
      shaderProgram(0), initialized(false), currentTime(0.0f),
      rng(std::random_device{}()), dist01(0.0f, 1.0f)
{
    particles.resize(maxParticles);
}

ParticleSystem::~ParticleSystem() {
    Cleanup();
}

bool ParticleSystem::Initialize() {
    std::cout << "[ParticleSystem] Initializing with " << maxParticles << " max particles..." << std::endl;
    
    if (!LoadShaders()) {
        std::cerr << "[ParticleSystem] Failed to load shaders" << std::endl;
        return false;
    }
    
    CreateBuffers();
    
    initialized = true;
    std::cout << "[ParticleSystem] Initialized successfully" << std::endl;
    return true;
}

bool ParticleSystem::LoadShaders() {
    std::string vertexCode = LoadShaderSource("shaders/particle.vert");
    std::string fragmentCode = LoadShaderSource("shaders/particle.frag");
    
    if (vertexCode.empty() || fragmentCode.empty()) {
        return false;
    }
    
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode.c_str());
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());
    
    if (vertexShader == 0 || fragmentShader == 0) {
        return false;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "[ParticleSystem] Shader linking failed:\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    std::cout << "[ParticleSystem] Shaders loaded successfully" << std::endl;
    return true;
}

void ParticleSystem::CreateBuffers() {
    // Quad vertices for each particle (billboard)
    float quadVertices[] = {
        // Position (XY)   // TexCoord
        -0.5f, -0.5f,      0.0f, 0.0f,
         0.5f, -0.5f,      1.0f, 0.0f,
         0.5f,  0.5f,      1.0f, 1.0f,
        -0.5f, -0.5f,      0.0f, 0.0f,
         0.5f,  0.5f,      1.0f, 1.0f,
        -0.5f,  0.5f,      0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &instanceVBO);
    
    glBindVertexArray(VAO);
    
    // Quad vertex data (shared by all particles)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    // Instance data buffer
    // Layout: vec3 position, vec4 data (life, maxLife, size, rotation), vec4 color, vec3 velocity
    // Total: 14 floats per particle
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 14 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    // Particle position (location 2) - instanced
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
    glVertexAttribDivisor(2, 1);
    
    // Particle data (location 3) - instanced
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribDivisor(3, 1);
    
    // Particle color (location 4) - instanced
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(7 * sizeof(float)));
    glVertexAttribDivisor(4, 1);
    
    // Particle velocity (location 5) - instanced
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    
    glBindVertexArray(0);
    
    std::cout << "[ParticleSystem] Buffers created" << std::endl;
}

void ParticleSystem::Update(float deltaTime) {
    if (!initialized) return;
    
    currentTime += deltaTime;
    activeParticles = 0;
    
    // Update existing particles
    for (auto& p : particles) {
        if (p.active) {
            UpdateParticle(p, deltaTime);
            if (p.active) {
                activeParticles++;
            }
        }
    }
    
    // Emit new particles
    if (isEmitting && !emitterConfig.burst) {
        emitAccumulator += emitterConfig.emitRate * deltaTime;
        while (emitAccumulator >= 1.0f) {
            EmitParticle();
            emitAccumulator -= 1.0f;
        }
    }
    
    // Upload instance data to GPU
    UploadInstanceData();
}

void ParticleSystem::UpdateParticle(Particle& p, float dt) {
    p.life -= dt;
    
    if (p.life <= 0.0f) {
        p.active = false;
        return;
    }
    
    // Apply gravity
    p.velocity += emitterConfig.gravity * dt;
    
    // Update position
    p.position += p.velocity * dt;
    
    // Update rotation
    p.rotation += dt * 2.0f;
    
    // Interpolate color based on life
    float lifeRatio = 1.0f - (p.life / p.maxLife);
    p.color = glm::mix(emitterConfig.startColor, emitterConfig.endColor, lifeRatio);
}

void ParticleSystem::EmitParticle() {
    int index = FindDeadParticle();
    if (index < 0) return;
    
    Particle& p = particles[index];
    
    // Position with slight randomness
    p.position = emitterConfig.position;
    p.position.x += RandomFloat(-0.1f, 0.1f);
    p.position.z += RandomFloat(-0.1f, 0.1f);
    
    // Random direction within cone
    glm::vec3 dir = RandomDirection(emitterConfig.direction, emitterConfig.spread);
    float speed = RandomFloat(emitterConfig.minSpeed, emitterConfig.maxSpeed);
    p.velocity = dir * speed;
    
    // Random lifetime
    p.life = RandomFloat(emitterConfig.minLife, emitterConfig.maxLife);
    p.maxLife = p.life;
    
    // Random size
    p.size = RandomFloat(emitterConfig.minSize, emitterConfig.maxSize);
    
    // Random initial rotation
    p.rotation = RandomFloat(0.0f, 6.28318f);
    
    // Initial color
    p.color = emitterConfig.startColor;
    
    p.active = true;
}

int ParticleSystem::FindDeadParticle() {
    // First search from last used particle
    static int lastUsed = 0;
    
    for (int i = lastUsed; i < (int)maxParticles; i++) {
        if (!particles[i].active) {
            lastUsed = i;
            return i;
        }
    }
    
    for (int i = 0; i < lastUsed; i++) {
        if (!particles[i].active) {
            lastUsed = i;
            return i;
        }
    }
    
    // All particles are alive, override first one
    lastUsed = 0;
    return 0;
}

void ParticleSystem::UploadInstanceData() {
    if (activeParticles == 0) return;
    
    // Prepare instance data
    std::vector<float> instanceData;
    instanceData.reserve(activeParticles * 14);
    
    for (const auto& p : particles) {
        if (p.active) {
            // Position (3 floats)
            instanceData.push_back(p.position.x);
            instanceData.push_back(p.position.y);
            instanceData.push_back(p.position.z);
            
            // Data: life, maxLife, size, rotation (4 floats)
            instanceData.push_back(p.life);
            instanceData.push_back(p.maxLife);
            instanceData.push_back(p.size);
            instanceData.push_back(p.rotation);
            
            // Color (4 floats)
            instanceData.push_back(p.color.r);
            instanceData.push_back(p.color.g);
            instanceData.push_back(p.color.b);
            instanceData.push_back(p.color.a);
            
            // Velocity (3 floats)
            instanceData.push_back(p.velocity.x);
            instanceData.push_back(p.velocity.y);
            instanceData.push_back(p.velocity.z);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instanceData.size() * sizeof(float), instanceData.data());
}

void ParticleSystem::Render(const glm::mat4& view, const glm::mat4& projection, 
                            const glm::vec3& cameraPos) {
    if (!initialized || activeParticles == 0) return;
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for fire/glow
    glDepthMask(GL_FALSE);  // Don't write to depth buffer
    
    glUseProgram(shaderProgram);
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Calculate camera right and up vectors for billboarding
    glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
    
    glUniform3fv(glGetUniformLocation(shaderProgram, "cameraRight"), 1, glm::value_ptr(cameraRight));
    glUniform3fv(glGetUniformLocation(shaderProgram, "cameraUp"), 1, glm::value_ptr(cameraUp));
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), currentTime);
    
    // Particle type and texture settings
    glUniform1i(glGetUniformLocation(shaderProgram, "particleType"), static_cast<int>(emitterConfig.type));
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);  // Use procedural
    
    // Draw instanced
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, activeParticles);
    glBindVertexArray(0);
    
    // Restore state
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ParticleSystem::Burst(int count) {
    int toEmit = (count < 0) ? emitterConfig.burstCount : count;
    for (int i = 0; i < toEmit; i++) {
        EmitParticle();
    }
}

void ParticleSystem::SetEmitterConfig(const EmitterConfig& config) {
    emitterConfig = config;
}

float ParticleSystem::RandomFloat(float min, float max) {
    return min + dist01(rng) * (max - min);
}

glm::vec3 ParticleSystem::RandomDirection(const glm::vec3& mainDir, float spread) {
    // Random angle within cone
    float angle = RandomFloat(0.0f, spread);
    float phi = RandomFloat(0.0f, 6.28318f);
    
    // Create perpendicular vectors
    glm::vec3 up = glm::abs(mainDir.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(mainDir, up));
    glm::vec3 forward = glm::normalize(glm::cross(right, mainDir));
    
    // Rotate main direction
    float sinAngle = sin(angle);
    float cosAngle = cos(angle);
    
    glm::vec3 result = mainDir * cosAngle + 
                       (right * cos(phi) + forward * sin(phi)) * sinAngle;
    
    return glm::normalize(result);
}

void ParticleSystem::Cleanup() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
    
    VAO = VBO = instanceVBO = shaderProgram = 0;
    initialized = false;
    particles.clear();
    
    std::cout << "[ParticleSystem] Cleaned up" << std::endl;
}
