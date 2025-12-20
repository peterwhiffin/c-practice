#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 5) uniform vec3 pos;
layout(location = 6) uniform float scale;
layout(location = 7) uniform vec3 rot;
layout(location = 9) uniform mat4 viewProj;
layout(location = 13) uniform mat4 model;

layout(location = 4) out vec2 texCoord;
layout(location = 8) out vec3 normal;

void main() {
        texCoord = aTexCoord;
        // vec3 finalPos = aPos * scale;
        //
        // float x = finalPos.x;
        // float y = finalPos.y * cos(rot.x) - finalPos.z * sin(rot.x);
        // float z = finalPos.y * sin(rot.x) + finalPos.z * cos(rot.x);
        //
        // x = x * cos(rot.y) + z * sin(rot.y);
        // y = y;
        // z = -x * sin(rot.y) + z * cos(rot.y);
        //
        // x = x * cos(rot.z) - y * sin(rot.z);
        // y = x * sin(rot.z) + y * cos(rot.z);
        // z = z;
        //
        // finalPos.x = x;
        // finalPos.y = y;
        // finalPos.z = z;

        vec3 finalPos = vec3(model * vec4(aPos, 1.0));
        normal = aNormal * finalPos;

        gl_Position = viewProj * vec4(finalPos, 1.0);
}
