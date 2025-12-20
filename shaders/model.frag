#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct Material {
    sampler2D diffuse1;
};

uniform Material material;

void main()
{
    // Sample the texture
    vec4 texColor = texture(material.diffuse1, TexCoords);
    
    // Simple lighting calculation for fallback
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Ambient + diffuse lighting
    float ambient = 0.4;
    float lighting = ambient + (0.6 * diff);
    
    // Check if texture is valid
    // A fully black texture with alpha 0 or very low likely means no texture bound
    if (texColor.a < 0.01 || (texColor.r + texColor.g + texColor.b < 0.01))
    {
        // No valid texture - use fallback with simple lighting
        vec3 baseColor = vec3(0.6, 0.7, 0.8);  // Light blue-gray
        FragColor = vec4(baseColor * lighting, 1.0);
    }
    else
    {
        // Valid texture exists - apply simple lighting to it
        vec3 texturedColor = texColor.rgb * lighting;
        FragColor = vec4(texturedColor, texColor.a);
    }
}
