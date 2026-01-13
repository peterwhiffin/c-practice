#version 460 core
layout(location = 4) in vec2 texCoord;

layout(binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 fragColor;

layout(location = 13) uniform vec2 iResolution;
layout(location = 14) uniform vec2 iMouse;

// void main()
// {
//         vec2 mouse = vec2(iMouse.x, iMouse.y);
//         vec2 p = gl_FragCoord.xy;
//         vec2 R = iResolution;
//         vec2 u = p / R;
//         vec2 m = mouse.xy / R;
//         vec2 o = m - u;
//         vec2 l;
//         vec2 i;
//         o.x *= R.x / R.y;
//
//         float d = length(o);
//
//         fragColor = mix(vec4(0.0),
//
//                         texture(tex, u + o / d * .01 / (d * d)),
//                         step(.1, d));
// }

void main() {
        vec3 texColor = texture(tex, texCoord).rgb;
        fragColor = vec4(texColor, 1.0);
}
