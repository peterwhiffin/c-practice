// #version 460 core
//
// layout(location = 0) out vec4 frag_color;
//
// layout(location = 4) in vec2 tex_coord;
// layout(location = 5) uniform vec2 resolution;
// layout(location = 6) uniform mat4 inv_view_proj;
//
// float sdf_sphere(vec3 position, float radius) {
//         return length(position) - radius;
// }
//
// vec3 WorldPosFromDepth(float depth, vec2 screenSize, mat4 invViewProj)
// {
//         float z = depth * 2.0 - 1.0; // [0, 1] -> [-1, 1]
//         // vec2 normalized = gl_FragCoord.xy / screenSize; // [0.5, u_viewPortSize] -> [0, 1]
//         vec2 normalized = gl_FragCoord.xy / vec2(800, 600); // [0.5, u_viewPortSize] -> [0, 1]
//         vec4 clipSpacePosition = vec4(normalized * 2.0 - 1.0, z, 1.0); // [0, 1] -> [-1, 1]
//
//         // undo view + projection
//         vec4 worldSpacePosition = invViewProj * clipSpacePosition;
//         worldSpacePosition /= worldSpacePosition.w;
//
//         return worldSpacePosition.xyz;
// }
//
// void main() {
//         vec3 ray_origin = WorldPosFromDepth(0.0, resolution, inv_view_proj);
//         vec3 ray_end = WorldPosFromDepth(1.0, resolution, inv_view_proj);
//         vec3 ray_dir = normalize(ray_end - ray_origin);
//
//         float epsilon = 0.01;
//
//         float total_distance = 0.0;
//         vec4 final_color = vec4(0.0, 0.0, 0.0, 1.0);
//
//         for (int i = 0; i < 100; i++) {
//                 vec3 position = ray_origin + ray_dir * total_distance;
//                 float d = sdf_sphere(position, 1.0);
//                 if (d < epsilon) {
//                         final_color = vec4(0.0, 0.0, 1.0, 1.0);
//                         break;
//                 }
//                 total_distance += d;
//         }
//
//         frag_color = final_color;
// }

#version 460 core

layout(location = 0) out vec4 frag_color;

layout(location = 4) in vec2 tex_coord;
layout(location = 5) uniform vec2 resolution;
layout(location = 6) uniform mat4 inv_view_proj;

layout(location = 7) uniform vec3 lightDir;
layout(location = 8) uniform float time;

float light_brightness = 3.0;

float sdf_sphere(vec3 position, float radius) {
        return length(position) - radius;
}

float sdf_box(vec3 p, vec3 b) {
        vec3 q = abs(p) - b;
        return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float opSmoothUnion(float a, float b, float k)
{
        k *= 4.0;
        float h = max(k - abs(a - b), 0.0);
        return min(a, b) - h * h * 0.25 / k;
}

vec3 WorldPosFromDepth(float depth, vec2 screenSize, mat4 invViewProj)
{
        float z = depth * 2.0 - 1.0; // [0, 1] -> [-1, 1]
        // vec2 normalized = gl_FragCoord.xy / screenSize; // [0.5, u_viewPortSize] -> [0, 1]
        vec2 normalized = gl_FragCoord.xy / vec2(800, 600); // [0.5, u_viewPortSize] -> [0, 1]
        vec4 clipSpacePosition = vec4(normalized * 2.0 - 1.0, z, 1.0); // [0, 1] -> [-1, 1]

        // undo view + projection
        vec4 worldSpacePosition = invViewProj * clipSpacePosition;
        worldSpacePosition /= worldSpacePosition.w;

        return worldSpacePosition.xyz;
}

float map(vec3 p) {
        // sdf_sphere(p, 0.5);
        // sdf_sphere(p, 0.5);
        vec3 spherePos = vec3(2.0, 4.0 + sin(time), -5.0); // The world position
        // float radius = 1.0;

        // Transform p into the sphere's local space
        vec3 localP1 = p - spherePos;

        vec3 boxPos = vec3(4.0, 2.0, -5.0); // The world position
        vec3 localP2 = p - boxPos;

        // return sdSphere(localP, radius);
        return opSmoothUnion(sdf_sphere(localP1, 0.5), sdf_box(localP2, vec3(40.0, 2.0, 40.0)), 0.05);
}

vec3 calcNormal(vec3 p) // for function f(p)
{
        const float h = 0.0001; // replace by an appropriate value
        const vec2 k = vec2(1, -1);
        return normalize(k.xyy * map(p + k.xyy * h) +
                        k.yyx * map(p + k.yyx * h) +
                        k.yxy * map(p + k.yxy * h) +
                        k.xxx * map(p + k.xxx * h));
}

// layout(location = 11) uniform vec3 lightDir;
//
// layout(location = 0) out vec4 fragColor;
//
// layout(location = 12) uniform float lightActive;
//
// layout(location = 25) in vec3 fragPos;
// float light_brightness = 1.0;
//
// void main() {
//         vec4 texColor = texture(tex, texCoord);
//
//         vec3 norm = normalize(normal);
//         vec3 albedo = texColor.rgb;
//         vec3 direction = normalize(lightDir - fragPos);
//         float diff = max(dot(norm, direction), 0.0);
//
//         if (lightActive != 0.0) {
//                 // albedo += albedo * dot(lightDir, normal) * light_brightness + 0.0;
//                 albedo = albedo * diff * light_brightness + 0.0;
//         }
//
//         fragColor = vec4(albedo, texColor.a) * color;
//         // fragColor = vec4(vec3(norm), texColor.a) * color;
// }

void main() {
        vec3 ray_origin = WorldPosFromDepth(0.0, resolution, inv_view_proj);
        vec3 ray_end = WorldPosFromDepth(1.0, resolution, inv_view_proj);
        vec3 ray_dir = normalize(ray_end - ray_origin);

        float epsilon = 0.01;

        float total_distance = 0.0;
        vec4 final_color = vec4(0.0, 0.0, 0.0, 1.0);

        for (int i = 0; i < 100; i++) {
                vec3 position = ray_origin + ray_dir * total_distance;
                float d = map(position);
                // float d = sdf_sphere(position, 1.0);
                if (d < epsilon) {
                        final_color = vec4(0.0, 0.0, 1.0, 1.0);
                        // final_color = vec4(d, d, d, 1.0);
                        // total_distance += d;
                        position = ray_origin + ray_dir * total_distance;
                        vec3 direction = normalize(lightDir - position);
                        vec3 norm = calcNormal(position);
                        float diff = max(dot(norm, direction), 0.0);
                        float tester = dot(norm, lightDir);
                        final_color = (final_color * 0.07) + vec4(final_color.xyz * diff * light_brightness, 1.0);

                        // final_color = norm;
                        break;
                }
                total_distance += d;
        }

        frag_color = final_color;
        // frag_color = vec4(1.0, 0.0, 0.0, 1.0);
}
