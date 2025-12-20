#version 330 core

out vec4 FragColor;

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

// Shadow calculation with HIGHLY VISIBLE PCF difference
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if outside shadow map
    if(projCoords.z > 1.0)
        return 0.0;
    
    // Get depth from light's perspective
    float currentDepth = projCoords.z;
    
    // Adaptive bias to prevent shadow acne
    float bias = max(0.008 * (1.0 - dot(normal, lightDir)), 0.002);
    
    float shadow = 0.0;
    
    if (uUsePCF)
    {
        // PCF: 7x7 kernel = 49 samples (visible soft shadows, realistic)
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        int sampleRadius = 3; // 7x7 kernel
        float radiusMultiplier = 2.0; // Moderate blur radius
        int sampleCount = 0;
        
        for(int x = -sampleRadius; x <= sampleRadius; ++x)
        {
            for(int y = -sampleRadius; y <= sampleRadius; ++y)
            {
                vec2 offset = vec2(x, y) * texelSize * radiusMultiplier;
                float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
                sampleCount++;
            }
        }
        shadow /= float(sampleCount);
        
        // Smoothstep for gradual shadow edges
        shadow = smoothstep(0.2, 0.8, shadow);
    }
    else
    {
        // Hard shadows - single sample, sharp edges
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        float hardBias = bias * 0.5;
        shadow = currentDepth - hardBias > closestDepth ? 1.0 : 0.0;
    }
    
    return shadow;
}

// Blinn-Phong lighting calculation for directional light
vec3 CalcDirLight(vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(-dirLightDir);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    
    // Combine
    vec3 ambient = 0.15 * dirLightColor * albedo;
    vec3 diffuse = diff * dirLightColor * albedo;
    vec3 specular = spec * dirLightColor * 0.5;
    
    // Shadow
    float shadow = 0.0;
    if (enableShadows)
    {
        shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);
    }
    
    // Apply shadow only to diffuse and specular
    return ambient + (1.0 - shadow) * (diffuse + specular);
}

// Point light calculation with attenuation
vec3 CalcPointLight(vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(pointLightPos - FragPos);
    
    // Attenuation
    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (pointLightConstant + pointLightLinear * distance + 
                               pointLightQuadratic * (distance * distance));
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    
    // Combine
    vec3 ambient = 0.1 * pointLightColor * albedo;
    vec3 diffuse = diff * pointLightColor * albedo;
    vec3 specular = spec * pointLightColor * 0.3;
    
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}

void main()
{
    // Sample texture
    vec4 texColor = texture(material.diffuse1, TexCoords);
    
    // Use texture color or fallback
    vec3 albedo;
    if (texColor.a < 0.01 || (texColor.r + texColor.g + texColor.b < 0.01))
    {
        albedo = vec3(0.7, 0.75, 0.8); // Light gray-blue fallback
    }
    else
    {
        albedo = texColor.rgb;
    }
    
    // Normalize inputs
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // Calculate lighting
    vec3 result = vec3(0.0);
    
    // Directional light (with shadows)
    result += CalcDirLight(norm, viewDir, albedo);
    
    // Point light (no shadows)
    result += CalcPointLight(norm, viewDir, albedo);
    
    // Gamma correction
    if (enableGammaCorrection)
    {
        result = pow(result, vec3(1.0/2.2));
    }
    
    FragColor = vec4(result, 1.0);
}
