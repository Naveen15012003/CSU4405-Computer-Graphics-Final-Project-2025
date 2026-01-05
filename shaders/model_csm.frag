#version 330 core

// MRT outputs for HDR + Bloom
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in float ClipSpaceDepth;  // View space depth for cascade selection

struct Material {
    sampler2D diffuse1;
    float shininess;
};

uniform Material material;

uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform float pointLightConstant;
uniform float pointLightLinear;
uniform float pointLightQuadratic;
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
uniform bool enableGammaCorrection;
uniform float bloomThreshold;
uniform mat4 view;

// Blend region as percentage of cascade range (0.1 = 10% of cascade for blending)
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
    // Transform fragment position to light space for the selected cascade
    vec4 fragPosLightSpace = lightSpaceMatrices[cascadeIndex] * vec4(FragPos, 1.0);
    
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform from [-1,1] to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if outside shadow map bounds
    if (projCoords.z > 1.0 || projCoords.z < 0.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }
    
    float currentDepth = projCoords.z;
    
    // Calculate bias
    float baseBias = 0.0005;
    float cascadeScale = float(cascadeIndex + 1);
    float slopeBias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float bias = baseBias * cascadeScale + slopeBias;
    
    float shadow = 0.0;
    
    if (uUsePCF) {
        vec2 texelSize = 1.0 / vec2(textureSize(csmShadowMap, 0).xy);
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                vec2 offset = vec2(float(x), float(y)) * texelSize;
                float pcfDepth = texture(csmShadowMap, vec3(projCoords.xy + offset, float(cascadeIndex))).r;
                shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    } else {
        float closestDepth = texture(csmShadowMap, vec3(projCoords.xy, float(cascadeIndex))).r;
        shadow = (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
    }
    
    return shadow;
}

// CSM Shadow calculation with cascade blending to eliminate seams
float CSMShadowCalculation(vec3 normal, vec3 lightDir)
{
    int cascadeIndex = GetCascadeIndex();
    
    // Calculate shadow for current cascade
    float shadow = SampleCascadeShadow(cascadeIndex, normal, lightDir);
    
    // Blend with next cascade near boundaries to eliminate seams
    if (cascadeIndex < numCascades - 1)
    {
        // Calculate blend region
        float currentSplit = cascadeSplits[cascadeIndex];
        float prevSplit = (cascadeIndex > 0) ? cascadeSplits[cascadeIndex - 1] : 0.0;
        float cascadeRange = currentSplit - prevSplit;
        float blendRegion = cascadeRange * CASCADE_BLEND_FACTOR;
        
        // Calculate how close we are to the edge of current cascade
        float distanceToEdge = currentSplit - ClipSpaceDepth;
        
        // If within blend region, blend with next cascade
        if (distanceToEdge < blendRegion && distanceToEdge > 0.0)
        {
            float nextShadow = SampleCascadeShadow(cascadeIndex + 1, normal, lightDir);
            float blendFactor = (blendRegion - distanceToEdge) / blendRegion;
            blendFactor = smoothstep(0.0, 1.0, blendFactor);  // Smooth interpolation
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
    
    if (projCoords.z > 1.0)
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
    vec4 texColor = texture(material.diffuse1, TexCoords);
    
    vec3 albedo;
    if (texColor.a < 0.01 || (texColor.r + texColor.g + texColor.b < 0.01)) {
        albedo = vec3(0.7, 0.75, 0.8);
    } else {
        albedo = texColor.rgb;
    }
    
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 lightDir = normalize(-dirLightDir);
    
    // Ambient
    vec3 ambient = 0.3 * dirLightColor * albedo;
    
    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLightColor;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = spec * dirLightColor * 0.5;
    
    // Shadow calculation - use CSM when enabled, otherwise legacy
    float shadow = 0.0;
    if (enableShadows) {
        if (enableCSM && numCascades > 0) {
            shadow = CSMShadowCalculation(norm, lightDir);
        } else {
            shadow = LegacyShadowCalculation(FragPosLightSpace, norm, lightDir);
        }
    }
    
    // Clamp shadow to prevent total darkness
    shadow = clamp(shadow, 0.0, 0.85);
    
    // Final lighting
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular) * albedo;
    
    // Point light contribution
    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (pointLightConstant + pointLightLinear * distance + 
                               pointLightQuadratic * (distance * distance));
    
    vec3 pointDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation * albedo;
    
    vec3 pointHalf = normalize(pointDir + viewDir);
    float pointSpec = pow(max(dot(norm, pointHalf), 0.0), material.shininess);
    vec3 pointSpecular = pointSpec * pointLightColor * attenuation * 0.5;
    
    vec3 color = lighting + pointDiffuse + pointSpecular;
    
    // Cascade visualization overlay (debug feature)
    if (enableCSM && visualizeCascades) {
        int cascade = GetCascadeIndex();
        vec3 cascadeColor;
        if (cascade == 0) cascadeColor = vec3(1.0, 0.0, 0.0);
        else if (cascade == 1) cascadeColor = vec3(0.0, 1.0, 0.0);
        else if (cascade == 2) cascadeColor = vec3(0.0, 0.0, 1.0);
        else cascadeColor = vec3(1.0, 1.0, 0.0);
        color = mix(color, cascadeColor, 0.3);
    }
    
    if (enableGammaCorrection) {
        color = pow(color, vec3(1.0/2.2));
    }
    
    FragColor = vec4(color, 1.0);
    
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = brightness > bloomThreshold ? vec4(color, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
}
