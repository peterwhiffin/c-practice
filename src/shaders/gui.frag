#version 460 core
layout(location = 4) in vec2 texCoord;

// layout(binding = 0) uniform sampler2D tex;

layout(location = 10) uniform vec4 color;
layout(location = 0) out vec4 fragColor;

void main() {
        // vec3 texColor = texture(tex, texCoord).rgb;
        fragColor = color;
}
