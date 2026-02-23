#version 460 core
layout(location = 4) in vec2 texCoord;
layout(location = 8) in vec3 normal;

layout(binding = 0) uniform sampler2D tex;
layout(location = 10) uniform vec4 color;
layout(location = 11) uniform vec3 lightDir;

layout(location = 0) out vec4 fragColor;

layout(location = 12) uniform float lightActive;

layout(location = 25) in vec3 fragPos;
float light_brightness = 1.0;

void main() {
        vec4 texColor = texture(tex, texCoord);

        vec3 norm = normalize(normal);
        vec3 albedo = texColor.rgb;
        vec3 direction = normalize(lightDir - fragPos);
        float diff = max(dot(norm, direction), 0.0);

        if (lightActive != 0.0) {
                // albedo += albedo * dot(lightDir, normal) * light_brightness + 0.0;
                albedo = albedo * diff * light_brightness + 0.0;
        }

        fragColor = vec4(albedo, texColor.a) * color;
        // fragColor = vec4(vec3(norm), texColor.a) * color;
}
