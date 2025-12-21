#version 330 core

// MRT outputs for HDR + Bloom
layout(location = 0) out vec4 FragColor;      // Main HDR color
layout(location = 1) out vec4 BrightColor;    // Brightness for bloom

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
    }
    
    vec3 dirResult = (ambient + (1.0 - shadow) * (diffuse + specular));
    
    // Point light calculation with attenuation
    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (pointLightConstant + pointLightLinear * distance + 
                               pointLightQuadratic * (distance * distance));
    
    vec3 pointLightDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointLightDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation;
    
    vec3 pointHalfway = normalize(pointLightDir + viewDir);
    float pointSpec = pow(max(dot(norm, pointHalfway), 0.0), material.shininess);
    vec3 pointSpecular = pointSpec * pointLightColor * attenuation * 0.5;
    
    vec3 pointResult = (pointDiffuse + pointSpecular);
    
    // Combine
    vec3 result = dirResult + pointResult;
    vec3 color = result * texture(material.diffuse1, TexCoords).rgb;
    
    // Gamma correction (if enabled)
    if (enableGammaCorrection)
    {
        color = pow(color, vec3(1.0/2.2));
    }
    
    // Output to MRT
    FragColor = vec4(color, 1.0);
    
    // Brightness threshold for bloom
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
