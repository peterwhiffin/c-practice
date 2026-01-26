#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 10) out vec2 texCoord;

layout(location = 4) uniform mat4 projection;
layout(location = 5) uniform mat4 model;

layout(location = 6) uniform vec2 uvOffset;
layout(location = 7) uniform vec2 uvWidth;

void main()
{
        texCoord = aTexCoord;
        vec4 pos = projection * model * vec4(aPos, 0.0f, 1.0);
        pos.z = 0.0;
        gl_Position = pos;
}
