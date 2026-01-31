#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

out vec4 color;

layout(location = 2) uniform mat4 view_proj;

// layout(std140, binding = 0) uniform global {
//         mat4 view;
//         mat4 projection;
// };

void main() {
        color = aColor;
        // gl_Position = projection * view * vec4(aPos, 1.0);
        gl_Position = view_proj * vec4(aPos, 1.0);
}
