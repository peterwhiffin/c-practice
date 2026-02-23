#version 460 core

layout(location = 0) out vec4 frag_color;

layout(binding = 0) uniform sampler2D scene_tex;
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
float opSmoothSubtraction(float a, float b, float k)
{
        return -opSmoothUnion(a, -b, k);

        // k *= 4.0;
        // float h = max(k-abs(-a-b),0.0);
        // return max(-a, b) + h*h*0.25/k;
}

float opSmoothIntersection(float a, float b, float k)
{
        return -opSmoothUnion(-a, -b, k);

        // k *= 4.0;
        // float h = max(k-abs(a-b),0.0);
        // return max(a, b) + h*h*0.25/k;
}

vec3 WorldPosFromDepth(float depth, vec2 screenSize, mat4 invViewProj)
{
        float z = depth * 2.0 - 1.0; // [0, 1] -> [-1, 1]
        vec2 normalized = gl_FragCoord.xy / vec2(800, 600); // [0.5, u_viewPortSize] -> [0, 1]
        vec4 clipSpacePosition = vec4(normalized * 2.0 - 1.0, z, 1.0); // [0, 1] -> [-1, 1]

        // undo view + projection
        vec4 worldSpacePosition = invViewProj * clipSpacePosition;
        worldSpacePosition /= worldSpacePosition.w;

        return worldSpacePosition.xyz;
}

float map(vec3 p) {
        vec3 boxPos = p - vec3(0.0, 0.0, 0.0); // The world position
        vec3 box2Pos = p - vec3(4.5, 2.0, 0.0); // The world position
        vec3 spherePos = p - vec3(0.0, 0.0 + sin(time), 0.0); // The world position
        vec3 sphere2Pos = p - vec3(0.0, 1.5, 0.0);
        vec3 testSphere = spherePos;

        float floor1 = sdf_box(boxPos, vec3(5.0, 0.5, 5.0));
        float wall1 = sdf_box(box2Pos, vec3(0.4, 3.0, 5.0));
        float sphere1 = sdf_sphere(spherePos, 0.5);
        float sphere2 = sdf_sphere(sphere2Pos, 0.5);
        float testSphere1 = sdf_sphere(testSphere, 0.5);

        float m = opSmoothUnion(floor1, sphere1, 0.05);
        m = opSmoothUnion(m, sphere2, 0.05);
        m = opSmoothUnion(m, wall1, 0.05);
        m = opSmoothUnion(m, floor1, 0.05);
        // float m = opSmoothSubtraction(sphere1, floor1, 0.05);
        // m = min(m, testSphere1);
        return m;
}

float calcSoftshadow(in vec3 ro, in vec3 rd, float tmin, float tmax, const float k)
{
        float res = 1.0;
        float t = tmin;
        for (int i = 0; i < 64; i++)
        {
                float h = map(ro + rd * t);
                res = min(res, k * h / t);
                t += clamp(h, 0.003, 0.10);
                if (res < 0.002 || t > tmax) break;
        }
        return clamp(res, 0.0, 1.0);
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

void main() {
        vec3 ray_origin = WorldPosFromDepth(0.0, resolution, inv_view_proj);
        vec3 ray_end = WorldPosFromDepth(1.0, resolution, inv_view_proj);
        vec3 ray_dir = normalize(ray_end - ray_origin);

        float epsilon = 0.0000001;

        float total_distance = 0.0;
        float max_distance = 1000.0;
        vec3 final_color = vec3(0.0, 0.0, 0.0);

        int max_steps = 80;

        vec3 fog = vec3(0.0);
        float accum = 0.0;
        int steps = 0;
        float alpha = 1.0;

        for (steps = 0; steps < max_steps; steps++) {
                vec3 position = ray_origin + ray_dir * total_distance;
                float d = map(position);
                accum += 1.0 + .03 * steps;

                if (d < epsilon || total_distance > max_distance) {
                        break;
                }
                total_distance += d;
        }

        // final_color = vec3(0.0, 0.0, 0.8);

        vec4 scene_color = texture(scene_tex, tex_coord);
        final_color = scene_color.rgb;
        if (total_distance < max_distance) {
                final_color = vec3(1.0, 1.0, 1.0);
                vec3 position = ray_origin + ray_dir * total_distance;
                // vec3 direction = normalize(lightDir - position);
                vec3 norm = calcNormal(position);
                // vec3 lig = vec3(0.57703);
                // vec3 lig = normalize(lightDir);
                vec3 lig = normalize(lightDir - position);
                // float diff = max(dot(norm, lig), 0.0);
                float diff = clamp(dot(norm, lig), 0.0, 1.0);

                if (diff > 0.001) {
                        float shadow = calcSoftshadow(position + norm * 0.001, lig, 0.999, 07.0, 32.0);
                        diff *= shadow;
                }
                // float amb = 0.5 + 0.5 * dot(norm, vec3(0.0, 1.0, 0.0));
                float amb = 0.5 + 0.5 * dot(norm, lig);
                // amb = 0.1;

                float lig_intensity = 0.5;
                // float tester = dot(norm, lightDir);
                // final_color = (final_color * 0.02) + vec4(final_color.xyz * diff * light_brightness, 1.0);
                final_color = vec3(0.2, 0.2, 0.2) * amb + vec3(1.0, 1.0, 1.0) * diff * lig_intensity;
                // final_color = norm * amb + norm * diff * lig_intensity;

                // final_color = vec3(steps / 2000);
        }

        // final_color += final_color * accum * (total_distance / steps);

        frag_color = vec4(final_color, alpha);
}
