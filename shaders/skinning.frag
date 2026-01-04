#version 330 core

// Input from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

// Output
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

// Uniforms - Lighting
uniform vec3 uDirLightDir;
uniform vec3 uDirLightColor;
uniform vec3 uPointLightPos;
uniform vec3 uPointLightColor;
uniform float uPointLightConstant;
uniform float uPointLightLinear;
uniform float uPointLightQuadratic;

// Uniforms - Shadow
uniform sampler2D uShadowMap;
uniform int uEnableShadows;
uniform int uUsePCF;

// Uniforms - Material
uniform float uBloomThreshold;
uniform int uForceEmissive;

// Shadow calculation with PCF
float ShadowCalculation(vec4 fragPosLightSpace)
{
    if (uEnableShadows == 0) return 0.0;
    
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Outside shadow map bounds
    if (projCoords.z > 1.0)
        return 0.0;
    
    // Current depth
    float currentDepth = projCoords.z;
    
    // Bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(Normal, -uDirLightDir)), 0.001);
    
    float shadow = 0.0;
    
    if (uUsePCF == 1) {
        // PCF (Percentage Closer Filtering)
        vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    } else {
        // Simple shadow
        float closestDepth = texture(uShadowMap, projCoords.xy).r;
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }
    
    return shadow;
}

void main()
{
    // Base color (white for now, could add texture sampling here)
    vec3 color = vec3(1.0, 1.0, 1.0);
    
    // Force emissive cyan for visibility debug
    if (uForceEmissive == 1) {
        FragColor = vec4(0.0, 1.0, 1.0, 1.0);
        BrightColor = vec4(0.0, 1.0, 1.0, 1.0);
        return;
    }
    
    vec3 normal = normalize(Normal);
    
    // Ambient
    vec3 ambient = 0.2 * color;
    
    // Directional light
    vec3 lightDir = normalize(-uDirLightDir);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uDirLightColor * color;
    
    // Point light
    vec3 pointLightDir = normalize(uPointLightPos - FragPos);
    float pointDiff = max(dot(normal, pointLightDir), 0.0);
    float distance = length(uPointLightPos - FragPos);
    float attenuation = 1.0 / (uPointLightConstant + uPointLightLinear * distance + 
                               uPointLightQuadratic * (distance * distance));
    vec3 pointDiffuse = pointDiff * uPointLightColor * color * attenuation;
    
    // Shadow
    float shadow = ShadowCalculation(FragPosLightSpace);
    
    // Combine lighting (shadow only affects directional light)
    vec3 lighting = ambient + (1.0 - shadow) * diffuse + pointDiffuse;
    
    FragColor = vec4(lighting, 1.0);
    
    // Bright color for bloom
    float brightness = dot(lighting, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > uBloomThreshold) {
        BrightColor = vec4(lighting, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
