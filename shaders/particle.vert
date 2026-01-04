#version 330 core

// Per-particle attributes (instanced)
layout (location = 0) in vec3 aPosition;      // Base quad vertex position
layout (location = 1) in vec2 aTexCoord;      // Texture coordinates
layout (location = 2) in vec3 aParticlePos;   // Particle world position (instanced)
layout (location = 3) in vec4 aParticleData;  // x=life, y=maxLife, z=size, w=rotation (instanced)
layout (location = 4) in vec4 aParticleColor; // Particle color with alpha (instanced)
layout (location = 5) in vec3 aVelocity;      // Particle velocity (instanced)

out vec2 TexCoords;
out vec4 ParticleColor;
out float LifeRatio;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;    // Camera right vector for billboarding
uniform vec3 cameraUp;       // Camera up vector for billboarding
uniform float time;          // Current time for animation

void main()
{
    // Calculate life ratio (0 = just born, 1 = about to die)
    float life = aParticleData.x;
    float maxLife = aParticleData.y;
    LifeRatio = 1.0 - (life / maxLife);
    
    // Particle size with life-based scaling
    float size = aParticleData.z;
    // Particles grow slightly then shrink as they die
    float sizeMultiplier = sin(LifeRatio * 3.14159) * 0.5 + 0.5;
    size *= sizeMultiplier;
    
    // Rotation
    float rotation = aParticleData.w + time * 0.5;
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    
    // Rotate the quad vertex
    vec2 rotatedPos;
    rotatedPos.x = aPosition.x * cosR - aPosition.y * sinR;
    rotatedPos.y = aPosition.x * sinR + aPosition.y * cosR;
    
    // Billboard: always face camera
    vec3 vertexPosition = aParticlePos 
        + cameraRight * rotatedPos.x * size 
        + cameraUp * rotatedPos.y * size;
    
    gl_Position = projection * view * vec4(vertexPosition, 1.0);
    
    TexCoords = aTexCoord;
    
    // Color with alpha fade based on life
    ParticleColor = aParticleColor;
    // Fade out as particle dies
    ParticleColor.a *= (1.0 - LifeRatio * LifeRatio);
}
