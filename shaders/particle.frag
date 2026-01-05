#version 330 core

in vec2 TexCoords;
in vec4 ParticleColor;
in float LifeRatio;

// MRT outputs for HDR + Bloom (matching other shaders)
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

uniform sampler2D particleTexture;
uniform bool useTexture;
uniform int particleType;  // 0 = fire, 1 = smoke, 2 = spark, 3 = magic
uniform float bloomThreshold;  // Bloom threshold (default to 1.0 if not set)

// Procedural circle/soft particle
float softCircle(vec2 uv) {
    vec2 center = uv - vec2(0.5);
    float dist = length(center) * 2.0;
    return 1.0 - smoothstep(0.0, 1.0, dist);
}

// Procedural fire pattern
vec3 fireColor(float life) {
    // Fire gradient: white -> yellow -> orange -> red -> dark
    vec3 white = vec3(1.0, 1.0, 0.9);
    vec3 yellow = vec3(1.0, 0.9, 0.2);
    vec3 orange = vec3(1.0, 0.5, 0.1);
    vec3 red = vec3(0.8, 0.2, 0.1);
    vec3 dark = vec3(0.2, 0.05, 0.0);
    
    if (life < 0.2) return mix(white, yellow, life / 0.2);
    else if (life < 0.4) return mix(yellow, orange, (life - 0.2) / 0.2);
    else if (life < 0.7) return mix(orange, red, (life - 0.4) / 0.3);
    else return mix(red, dark, (life - 0.7) / 0.3);
}

// Procedural smoke pattern
vec3 smokeColor(float life) {
    // Smoke: light gray -> dark gray
    float gray = mix(0.6, 0.2, life);
    return vec3(gray);
}

// Procedural spark pattern
vec3 sparkColor(float life) {
    // Sparks: bright yellow-white that dims
    vec3 bright = vec3(1.0, 0.95, 0.7);
    vec3 dim = vec3(1.0, 0.6, 0.2);
    return mix(bright, dim, life);
}

// Procedural magic pattern
vec3 magicColor(float life, vec2 uv) {
    // Magic: cycling colors with sparkle
    float hue = life * 2.0 + length(uv - 0.5) * 3.0;
    vec3 color;
    color.r = sin(hue) * 0.5 + 0.5;
    color.g = sin(hue + 2.094) * 0.5 + 0.5;
    color.b = sin(hue + 4.188) * 0.5 + 0.5;
    return color * 1.5;  // HDR for bloom
}

void main()
{
    float alpha = softCircle(TexCoords);
    
    if (alpha < 0.01) {
        discard;
    }
    
    vec3 color;
    float finalAlpha = alpha * ParticleColor.a;
    
    if (useTexture) {
        vec4 texColor = texture(particleTexture, TexCoords);
        color = texColor.rgb * ParticleColor.rgb;
        finalAlpha *= texColor.a;
    } else {
        // Procedural colors based on particle type
        if (particleType == 0) {
            // Fire
            color = fireColor(LifeRatio) * ParticleColor.rgb;
            // Fire is emissive - boost for HDR/bloom
            color *= 2.0;
        } else if (particleType == 1) {
            // Smoke
            color = smokeColor(LifeRatio) * ParticleColor.rgb;
            finalAlpha *= 0.6;  // Smoke is more transparent
        } else if (particleType == 2) {
            // Sparks
            color = sparkColor(LifeRatio) * ParticleColor.rgb;
            color *= 3.0;  // Very bright for bloom
            finalAlpha = alpha * ParticleColor.a * (1.0 - LifeRatio);
        } else if (particleType == 3) {
            // Magic
            color = magicColor(LifeRatio, TexCoords);
            color *= 2.5;  // HDR for bloom
        } else {
            // Default
            color = ParticleColor.rgb;
        }
    }
    
    FragColor = vec4(color, finalAlpha);
    
    // MRT: Output bright pixels to second attachment for bloom
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float threshold = bloomThreshold > 0.0 ? bloomThreshold : 1.0;
    if (brightness > threshold) {
        BrightColor = vec4(color, finalAlpha);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
