#version 330 core

// MRT outputs for HDR + Bloom
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in float ClipSpaceDepth;  // View space depth for cascade selection

// Texture
uniform sampler2D buildingTexture;

// Lights
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform float pointLightConstant;
uniform float pointLightLinear;
uniform float pointLightQuadratic;

// Camera
uniform vec3 viewPos;

// CSM uniforms
uniform sampler2DArray csmShadowMap;
uniform mat4 lightSpaceMatrices[4];
uniform float cascadeSplits[4];
uniform int numCascades;
uniform bool enableCSM;
uniform bool visualizeCascades;

// Legacy shadow map
uniform sampler2D shadowMap;

uniform bool enableShadows;
uniform bool uUsePCF;

// Bloom threshold
uniform float bloomThreshold;

// Blend region as percentage of cascade range
const float CASCADE_BLEND_FACTOR = 0.15;

// Get cascade index based on view space depth
int GetCascadeIndex()
{
    for (int i = 0; i < numCascades; i++) {
        if (ClipSpaceDepth < cascadeSplits[i]) {
            return i;
        }
    }
    return numCascades - 1;
}

// Calculate shadow for a specific cascade
float SampleCascadeShadow(int cascadeIndex, vec3 normal, vec3 lightDir)
{
    // Safety check for cascade index
    if (cascadeIndex < 0 || cascadeIndex >= numCascades) {
        return 0.0;  // No shadow for invalid cascade
    }
    
    vec4 fragPosLightSpace = lightSpaceMatrices[cascadeIndex] * vec4(FragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    // CRITICAL: Check if fragment is outside the shadow map bounds
    // Return NO SHADOW (0.0) if outside - this prevents dark artifacts at edges
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;  // Outside shadow map X/Y bounds - no shadow
    }
    
    // Check Z bounds - fragments behind or too far should not be shadowed
    if (projCoords.z > 1.0 || projCoords.z < 0.0) {
        return 0.0;  // Outside depth range - no shadow
    }
    
    float currentDepth = projCoords.z;
    
    // Improved bias calculation
    // Use a smaller base bias and scale appropriately for each cascade
    float baseBias = 0.0003;
    float cascadeScale = 1.0 + float(cascadeIndex) * 0.5;  // Gradual increase per cascade
    float slopeBias = max(0.003 * (1.0 - dot(normal, lightDir)), 0.0005);
    float bias = baseBias * cascadeScale + slopeBias;
    
    float shadow = 0.0;
    
    if (uUsePCF) {
        vec2 texelSize = 1.0 / vec2(textureSize(csmShadowMap, 0).xy);
        int sampleCount = 0;
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                vec2 offset = vec2(float(x), float(y)) * texelSize;
                vec2 sampleCoord = projCoords.xy + offset;
                
                // Only sample if within bounds
                if (sampleCoord.x >= 0.0 && sampleCoord.x <= 1.0 &&
                    sampleCoord.y >= 0.0 && sampleCoord.y <= 1.0) {
                    float pcfDepth = texture(csmShadowMap, vec3(sampleCoord, float(cascadeIndex))).r;
                    shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
                    sampleCount++;
                }
            }
        }
        if (sampleCount > 0) {
            shadow /= float(sampleCount);
        }
    } else {
        float closestDepth = texture(csmShadowMap, vec3(projCoords.xy, float(cascadeIndex))).r;
        shadow = (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
    }
    
    return shadow;
}

// CSM Shadow calculation with cascade blending
float CSMShadowCalculation(vec3 normal, vec3 lightDir)
{
    // If fragment is beyond all cascades, no shadow
    if (ClipSpaceDepth > cascadeSplits[numCascades - 1] * 1.1) {
        return 0.0;  // Beyond shadow range - no shadow
    }
    
    int cascadeIndex = GetCascadeIndex();
    float shadow = SampleCascadeShadow(cascadeIndex, normal, lightDir);
    
    // Blend with next cascade near boundaries
    if (cascadeIndex < numCascades - 1)
    {
        float currentSplit = cascadeSplits[cascadeIndex];
        float prevSplit = (cascadeIndex > 0) ? cascadeSplits[cascadeIndex - 1] : 0.0;
        float cascadeRange = currentSplit - prevSplit;
        float blendRegion = cascadeRange * CASCADE_BLEND_FACTOR;
        
        float distanceToEdge = currentSplit - ClipSpaceDepth;
        
        if (distanceToEdge < blendRegion && distanceToEdge > 0.0)
        {
            float nextShadow = SampleCascadeShadow(cascadeIndex + 1, normal, lightDir);
            float blendFactor = (blendRegion - distanceToEdge) / blendRegion;
            blendFactor = smoothstep(0.0, 1.0, blendFactor);
            shadow = mix(shadow, nextShadow, blendFactor);
        }
    }
    
    return shadow;
}

// Legacy shadow calculation (fallback)
float LegacyShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    // Return no shadow if outside bounds
    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    float shadow = 0.0;
    if (uUsePCF) {
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    } else {
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }
    return shadow;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(-dirLightDir);
    
    // Sample building texture
    vec3 albedo = texture(buildingTexture, TexCoords).rgb;
    
    // Ambient - slightly increased for better visibility
    vec3 ambient = 0.35 * dirLightColor * albedo;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLightColor * albedo;
    
    // Specular (minimal for buildings)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = spec * dirLightColor * 0.2;
    
    // Shadow calculation - use CSM when enabled, otherwise legacy
    float shadow = 0.0;
    if (enableShadows) {
        if (enableCSM && numCascades > 0) {
            shadow = CSMShadowCalculation(norm, lightDir);
        } else {
            shadow = LegacyShadowCalculation(FragPosLightSpace, norm, lightDir);
        }
    }
    
    // Clamp shadow to allow some ambient light through
    shadow = clamp(shadow, 0.0, 0.75);
    
    vec3 dirResult = ambient + (1.0 - shadow) * (diffuse + specular);
    
    // Point light
    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (pointLightConstant + pointLightLinear * distance + 
                               pointLightQuadratic * (distance * distance));
    
    vec3 pointLightDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointLightDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation * albedo;
    
    vec3 color = dirResult + pointDiffuse;
    
    // Cascade visualization overlay (debug feature)
    if (enableCSM && visualizeCascades) {
        int cascade = GetCascadeIndex();
        vec3 cascadeColor;
        if (cascade == 0) cascadeColor = vec3(1.0, 0.0, 0.0);       // Red - nearest
        else if (cascade == 1) cascadeColor = vec3(0.0, 1.0, 0.0);  // Green
        else if (cascade == 2) cascadeColor = vec3(0.0, 0.0, 1.0);  // Blue
        else cascadeColor = vec3(1.0, 1.0, 0.0);                     // Yellow - farthest
        color = mix(color, cascadeColor, 0.3);
    }
    
    FragColor = vec4(color, 1.0);
    
    // Brightness threshold for bloom
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = brightness > bloomThreshold ? vec4(color, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}
