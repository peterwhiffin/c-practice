#version 460 core
layout(location = 10) in vec2 TexCoords;

layout(location = 11) uniform vec3 textColor;

layout(binding = 0) uniform sampler2D text;

out vec4 color;

void main()
{
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        color = vec4(textColor, 1.0) * sampled;
        // color = vec4(TexCoords.x, TexCoords.y, 0.0, 1.0);
}
