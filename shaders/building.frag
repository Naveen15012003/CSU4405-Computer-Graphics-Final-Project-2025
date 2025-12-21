#version 330 core

// MRT outputs for HDR + Bloom (matching model.frag)
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

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

// Shadow
uniform sampler2D shadowMap;
uniform bool enableShadows;
uniform bool uUsePCF;

// Bloom threshold
uniform float bloomThreshold;

// Shadow calculation (matching model.frag)
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    float shadow = 0.0;
    if (uUsePCF) {
        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for(int x = -3; x <= 3; ++x) {
            for(int y = -3; y <= 3; ++y) {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 49.0;
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
    
    // Sample building texture
    vec3 albedo = texture(buildingTexture, TexCoords).rgb;
    
    // Ambient
    vec3 ambient = 0.3 * dirLightColor * albedo;
    
    // Directional light
    vec3 lightDir = normalize(-dirLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLightColor * albedo;
    
    // Specular (minimal for buildings)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
    vec3 specular = spec * dirLightColor * 0.2;
    
    // Shadow
    float shadow = 0.0;
    if (enableShadows) {
        shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);
    }
    
    vec3 dirResult = (ambient + (1.0 - shadow) * (diffuse + specular));
    
    // Point light
    float distance = length(pointLightPos - FragPos);
    float attenuation = 1.0 / (pointLightConstant + pointLightLinear * distance + 
                               pointLightQuadratic * (distance * distance));
    
    vec3 pointLightDir = normalize(pointLightPos - FragPos);
    float pointDiff = max(dot(norm, pointLightDir), 0.0);
    vec3 pointDiffuse = pointDiff * pointLightColor * attenuation * albedo;
    
    vec3 pointResult = pointDiffuse;
    
    // Combine
    vec3 color = dirResult + pointResult;
    
    // Output to MRT
    FragColor = vec4(color, 1.0);
    
    // Brightness threshold for bloom
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > bloomThreshold) {
        BrightColor = vec4(color, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
