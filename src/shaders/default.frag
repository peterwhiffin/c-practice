#version 460 core
layout(location = 4) in vec2 texCoord;

layout(binding = 0) uniform sampler2D tex;
layout(location = 10) uniform vec4 color;

layout(location = 0) out vec4 fragColor;

void main() {
        vec4 texColor = texture(tex, texCoord);

        if (texColor.a < 0.1f)
                discard;

        fragColor = texColor;
}
