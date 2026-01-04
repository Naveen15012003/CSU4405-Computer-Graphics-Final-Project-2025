#version 330 core

// Input attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aBoneIDs;
layout(location = 4) in vec4 aWeights;

// Uniforms
uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uBones[128];
uniform vec3 uMeshOffset;  // Offset to center and ground the mesh

void main()
{
    // Calculate total weight (should be 1.0, but may be 0 if no skinning)
    float totalWeight = aWeights.x + aWeights.y + aWeights.z + aWeights.w;
    
    vec4 skinnedPosition;
    
    if (totalWeight > 0.0001) {
        // Linear blend skinning (same as main vertex shader)
        mat4 skinMatrix = 
            aWeights.x * uBones[int(aBoneIDs.x)] +
            aWeights.y * uBones[int(aBoneIDs.y)] +
            aWeights.z * uBones[int(aBoneIDs.z)] +
            aWeights.w * uBones[int(aBoneIDs.w)];
        
        // Normalize if weights don't sum to 1
        if (abs(totalWeight - 1.0) > 0.0001) {
            skinMatrix = skinMatrix / totalWeight;
        }
        
        // Transform vertex position using skinning
        skinnedPosition = skinMatrix * vec4(aPosition, 1.0);
    } else {
        // No skinning - use position as-is
        skinnedPosition = vec4(aPosition, 1.0);
    }
    
    // Apply mesh offset to center and ground the character (in model space)
    skinnedPosition.xyz += uMeshOffset;
    
    // Transform to world space then to light space
    vec4 worldPos = uModel * skinnedPosition;
    gl_Position = uLightSpaceMatrix * worldPos;
}
