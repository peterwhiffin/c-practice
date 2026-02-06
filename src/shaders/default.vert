#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 5) uniform vec3 pos;
layout(location = 6) uniform float scale;
layout(location = 7) uniform vec3 rot;
layout(location = 9) uniform mat4 viewProj;
layout(location = 13) uniform mat4 model;

layout(location = 23) uniform float time;
layout(location = 24) uniform float normal_expansion;

layout(location = 4) out vec2 texCoord;
layout(location = 8) out vec3 normal;

layout(location = 25) out vec3 fragPos;

void main() {
        texCoord = aTexCoord;
        mat3 normalMat = transpose(inverse(mat3(model)));
        normal = normalize(normalMat * aNormal);
        fragPos = (model * vec4(aPos, 1.0)).xyz;
        vec3 temp_pos = aPos;
        vec3 finalPos = vec3(model * vec4(temp_pos, 1.0));

        // gl_Position = viewProj * vec4(finalPos + normal * normal_expansion, 1.0);
        gl_Position = viewProj * model * vec4(aPos, 1.0);
}
