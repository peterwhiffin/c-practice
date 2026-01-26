#version 460 core

layout(location = 0) out vec4 FragColor;

layout(location = 4) in vec3 TexCoords;

layout(binding = 0) uniform samplerCube skybox;
layout(location = 10) uniform vec3 color;

void main() {
        FragColor = texture(skybox, TexCoords) * vec4(color, 1.0);
        // FragColor = vec4(TexCoords.x, TexCoords.y, TexCoords.z, 1.0);
}
