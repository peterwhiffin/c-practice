#version 460 core
layout(location = 4) in vec2 texCoord;
layout(location = 8) in vec3 normal;

layout(binding = 0) uniform sampler2D tex;
layout(location = 10) uniform vec4 color;
layout(location = 11) uniform vec3 lightDir;

layout(location = 0) out vec4 fragColor;

layout(location = 12) uniform float lightActive;

void main() {
        vec3 texColor = texture(tex, texCoord).rgb;

        // if (texColor.a < 0.1f)
        //         discard;

        if (lightActive != 0.0) {
                texColor += texColor * dot(lightDir, normal);
        }

        fragColor = vec4(texColor, 1.0);
}
