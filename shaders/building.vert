#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform vec3 buildingScale; // NEW: Pass building scale for UV tiling

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // FIXED: Scale UVs based on building dimensions to prevent stretching
    // Vertical walls (front, back, left, right) tile based on height
    // Horizontal faces (top, bottom) tile based on width/depth
    vec2 scaledUVs = aTexCoords;
    
    // Detect which face we're on based on normal direction
    vec3 worldNormal = mat3(transpose(inverse(model))) * aNormal;
    vec3 absNormal = abs(worldNormal);
    
    // Vertical faces (walls) - tile vertically based on height
    if (absNormal.y < 0.5) {
        // Side walls: scale V coordinate by height for proper tiling
        scaledUVs.y = aTexCoords.y * buildingScale.y * 0.5; // 0.5 = tiling frequency
        scaledUVs.x = aTexCoords.x * max(buildingScale.x, buildingScale.z) * 0.5;
    }
    // Horizontal faces (top/bottom) - tile based on width/depth
    else {
        scaledUVs.x = aTexCoords.x * buildingScale.x * 0.5;
        scaledUVs.y = aTexCoords.y * buildingScale.z * 0.5;
    }
    
    TexCoords = scaledUVs;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
