#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneColor;      // HDR scene color
uniform sampler2D sceneDepth;      // Linear depth texture
uniform sampler2D blurredScene;    // Pre-blurred version of scene

// DoF parameters
uniform float focusDistance;       // Distance to focus plane (in world units)
uniform float focusRange;          // Range of sharp focus
uniform float maxBlur;             // Maximum blur amount (0-1)
uniform float aperture;            // Simulated aperture (affects bokeh intensity)
uniform bool enableDoF;            // Toggle DoF effect
uniform int debugMode;             // 0=normal, 5=show depth, 6=show CoC

// Camera parameters
uniform float nearPlane;
uniform float farPlane;

// Convert depth buffer value to linear depth
float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

// Calculate Circle of Confusion
// Returns blur amount based on distance from focus plane
float calculateCoC(float depth) {
    // Simple CoC calculation based on distance from focus plane
    float diff = abs(depth - focusDistance);
    
    // Smooth transition from sharp to blurred
    float coc = smoothstep(0.0, focusRange, diff);
    
    // Scale by max blur and aperture
    coc = coc * maxBlur * aperture;
    
    return clamp(coc, 0.0, 1.0);
}

// High-quality bokeh disk sampling
vec3 bokehBlur(vec2 uv, float coc) {
    if (coc < 0.001) {
        return texture(sceneColor, uv).rgb;
    }
    
    vec3 color = vec3(0.0);
    float totalWeight = 0.0;
    
    // Disk sampling pattern (golden angle spiral for uniform distribution)
    const int SAMPLES = 32;
    const float GOLDEN_ANGLE = 2.39996323;
    
    float blurRadius = coc * 0.02; // Scale blur based on CoC
    
    for (int i = 0; i < SAMPLES; i++) {
        float r = sqrt(float(i) / float(SAMPLES)) * blurRadius;
        float theta = float(i) * GOLDEN_ANGLE;
        
        vec2 offset = vec2(cos(theta), sin(theta)) * r;
        vec2 sampleUV = uv + offset;
        
        // Sample depth at this location
        float sampleDepth = linearizeDepth(texture(sceneDepth, sampleUV).r);
        float sampleCoC = calculateCoC(sampleDepth);
        
        // Weight samples: prefer sharp samples over blurry background bleeding
        float weight = 1.0;
        
        // Prevent background from bleeding into foreground
        if (sampleDepth > focusDistance && linearizeDepth(texture(sceneDepth, uv).r) < focusDistance) {
            weight *= 0.5;
        }
        
        color += texture(sceneColor, sampleUV).rgb * weight;
        totalWeight += weight;
    }
    
    return color / max(totalWeight, 0.001);
}

// Simple linear interpolation DoF (faster, good quality)
vec3 simpleDoF(vec2 uv, float coc) {
    vec3 sharp = texture(sceneColor, uv).rgb;
    vec3 blurred = texture(blurredScene, uv).rgb;
    
    // Lerp between sharp and blurred based on CoC
    return mix(sharp, blurred, coc);
}

void main()
{
    // Sample depth
    float depth = texture(sceneDepth, TexCoords).r;
    float linearDepth = linearizeDepth(depth);
    
    // Debug: Show depth visualization
    if (debugMode == 5) {
        // Visualize linear depth (normalized for display)
        float normalizedDepth = (linearDepth - nearPlane) / (farPlane - nearPlane);
        FragColor = vec4(vec3(normalizedDepth), 1.0);
        return;
    }
    
    // Calculate Circle of Confusion
    float coc = calculateCoC(linearDepth);
    
    // Debug: Show CoC visualization
    if (debugMode == 6) {
        // Red = max blur, Green = in focus
        vec3 cocColor = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), coc);
        FragColor = vec4(cocColor, 1.0);
        return;
    }
    
    // Apply DoF effect
    if (!enableDoF) {
        FragColor = vec4(texture(sceneColor, TexCoords).rgb, 1.0);
        return;
    }
    
    // Use simple interpolation DoF (high quality with pre-blurred texture)
    vec3 color = simpleDoF(TexCoords, coc);
    
    FragColor = vec4(color, 1.0);
}
