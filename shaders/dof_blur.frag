#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;
uniform bool horizontal;
uniform float blurScale;  // Control blur intensity

// 13-tap Gaussian blur weights
// These are pre-calculated for a standard deviation that gives good bokeh-like blur
const float weight[7] = float[](0.1964825501511, 0.2969069646728, 0.0918400000, 0.0516136137, 0.0182411, 0.00540135, 0.001);

void main()
{
    vec2 texelSize = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, TexCoords).rgb * weight[0];
    
    if (horizontal) {
        for (int i = 1; i < 7; ++i) {
            vec2 offset = vec2(texelSize.x * float(i) * blurScale, 0.0);
            result += texture(image, TexCoords + offset).rgb * weight[i];
            result += texture(image, TexCoords - offset).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 7; ++i) {
            vec2 offset = vec2(0.0, texelSize.y * float(i) * blurScale);
            result += texture(image, TexCoords + offset).rgb * weight[i];
            result += texture(image, TexCoords - offset).rgb * weight[i];
        }
    }
    
    FragColor = vec4(result, 1.0);
}
