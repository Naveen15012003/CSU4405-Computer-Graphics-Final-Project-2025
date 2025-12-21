#version 330 core

// MRT outputs for HDR + Bloom compatibility
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoords;

uniform sampler2D skyboxAtlas;

void main()
{
    vec3 color = texture(skyboxAtlas, TexCoords).rgb;
    
    // Output to MRT
    FragColor = vec4(color, 1.0);
    
    // Skybox doesn't contribute to bloom
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
