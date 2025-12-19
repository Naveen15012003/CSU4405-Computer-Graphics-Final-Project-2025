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
    // Simple textured output (no lighting in Phase 2)
    vec4 texColor = texture(material.diffuse1, TexCoords);
    
    // If no texture or texture is black, use a default color
    if (texColor.rgb == vec3(0.0))
    {
        FragColor = vec4(0.8, 0.8, 0.8, 1.0);  // Default gray
    }
    else
    {
        FragColor = texColor;
    }
}
