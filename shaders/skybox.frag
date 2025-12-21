#version 330 core

// MRT outputs for HDR + Bloom (same as model shader)
layout(location = 0) out vec4 FragColor;      // Main HDR color
layout(location = 1) out vec4 BrightColor;    // Brightness for bloom

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    vec3 color = texture(skybox, TexCoords).rgb;
    
    // Output to MRT
    FragColor = vec4(color, 1.0);
    
    // Skybox is usually not bright enough for bloom, so output black
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
