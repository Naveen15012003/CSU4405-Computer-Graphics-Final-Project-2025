#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out float ClipSpaceDepth;  // For CSM cascade selection

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform vec3 buildingScale; // Pass building scale for UV tiling

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Scale UVs based on building dimensions to prevent stretching
    vec2 scaledUVs = aTexCoords;
    vec3 worldNormal = mat3(transpose(inverse(model))) * aNormal;
    vec3 absNormal = abs(worldNormal);
    
    // Vertical faces (walls) - tile vertically based on height
    if (absNormal.y < 0.5) {
        scaledUVs.y = aTexCoords.y * buildingScale.y * 0.5;
        scaledUVs.x = aTexCoords.x * max(buildingScale.x, buildingScale.z) * 0.5;
    }
    // Horizontal faces (top/bottom) - tile based on width/depth
    else {
        scaledUVs.x = aTexCoords.x * buildingScale.x * 0.5;
        scaledUVs.y = aTexCoords.y * buildingScale.z * 0.5;
    }
    
    TexCoords = scaledUVs;
    
    // Legacy light space position (for fallback)
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    
    // Calculate view space depth for CSM cascade selection
    vec4 viewPos = view * worldPos;
    ClipSpaceDepth = -viewPos.z;  // Positive depth in view space
    
    gl_Position = projection * viewPos;
}
