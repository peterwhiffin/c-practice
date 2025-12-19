#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 5) uniform vec3 pos;
layout(location = 6) uniform float scale;
layout(location = 7) uniform vec3 rot;

layout(location = 4) out vec2 texCoord;

void main() {
        texCoord = aTexCoord;
        vec3 finalPos = aPos * scale;

        float x = finalPos.x;
        float y = finalPos.y * cos(rot.x) - finalPos.z * sin(rot.x);
        float z = finalPos.y * sin(rot.x) + finalPos.z * cos(rot.x);

        x = x * cos(rot.y) + z * sin(rot.y);
        y = y;
        z = -x * sin(rot.y) + z * cos(rot.y);

        x = x * cos(rot.z) - y * sin(rot.z);
        y = x * sin(rot.z) + y * cos(rot.z);
        z = z;

        finalPos.x = x;
        finalPos.y = y;
        finalPos.z = z;

        finalPos += pos;
        gl_Position = vec4(finalPos, 1.0);
}
