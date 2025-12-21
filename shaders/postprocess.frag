#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;
uniform float exposure;
uniform bool enableBloom;
uniform bool enableGamma;
uniform float bloomStrength;
uniform int debugMode;

// ACES Filmic Tone Mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Reinhard Tone Mapping (alternative)
vec3 Reinhard(vec3 x) {
    return x / (x + vec3(1.0));
}

void main()
{
    // Sample the bound texture (already correct based on debug mode)
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    
    // Debug mode: show textures with enhanced visibility
    if (debugMode == 1) {
        // Show HDR color with tone mapping for visibility
        vec3 color = hdrColor * exposure;
        color = ACESFilm(color);
        FragColor = vec4(color, 1.0);
        return;
    }
    else if (debugMode == 2) {
        // Show bright pass (already bound to hdrBuffer in this mode)
        // Amplify for visibility since bright pixels are usually sparse
        vec3 color = hdrColor * 3.0;
        FragColor = vec4(color, 1.0);
        return;
    }
    else if (debugMode == 3) {
        // Show bloom blur result (already bound to hdrBuffer in this mode)
        // Amplify for visibility
        vec3 color = hdrColor * 5.0;
        FragColor = vec4(color, 1.0);
        return;
    }
    
    // Normal mode: Full HDR pipeline
    vec3 color = hdrColor;
    
    // Add bloom if enabled
    if (enableBloom) {
        vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
        color += bloomColor * bloomStrength;
    }
    
    // Apply exposure tone mapping
    color *= exposure;
    
    // ACES tone mapping for film-like response
    color = ACESFilm(color);
    
    // Gamma correction (only if explicitly enabled)
    if (enableGamma) {
        color = pow(color, vec3(1.0 / 2.2));
    }
    
    FragColor = vec4(color, 1.0);
}
