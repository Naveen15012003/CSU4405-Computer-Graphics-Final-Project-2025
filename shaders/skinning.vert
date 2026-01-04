#version 330 core

// Input attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aBoneIDs;   // vec4 with float bone IDs
layout(location = 4) in vec4 aWeights;

// Output to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uBones[128];  // Max 128 joints
uniform vec3 uMeshOffset;  // Offset to center and ground the mesh

void main()
{
    // Calculate total weight (should be 1.0, but may be 0 if no skinning)
    float totalWeight = aWeights.x + aWeights.y + aWeights.z + aWeights.w;
    
    vec4 skinnedPosition;
    vec3 skinnedNormal;
    
    if (totalWeight > 0.0001) {
        // Linear blend skinning
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
        
        // Transform normal using skinning (use upper 3x3 for normals)
        mat3 skinMatrix3 = mat3(skinMatrix);
        skinnedNormal = skinMatrix3 * aNormal;
    } else {
        // No skinning - use position as-is
        skinnedPosition = vec4(aPosition, 1.0);
        skinnedNormal = aNormal;
    }
    
    // Apply mesh offset AFTER skinning to center and ground the animated character
    // This moves the skinned mesh so feet touch Y=0
    skinnedPosition.xyz += uMeshOffset;
    
    // Apply model matrix (scale, rotation, translation to world)
    vec4 worldPos = uModel * skinnedPosition;
    FragPos = worldPos.xyz;
    
    // Transform normal to world space
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    Normal = normalize(normalMatrix * skinnedNormal);
    
    // Pass through texture coordinate
    TexCoord = aUV;
    
    // Compute light space position for shadows
    FragPosLightSpace = uLightSpaceMatrix * worldPos;
    
    // Transform to clip space
    gl_Position = uProjection * uView * worldPos;
}
