#version 460 core

layout(location = 0) in vec3 aPos;

layout(location = 4) out vec3 TexCoords;

layout(location = 5) uniform mat4 view;
layout(location = 6) uniform mat4 projection;

void main() {
        TexCoords = aPos;
        vec4 pos = projection * view * vec4(aPos, 1.0);
        gl_Position = pos.xyww;
}
