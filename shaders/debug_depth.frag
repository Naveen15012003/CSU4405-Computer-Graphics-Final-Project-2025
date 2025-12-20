#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D shadowMap;

void main()
{
    // Sample depth from shadow map
    float depthValue = texture(shadowMap, TexCoords).r;
    
    // Simple visualization (no complex math)
    float visualDepth = 1.0 - depthValue;
    
    // Output pure grayscale
    FragColor = vec4(vec3(visualDepth), 1.0);
}
