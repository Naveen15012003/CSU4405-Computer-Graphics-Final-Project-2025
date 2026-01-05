#version 330 core

// MRT outputs for HDR + Bloom
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

// Material properties
struct Material {
    sampler2D diffuse1;
    float shininess;
};

uniform Material material;

// Lighting uniforms
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform float pointLightConstant;
uniform float pointLightLinear;
uniform float pointLightQuadratic;

uniform vec3 viewPos;
uniform sampler2D shadowMap;

// Toggles
uniform bool enableShadows;
uniform bool uUsePCF;
uniform bool enableGammaCorrection;

// Bloom threshold
uniform float bloomThreshold;

// Robust shadow calculation (same as building.frag)
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform from [-1,1] to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // ============================================
    // CRITICAL: Outside shadow map = NO SHADOW
    // ============================================
    
    // Check X/Y bounds
    if (projCoords.x <= 0.0 || projCoords.x >= 1.0 ||
        projCoords.y <= 0.0 || projCoords.y >= 1.0)
    {
        return 0.0;
    }
    
    // Check Z bounds
    if (projCoords.z <= 0.0 || projCoords.z >= 1.0)
    {
        return 0.0;
    }
    
    // ============================================
    // Inside shadow map - calculate actual shadow
    // ============================================
    float currentDepth = projCoords.z;
    
    // Bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    float shadow = 0.0;
    
    if (uUsePCF)
    {
        // PCF soft shadows with 3x3 kernel
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }
    else
    {
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        shadow = (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
    }
    
    return shadow;
}

void main()
{
    // Sample texture
    vec4 texColor = texture(material.diffuse1, TexCoords);
    
    // Use texture color or fallback
    vec3 albedo;
    if (texColor.a < 0.01 || (texColor.r + texColor.g + texColor.b < 0.01))
    {
        albedo = vec3(0.7, 0.75, 0.8);
    }
    else
    {
        albedo = texColor.rgb;
    }
    
    // Normalize inputs
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Ambient
    vec3 ambient = 0.3 * dirLightColor;
    
    // Directional light
    vec3 lightDir = normalize(-dirLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLightColor;
    
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specular = spec * dirLightColor * 0.5;
    
    // Shadow
    float shadow = 0.0;
    if (enableShadows)
    {
        shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
        shadow = min(shadow, 0.6);
    }
    
    vec3 dirResult = ambient + (1.0 - shadow) * (diffuse + specular);
    
    // Point light
    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (pointLightConstant + pointLightLinear * distance + 
                               pointLightQuadratic * (distance * distance));
    
    vec3 pointLightDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointLightDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation;
    
    vec3 pointHalfway = normalize(pointLightDir + viewDir);
    float pointSpec = pow(max(dot(norm, pointHalfway), 0.0), material.shininess);
    vec3 pointSpecular = pointSpec * pointLightColor * attenuation * 0.5;
    
    vec3 pointResult = pointDiffuse + pointSpecular;
    
    // Combine
    vec3 result = dirResult + pointResult;
    vec3 color = result * albedo;
    
    // Gamma correction
    if (enableGammaCorrection)
    {
        color = pow(color, vec3(1.0/2.2));
    }
    
    // Output
    FragColor = vec4(color, 1.0);
    
    // Bloom
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > bloomThreshold)
    {
        BrightColor = vec4(color, 1.0);
    }
    else
    {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
