#version 460 core
layout(location = 4) in vec2 texCoord;
layout(location = 8) in vec3 normal;

layout(binding = 0) uniform sampler2D tex;
layout(location = 10) uniform vec4 color;
layout(location = 11) uniform vec3 lightDir;

layout(location = 0) out vec4 fragColor;

layout(location = 12) uniform float lightActive;

float light_brightness = 1.0;

void main() {
        vec4 texColor = texture(tex, texCoord);

        vec3 albedo = texColor.rgb;

        if (lightActive != 0.0) {
                albedo += albedo * dot(lightDir, normal) * light_brightness + 0.0;
        }

        fragColor = vec4(albedo, texColor.a) * color;
}
