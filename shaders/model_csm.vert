#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out float ClipSpaceDepth;  // For cascade selection

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;  // Legacy shadow map (cascade 0 or single shadow)

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    TexCoords = aTexCoords;
    
    // Legacy light space position (for fallback)
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    
    // Calculate view space position for cascade selection
    vec4 viewPos = view * worldPos;
    ClipSpaceDepth = -viewPos.z;  // Positive depth in view space
    
    gl_Position = projection * viewPos;
}
